#pragma once

#include <array>
#include <atomic>
#include <mutex>

#include "../util/sync/sync_list.h"

#include "dxvk_bind_mask.h"
#include "dxvk_constant_state.h"
#include "dxvk_graphics_state.h"
#include "dxvk_pipecache.h"
#include "dxvk_pipelayout.h"
#include "dxvk_renderpass.h"
#include "dxvk_resource.h"
#include "dxvk_shader.h"
#include "dxvk_stats.h"

namespace dxvk {

  class DxvkDevice;
  class DxvkPipelineManager;

  /**
   * \brief Flags that describe pipeline properties
   */
  enum class DxvkGraphicsPipelineFlag {
    HasTransformFeedback,
    HasStorageDescriptors,
  };

  using DxvkGraphicsPipelineFlags = Flags<DxvkGraphicsPipelineFlag>;


  /**
   * \brief Shaders used in graphics pipelines
   */
  struct DxvkGraphicsPipelineShaders {
    Rc<DxvkShader> vs;
    Rc<DxvkShader> tcs;
    Rc<DxvkShader> tes;
    Rc<DxvkShader> gs;
    Rc<DxvkShader> fs;

    bool eq(const DxvkGraphicsPipelineShaders& other) const {
      return vs == other.vs && tcs == other.tcs
          && tes == other.tes && gs == other.gs
          && fs == other.fs;
    }

    size_t hash() const {
      DxvkHashState state;
      state.add(DxvkShader::getHash(vs));
      state.add(DxvkShader::getHash(tcs));
      state.add(DxvkShader::getHash(tes));
      state.add(DxvkShader::getHash(gs));
      state.add(DxvkShader::getHash(fs));
      return state;
    }

    bool validate() const {
      return validateShaderType(vs, VK_SHADER_STAGE_VERTEX_BIT)
          && validateShaderType(tcs, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT)
          && validateShaderType(tes, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
          && validateShaderType(gs, VK_SHADER_STAGE_GEOMETRY_BIT)
          && validateShaderType(fs, VK_SHADER_STAGE_FRAGMENT_BIT);
    }

    static bool validateShaderType(const Rc<DxvkShader>& shader, VkShaderStageFlagBits stage) {
      return shader == nullptr || shader->info().stage == stage;
    }
  };


  /**
   * \brief Common graphics pipeline state
   *
   * Non-dynamic pipeline state that cannot
   * be changed dynamically.
   */
  struct DxvkGraphicsCommonPipelineStateInfo {
    bool                                msSampleShadingEnable;
    float                               msSampleShadingFactor;
  };


  /**
   * \brief Graphics pipeline instance
   *
   * Stores a state vector and the
   * corresponding pipeline handle.
   */
  class DxvkGraphicsPipelineInstance {

  public:

    DxvkGraphicsPipelineInstance()
    : m_stateVector (),
      m_renderPass  (nullptr),
      m_pipeline    (VK_NULL_HANDLE) { }

    DxvkGraphicsPipelineInstance(
      const DxvkGraphicsPipelineStateInfo&  state,
      const DxvkRenderPass*                 rp,
            VkPipeline                      pipe)
    : m_stateVector (state),
      m_renderPass  (rp),
      m_pipeline    (pipe) { }

    /**
     * \brief Checks for matching pipeline state
     *
     * \param [in] stateVector Graphics pipeline state
     * \param [in] renderPass Render pass handle
     * \returns \c true if the specialization is compatible
     */
    bool isCompatible(
      const DxvkGraphicsPipelineStateInfo&  state,
      const DxvkRenderPass*                 rp) {
      return m_renderPass  == rp
          && m_stateVector == state;
    }

    VkPipeline pipeline() const {
      return m_pipeline;
    }

    const DxvkRenderPass* renderPass() const {
      return m_renderPass;
    }

  private:

    DxvkGraphicsPipelineStateInfo m_stateVector;
    const DxvkRenderPass*         m_renderPass;
    VkPipeline                    m_pipeline;

  };


  /**
   * \brief Graphics pipeline
   *
   * Stores the pipeline layout as well as methods to
   * recompile the graphics pipeline against a given
   * pipeline state vector.
   */
  class DxvkGraphicsPipeline {

  public:

    DxvkGraphicsPipeline(
            DxvkPipelineManager*        pipeMgr,
            DxvkGraphicsPipelineShaders shaders);

    ~DxvkGraphicsPipeline();

    /**
     * \brief Shaders used by the pipeline
     * \returns Shaders used by the pipeline
     */
    const DxvkGraphicsPipelineShaders& shaders() const {
      return m_shaders;
    }

    /**
     * \brief Returns graphics pipeline flags
     * \returns Graphics pipeline property flags
     */
    DxvkGraphicsPipelineFlags flags() const {
      return m_flags;
    }

    /**
     * \brief Pipeline layout
     *
     * Stores the pipeline layout and the descriptor set
     * layout, as well as information on the resource
     * slots used by the pipeline.
     * \returns Pipeline layout
     */
    DxvkPipelineLayout* layout() const {
      return m_layout.ptr();
    }

    /**
     * \brief Queries shader for a given stage
     *
     * In case no shader is specified for the
     * given stage, \c nullptr will be returned.
     * \param [in] stage The shader stage
     * \returns Shader of the given stage
     */
    Rc<DxvkShader> getShader(
            VkShaderStageFlagBits             stage) const;

    VkPipeline getPipelineHandle(
      const DxvkGraphicsPipelineStateInfo&    state,
      const DxvkRenderPass*                   renderPass);

    /**
     * \brief Compiles a pipeline
     *
     * Asynchronously compiles the given pipeline
     * and stores the result for future use.
     * \param [in] state Pipeline state vector
     * \param [in] renderPass The render pass
     * \returns \c true if compile succeeded
     */
    bool compilePipeline(
      const DxvkGraphicsPipelineStateInfo&    state,
      const DxvkRenderPass*                   renderPass);

    void writePipelineStateToCache(
      const DxvkGraphicsPipelineStateInfo& state,
      const DxvkRenderPassFormat&          format) const;

  private:

    Rc<vk::DeviceFn>            m_vkd;
    DxvkPipelineManager*        m_pipeMgr;

    DxvkGraphicsPipelineShaders m_shaders;
    DxvkDescriptorSlotMapping   m_slotMapping;

    Rc<DxvkPipelineLayout>      m_layout;

    uint32_t m_vsIn  = 0;
    uint32_t m_fsOut = 0;

    DxvkGraphicsPipelineFlags           m_flags;
    DxvkGraphicsCommonPipelineStateInfo m_common;

    struct LockFreeMapEntry {
      std::atomic<size_t>                        hash     { 0 };
      std::atomic<DxvkGraphicsPipelineInstance*> instance { nullptr };
    };

    struct FallbackMapEntry {
      std::atomic<uintptr_t>  key      { 0 };
      std::atomic<VkPipeline> pipeline { VK_NULL_HANDLE };
      std::atomic<uint8_t>    used     { 0 };
    };

    // Sizes of the lookup tables below. EVERY DxvkGraphicsPipeline carries
    // its own copy of all of them, and an application creates one pipeline
    // object per shader combination - thousands of them in a game like
    // Far Cry 2 - so they must stay small. They used to be 4096 / 8192 /
    // 2048 / 4096 entries, which made each pipeline object ~160 KiB in a
    // 32-bit build (~272 KiB in a 64-bit one) before it held a single state
    // variant. A few thousand pipelines were enough to use several hundred
    // MiB of a 32-bit address space, which is a plausible cause of the
    // "memory climbs, then the game suddenly dies" behaviour.
    //
    // A pipeline normally has a handful of state variants. The ones that do
    // not fit into the tables are still found by walking m_pipelines (see
    // m_instanceMapSaturated), so nothing depends on these sizes being
    // large enough; they only decide how often the slower walk is needed.
    // All sizes must be powers of two.
    static constexpr uint32_t InstanceMapSize    = 128;
    static constexpr uint32_t InstanceMapMask    = InstanceMapSize - 1;
    static constexpr uint32_t MaxProbeDistance   = 32;
    static constexpr uint32_t OverflowMapSize    = 128;
    static constexpr uint32_t OverflowMapMask    = OverflowMapSize - 1;
    static constexpr uint32_t OverflowProbeMax   = 32;
    static constexpr uint32_t FallbackMapSize    = 64;
    static constexpr uint32_t FallbackMapMask    = FallbackMapSize - 1;
    static constexpr uint32_t FallbackProbeMax   = 16;

    static constexpr uint32_t QueuedSetSize = 128;
    static constexpr uint32_t QueuedSetMask = QueuedSetSize - 1;

    static_assert((InstanceMapSize & InstanceMapMask) == 0, "InstanceMapSize must be a power of two");
    static_assert((OverflowMapSize & OverflowMapMask) == 0, "OverflowMapSize must be a power of two");
    static_assert((FallbackMapSize & FallbackMapMask) == 0, "FallbackMapSize must be a power of two");
    static_assert((QueuedSetSize   & QueuedSetMask)   == 0, "QueuedSetSize must be a power of two");
    static_assert(MaxProbeDistance  <= InstanceMapSize, "probe distance exceeds the instance map");
    static_assert(OverflowProbeMax  <= OverflowMapSize, "probe distance exceeds the overflow map");
    static_assert(FallbackProbeMax  <= FallbackMapSize, "probe distance exceeds the fallback map");

    sync::List<DxvkGraphicsPipelineInstance>  m_pipelines;

    std::array<std::atomic<size_t>, QueuedSetSize> m_queuedSet;

    std::array<LockFreeMapEntry,  InstanceMapSize>   m_instanceMap;
    std::atomic<bool>                                m_instanceMapOverflow { false };

    // Set once an instance could not be placed in either table. From then
    // on a miss in the tables is not authoritative and the lookup also
    // walks m_pipelines, which always contains every instance.
    std::atomic<bool>                                m_instanceMapSaturated { false };

    std::array<LockFreeMapEntry,  OverflowMapSize>   m_overflowMap;
    std::array<FallbackMapEntry,  FallbackMapSize>   m_fallbackMap;
    std::atomic<VkPipeline>                          m_basePipeline { VK_NULL_HANDLE };
    std::atomic<bool>                                m_hasBasePipeline { false };

    static size_t computeInstanceHash(
      const DxvkGraphicsPipelineStateInfo& state,
      const DxvkRenderPass*                renderPass);

    DxvkGraphicsPipelineInstance* findInstanceLockFree(
      const DxvkGraphicsPipelineStateInfo& state,
      const DxvkRenderPass*                renderPass);

    void insertInstanceToMap(
      const DxvkGraphicsPipelineStateInfo& state,
      const DxvkRenderPass*                renderPass,
      DxvkGraphicsPipelineInstance*         inst);

    DxvkGraphicsPipelineInstance* createInstance(
      const DxvkGraphicsPipelineStateInfo& state,
      const DxvkRenderPass*                renderPass);

    DxvkGraphicsPipelineInstance* findInstance(
      const DxvkGraphicsPipelineStateInfo& state,
      const DxvkRenderPass*                renderPass);

    static uintptr_t computeFallbackKey(
      const DxvkRenderPass*                renderPass);

    VkPipeline findFallback(
      const DxvkRenderPass*                renderPass);

    /**
     * \brief Clears the "already queued" marks of a state vector
     *
     * \param [in] hash Hash from computeInstanceHash
     */
    void releaseQueuedSlots(
            size_t                         hash);

    VkPipeline createPipeline(
      const DxvkGraphicsPipelineStateInfo& state,
      const DxvkRenderPass*                renderPass) const;

    void destroyPipeline(
            VkPipeline                     pipeline) const;

    DxvkShaderModule createShaderModule(
      const Rc<DxvkShader>&                shader,
      const DxvkGraphicsPipelineStateInfo& state) const;

    Rc<DxvkShader> getPrevStageShader(
            VkShaderStageFlagBits          stage) const;

    bool validatePipelineState(
      const DxvkGraphicsPipelineStateInfo& state,
            bool                           trusted) const;

    void logPipelineState(
            LogLevel                       level,
      const DxvkGraphicsPipelineStateInfo& state) const;

  };

}
