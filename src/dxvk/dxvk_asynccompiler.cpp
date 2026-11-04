#include <algorithm>
#include <utility>

#include "dxvk_device.h"
#include "dxvk_graphics.h"
#include "dxvk_asynccompiler.h"

namespace dxvk {

  DxvkAsyncCompiler::DxvkAsyncCompiler(const DxvkDevice* device) {
    std::string useAllCores = env::getEnvVar("DXVK_ALL_CORES");
    uint32_t numCpuCores = dxvk::thread::hardware_concurrency();
    uint32_t numWorkers = ((std::max(1u, numCpuCores) - 1) * 5) / 7;

    if (numWorkers <  1) numWorkers =  1;
    if (numWorkers > 32) numWorkers = 32;

    // Reduce worker count on 32-bit to save address space
    if (env::is32BitHostPlatform())
      numWorkers = std::min(numWorkers, 16u);

    if (device->config().numShaderCompilerThreads > 0)
      numWorkers = device->config().numShaderCompilerThreads;

    if (useAllCores == "1")
      numWorkers = numCpuCores;

    Logger::info(str::format("DXVK: Using ", numWorkers, " async compiler threads"));

    m_compilerThreads.reserve(numWorkers);

    for (uint32_t i = 0; i < numWorkers; i++) {
      // Creating a thread throws when the process is out of address space
      // or threads. This is the constructor, so letting it escape would
      // destroy m_compilerThreads with live threads in it, which
      // std::terminate answers. Run with the threads that could be started.
      try {
        m_compilerThreads.emplace_back([this] { this->runCompilerThread(); });
      } catch (const DxvkError& e) {
        Logger::err(str::format("DXVK: Could not start async compiler thread ",
          i + 1, " of ", numWorkers, ": ", e.message()));
        break;
      }
    }

    if (m_compilerThreads.empty())
      Logger::err("DXVK: No async compiler thread could be started");
  }


  DxvkAsyncCompiler::~DxvkAsyncCompiler() {
    this->stop();
  }


  void DxvkAsyncCompiler::stop() {
    {
      std::lock_guard<std::mutex> lock(m_compilerLock);
      m_compilerStop.store(true);

      // Drop whatever is still queued instead of compiling it first. The
      // entries hold raw pointers to pipeline objects, which are destroyed
      // while this compiler is still being shut down.
      std::queue<PipelineEntry>().swap(m_compilerQueue);
    }

    m_compilerCond.notify_all();

    // dxvk::thread::join() throws when the thread was already joined, so
    // only join the ones that still can be: stop() may run more than once.
    for (auto& thread : m_compilerThreads) {
      if (thread.joinable())
        thread.join();
    }
  }


  void DxvkAsyncCompiler::queueCompilation(
    DxvkGraphicsPipeline*                pipeline,
    const DxvkGraphicsPipelineStateInfo& state,
    const DxvkRenderPass*                renderPass) {
    {
      std::lock_guard<std::mutex> lock(m_compilerLock);

      // Nothing is accepted once a stop was requested.
      if (m_compilerStop.load())
        return;

      m_compilerQueue.emplace(PipelineEntry{ pipeline, state, renderPass });
    }

    m_compilerCond.notify_one();
  }


  void DxvkAsyncCompiler::runCompilerThread() {
    env::setThreadName("dxvk-pcompiler");

    while (true) {
      PipelineEntry entry;

      {
        std::unique_lock<std::mutex> lock(m_compilerLock);

        m_compilerCond.wait(lock, [this] {
          return m_compilerStop.load() || !m_compilerQueue.empty();
        });

        // A stop request ends the loop immediately, queued work or not.
        // stop() already emptied the queue, and anything compiled after it
        // could touch pipelines that are being destroyed.
        if (m_compilerStop.load())
          break;

        // Take ONE entry. This used to move the whole backlog into a
        // local vector, so a single worker compiled everything serially
        // while every other worker found an empty queue and sat idle.
        entry = std::move(m_compilerQueue.front());
        m_compilerQueue.pop();
      }

      if (!entry.pipeline || !entry.renderPass)
        continue;

      // Compiling can throw (shader module or pipeline layout creation is
      // allowed to fail with a DxvkError, allocations with std::bad_alloc),
      // and nothing above this loop catches on a worker thread: an
      // exception here would end in std::terminate and take the whole game
      // down with it. One lost compile only means that draw stays skipped.
      try {
        if (entry.pipeline->compilePipeline(entry.state, entry.renderPass))
          entry.pipeline->writePipelineStateToCache(entry.state, entry.renderPass->format());
      } catch (const DxvkError& e) {
        Logger::err(str::format("DXVK: async pipeline compile failed: ", e.message()));
      } catch (const std::exception& e) {
        Logger::err(str::format("DXVK: async pipeline compile failed: ", e.what()));
      } catch (...) {
        Logger::err("DXVK: async pipeline compile failed with an unknown exception");
      }
    }
  }

}
