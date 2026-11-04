#include <algorithm>
#include <utility>

#include "dxvk_device.h"
#include "dxvk_graphics.h"
#include "dxvk_pipecompiler.h"

namespace dxvk {

  namespace {

    constexpr uint32_t DYASYNC_MIN_WORKERS         = 1;
    constexpr uint32_t DYASYNC_MAX_WORKERS         = 32;
    constexpr uint32_t DYASYNC_MAX_WORKERS_32BIT   = 16;
    constexpr uint32_t DYASYNC_QUEUE_CAPACITY      = 4096;
    constexpr uint32_t DYASYNC_BACKGROUND_INTERVAL = 8;

    enum class TakeLane : uint32_t {
      None       = 0,
      Live       = 1,
      Background = 2,
    };

    uint32_t base_worker_count(uint32_t cpu_cores) {
      return ((std::max(1u, cpu_cores) - 1u) * 5u) / 7u;
    }

    uint32_t clamp_worker_count(uint32_t count) {
      return std::clamp(count, DYASYNC_MIN_WORKERS, DYASYNC_MAX_WORKERS);
    }

    uint32_t apply_platform_cap(uint32_t count, bool is_32bit) {
      return is_32bit ? std::min(count, DYASYNC_MAX_WORKERS_32BIT) : count;
    }

    uint32_t resolve_worker_count(
            uint32_t cpu_cores,
            int32_t  config_thread_count,
            bool     is_32bit,
            bool     use_all_cores) {
      return use_all_cores           ? cpu_cores
           : config_thread_count > 0 ? uint32_t(config_thread_count)
           :                           apply_platform_cap(
                                         clamp_worker_count(base_worker_count(cpu_cores)),
                                         is_32bit);
    }

    bool prefer_background(uint32_t pick) {
      return (pick % DYASYNC_BACKGROUND_INTERVAL) == 0;
    }

    bool entry_is_valid(const DxvkPipelineEntry& entry) {
      return entry.pipeline   != nullptr
          && entry.renderPass != nullptr;
    }

    bool work_available(
            bool                           stop,
      const std::deque<DxvkPipelineEntry>& liveQueue,
      const std::deque<DxvkPipelineEntry>& backgroundQueue) {
      return stop
          || !liveQueue.empty()
          || !backgroundQueue.empty();
    }

    TakeLane select_lane(bool live_empty, bool bg_empty, bool prefer_bg) {
      return (prefer_bg && !bg_empty) ? TakeLane::Background
           : (!live_empty)            ? TakeLane::Live
           : (!bg_empty)              ? TakeLane::Background
           :                            TakeLane::None;
    }

    DxvkPipelineEntry pop_front(std::deque<DxvkPipelineEntry>& queue) {
      DxvkPipelineEntry entry = std::move(queue.front());
      queue.pop_front();
      return entry;
    }

  }


  DxvkPipelineCompiler::DxvkPipelineCompiler(const DxvkDevice* device) {
    uint32_t numWorkers = resolve_worker_count(
      dxvk::thread::hardware_concurrency(),
      device->config().numShaderCompilerThreads,
      env::is32BitHostPlatform(),
      env::getEnvVar("DXVK_ALL_CORES") == "1");

    Logger::info(str::format("DXVK: Using ", numWorkers, " dyasync compiler threads"));

    this->spawnWorkers(numWorkers);
  }


  DxvkPipelineCompiler::~DxvkPipelineCompiler() {
    this->stop();
  }


  void DxvkPipelineCompiler::stop() {
    this->requestStop();
    this->joinAll();
  }


  bool DxvkPipelineCompiler::queueCompilation(
          DxvkGraphicsPipeline*          pipeline,
    const DxvkGraphicsPipelineStateInfo& state,
    const DxvkRenderPass*                renderPass,
          DxvkPipelinePriority           priority) {
    bool pushed = this->pushEntry(DxvkPipelineEntry {
      pipeline,
      state,
      renderPass,
      priority,
    });

    this->notifyIfPushed(pushed);
    return pushed;
  }


  void DxvkPipelineCompiler::spawnWorkers(uint32_t count) {
    m_workers.reserve(count);

    for (uint32_t i = 0; i < count; i++) {
      // Creating a thread throws when the process is out of address space
      // or threads. This runs in the constructor, so letting it escape
      // would destroy m_workers with live threads in it, which std::terminate
      // answers. Run with the threads that could be started instead.
      try {
        m_workers.emplace_back([this] { this->runCompilerThread(); });
      } catch (const DxvkError& e) {
        Logger::err(str::format("DXVK: Could not start dyasync compiler thread ",
          i + 1, " of ", count, ": ", e.message()));
        break;
      }
    }

    if (m_workers.empty())
      Logger::err("DXVK: No dyasync compiler thread could be started");
  }


  void DxvkPipelineCompiler::requestStop() {
    { std::lock_guard<std::mutex> lock(m_mutex);
      m_stop = true;

      // Whatever is still queued is dropped rather than compiled first.
      // Workers used to drain both lanes (up to 4096 entries each) before
      // they would exit, and the entries point at pipeline objects that
      // are destroyed while the compiler is still shutting down.
      m_liveQueue.clear();
      m_backgroundQueue.clear();
    }

    m_cond.notify_all();
  }


  void DxvkPipelineCompiler::joinAll() {
    // dxvk::thread::join() throws when the thread was already joined, so
    // only join the ones that still can be: stop() may run more than once.
    for (dxvk::thread& worker : m_workers) {
      if (worker.joinable())
        worker.join();
    }
  }


  std::deque<DxvkPipelineEntry>& DxvkPipelineCompiler::laneFor(DxvkPipelinePriority priority) {
    return priority == DxvkPipelinePriority::Background
      ? m_backgroundQueue
      : m_liveQueue;
  }


  bool DxvkPipelineCompiler::pushEntry(DxvkPipelineEntry&& entry) {
    std::deque<DxvkPipelineEntry>& queue = this->laneFor(entry.priority);

    std::lock_guard<std::mutex> lock(m_mutex);

    // Nothing is accepted once a stop was requested; the caller treats this
    // like a full queue and keeps using the fallback pipeline.
    if (m_stop)
      return false;

    switch (queue.size() >= DYASYNC_QUEUE_CAPACITY) {
      case true:  return false;
      case false: queue.push_back(std::move(entry)); return true;
    }

    return false;
  }


  void DxvkPipelineCompiler::notifyIfPushed(bool pushed) {
    switch (pushed) {
      case true:  m_cond.notify_one(); break;
      case false: break;
    }
  }


  bool DxvkPipelineCompiler::takeEntry(DxvkPipelineEntry& entry, bool preferBackground) {
    switch (select_lane(m_liveQueue.empty(), m_backgroundQueue.empty(), preferBackground)) {
      case TakeLane::Live:       entry = pop_front(m_liveQueue);       return true;
      case TakeLane::Background: entry = pop_front(m_backgroundQueue); return true;
      case TakeLane::None:       return false;
    }

    return false;
  }


  bool DxvkPipelineCompiler::waitForWork(DxvkPipelineEntry& entry) {
    uint32_t pick = m_pickCounter.fetch_add(1, std::memory_order_relaxed);

    std::unique_lock<std::mutex> lock(m_mutex);

    m_cond.wait(lock, [this] {
      return work_available(m_stop, m_liveQueue, m_backgroundQueue);
    });

    // Once a stop was requested nothing may start any more: the entries
    // reference pipelines that are about to be destroyed.
    if (m_stop)
      return false;

    return this->takeEntry(entry, prefer_background(pick));
  }


  void DxvkPipelineCompiler::compileAndCache(const DxvkPipelineEntry& entry) {
    switch (entry.pipeline->compilePipeline(entry.state, entry.renderPass)) {
      case true:  entry.pipeline->writePipelineStateToCache(entry.state, entry.renderPass->format()); break;
      case false: break;
    }
  }


  void DxvkPipelineCompiler::processEntry(const DxvkPipelineEntry& entry) {
    if (!entry_is_valid(entry))
      return;

    // Compiling can throw (shader module or pipeline layout creation is
    // allowed to fail with a DxvkError, allocations with std::bad_alloc).
    // Nothing above this function catches on a worker thread, so an
    // exception here would end in std::terminate and take the whole game
    // down with it. Losing one background compile is harmless: the draw
    // keeps using its fallback pipeline.
    try {
      this->compileAndCache(entry);
    } catch (const DxvkError& e) {
      Logger::err(str::format("DXVK: dyasync pipeline compile failed: ", e.message()));
    } catch (const std::exception& e) {
      Logger::err(str::format("DXVK: dyasync pipeline compile failed: ", e.what()));
    } catch (...) {
      Logger::err("DXVK: dyasync pipeline compile failed with an unknown exception");
    }
  }


  void DxvkPipelineCompiler::runCompilerThread() {
    env::setThreadName("dxvk-dyasync");

    for (DxvkPipelineEntry entry; this->waitForWork(entry); )
      this->processEntry(entry);
  }

}
