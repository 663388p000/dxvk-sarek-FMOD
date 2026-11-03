#pragma once

#include <algorithm>
#include <map>
#include <vector>

#include "dxvk_include.h"

namespace dxvk {
  
  /**
   * \brief Vulkan extension mode
   * 
   * Defines whether an extension is
   * optional, required, or disabled.
   */
  enum class DxvkExtMode {
    Disabled,
    Optional,
    Required,
    Passive,
  };


  /**
   * \brief Vulkan extension info
   * 
   * Stores information for a single extension.
   * The renderer can use this information to
   * find out which extensions are enabled.
   */
  class DxvkExt {

  public:

    DxvkExt(
      const char*       pName,
            DxvkExtMode mode)
    : m_name(pName), m_mode(mode) { }

    /**
     * \brief Extension name
     * \returns Extension name
     */
    const char* name() const {
      return m_name;
    }

    /**
     * \brief Extension mode
     * \returns Extension mode
     */
    DxvkExtMode mode() const {
      return m_mode;
    }

    /**
     * \brief Checks whether the extension is enabled
     * 
     * If an extension is enabled, the features
     * provided by the extension can be used.
     * \returns \c true if the extension is enabled
     */
    operator bool () const {
      return m_revision != 0;
    }

    /**
     * \brief Supported revision
     * \returns Supported revision
     */
    uint32_t revision() const {
      return m_revision;
    }

    /**
     * \brief Changes extension mode
     * 
     * In some cases, it may be useful to change the
     * default mode dynamically after initialization.
     */
    void setMode(DxvkExtMode mode) {
      m_mode = mode;
    }

    /**
     * \brief Enables the extension
     */
    void enable(uint32_t revision) {
      m_revision = revision;
    }

    /**
     * \brief Disables the extension
     */
    void disable() {
      m_revision = 0;
    }

  private:

    const char* m_name     = nullptr;
    DxvkExtMode m_mode     = DxvkExtMode::Disabled;
    uint32_t    m_revision = 0;

  };


  /**
   * \brief Vulkan name list
   * 
   * A simple \c vector wrapper that can
   * be used to build a list of layer and
   * extension names.
   */
  class DxvkNameList {

  public:

    /**
     * \brief Adds a name
     * \param [in] pName The name
     */
    void add(const char* pName) {
      m_names.push_back(pName);
    }

    /**
     * \brief Number of names
     * \returns Name count
     */
    uint32_t count() const {
      return m_names.size();
    }

    /**
     * \brief Name list
     * \returns Name list
     */
    const char* const* names() const {
      return m_names.data();
    }

    /**
     * \brief Retrieves a single name
     * 
     * \param [in] index Name index
     * \returns The given name
     */
    const char* name(uint32_t index) const {
      return m_names.at(index);
    }

  private:

    std::vector<const char*> m_names;

  };


  /**
   * \brief Vulkan extension set
   * 
   * Stores a set of extensions or layers
   * supported by the Vulkan implementation.
   */
  class DxvkNameSet {

  public:

    DxvkNameSet();
    ~DxvkNameSet();

    /**
     * \brief Adds a name to the set
     * \param [in] pName Extension name
     */
    void add(
      const char*             pName);
    
    /**
     * \brief Merges two name sets
     * 
     * Adds all names from the given name set to
     * this name set, avoiding duplicate entries.
     * \param [in] names Name set to merge
     */
    void merge(
      const DxvkNameSet&      names);

    /**
     * \brief Checks whether an extension is supported
     * 
     * \param [in] pName Extension name
     * \returns Supported revision, or zero
     */
    uint32_t supports(
      const char*             pName) const;
    
    /**
     * \brief Enables requested extensions
     * 
     * Walks over a set of extensions and enables all
     * extensions that are supported and not disabled.
     * This also checks whether all required extensions
     * could be enabled, and returns \c false otherwise.
     * \param [in] numExtensions Number of extensions
     * \param [in] ppExtensions List of extensions
     * \param [out] nameSet Extension name set
     * \returns \c true on success
     */
    bool enableExtensions(
            uint32_t          numExtensions,
            DxvkExt**         ppExtensions,
            DxvkNameSet&      nameSet) const;
    
    /**
     * \brief Disables given extension
     *
     * Removes the given extension from the set
     * and sets its revision to 0 (i.e. disabled).
     * \param [in,out] ext Extension to disable
     */
    void disableExtension(
            DxvkExt&          ext);

    /**
     * \brief Creates name list from name set
     * 
     * Adds all names contained in the name set
     * to a name list, which can then be passed
     * to Vulkan functions.
     * \returns Name list
     */
    DxvkNameList toNameList() const;

    /**
     * \brief Enumerates instance layers
     * 
     * \param [in] vkl Vulkan library functions
     * \returns Set of available instance layers
     */
    static DxvkNameSet enumInstanceLayers(
      const Rc<vk::LibraryFn>&  vkl);
    
    /**
     * \brief Enumerates instance extensions
     * 
     * \param [in] vkl Vulkan library functions
     * \returns Set of available instance extensions
     */
    static DxvkNameSet enumInstanceExtensions(
      const Rc<vk::LibraryFn>&  vkl);
    
    /**
     * \brief Enumerates device extensions
     * 
     * \param [in] vki Vulkan instance functions
     * \param [in] device The device to query
     * \returns Set of available device extensions
     */
    static DxvkNameSet enumDeviceExtensions(
      const Rc<vk::InstanceFn>& vki,
            VkPhysicalDevice    device);

  private:

    std::map<std::string, uint32_t> m_names;

  };

  /**
   * \brief Device extensions
   * 
   * Lists all Vulkan extensions that are potentially
   * used by DXVK if supported by the implementation.
   */
  struct DxvkDeviceExtensions {
    DxvkExt amdMemoryOverallocationBehaviour  = { VK_AMD_MEMORY_OVERALLOCATION_BEHAVIOR_EXTENSION_NAME,     DxvkExtMode::Optional };
    DxvkExt amdShaderFragmentMask             = { VK_AMD_SHADER_FRAGMENT_MASK_EXTENSION_NAME,               DxvkExtMode::Optional };
    DxvkExt ext4444Formats                    = { VK_EXT_4444_FORMATS_EXTENSION_NAME,                       DxvkExtMode::Optional };
    DxvkExt extConservativeRasterization      = { VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME,         DxvkExtMode::Optional };
    DxvkExt extCustomBorderColor              = { VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME,                DxvkExtMode::Optional };
    DxvkExt extDepthClipEnable                = { VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME,                  DxvkExtMode::Optional };
    DxvkExt extDynamicRenderingUnusedAttachments = { VK_EXT_DYNAMIC_RENDERING_UNUSED_ATTACHMENTS_EXTENSION_NAME, DxvkExtMode::Optional };
    DxvkExt extExtendedDynamicState           = { VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME,             DxvkExtMode::Optional };
    DxvkExt extExtendedDynamicState2          = { VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME,           DxvkExtMode::Optional };
    DxvkExt extExtendedDynamicState3          = { VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME,           DxvkExtMode::Optional };
    DxvkExt extFullScreenExclusive            = { VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME,              DxvkExtMode::Optional };
    DxvkExt extGlobalPriority                 = { VK_EXT_GLOBAL_PRIORITY_EXTENSION_NAME,                    DxvkExtMode::Optional };
    DxvkExt extGlobalPriorityQuery            = { VK_EXT_GLOBAL_PRIORITY_QUERY_EXTENSION_NAME,               DxvkExtMode::Optional };
    DxvkExt extHostQueryReset                 = { VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME,                   DxvkExtMode::Optional };
    DxvkExt extImageCompressionControl        = { VK_EXT_IMAGE_COMPRESSION_CONTROL_EXTENSION_NAME,           DxvkExtMode::Optional };
    DxvkExt extImageCompressionControlSwapchain = { VK_EXT_IMAGE_COMPRESSION_CONTROL_SWAPCHAIN_EXTENSION_NAME, DxvkExtMode::Optional };
    DxvkExt extMemoryBudget                   = { VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,                      DxvkExtMode::Passive  };
    DxvkExt extMemoryPriority                 = { VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME,                    DxvkExtMode::Optional };
    DxvkExt extMultisampledRenderToSingleSampled = { VK_EXT_MULTISAMPLED_RENDER_TO_SINGLE_SAMPLED_EXTENSION_NAME, DxvkExtMode::Optional };
    DxvkExt extNonSeamlessCubeMap             = { VK_EXT_NON_SEAMLESS_CUBE_MAP_EXTENSION_NAME,              DxvkExtMode::Optional };
    DxvkExt extPipelineCreationCacheControl   = { VK_EXT_PIPELINE_CREATION_CACHE_CONTROL_EXTENSION_NAME,     DxvkExtMode::Optional };
    DxvkExt extPipelineCreationFeedback       = { VK_EXT_PIPELINE_CREATION_FEEDBACK_EXTENSION_NAME,          DxvkExtMode::Optional };
    DxvkExt extPipelineProtectedAccess        = { VK_EXT_PIPELINE_PROTECTED_ACCESS_EXTENSION_NAME,           DxvkExtMode::Optional };
    DxvkExt extPipelineRobustness             = { VK_EXT_PIPELINE_ROBUSTNESS_EXTENSION_NAME,                 DxvkExtMode::Optional };
    DxvkExt extQueueFamilyForeign             = { VK_EXT_QUEUE_FAMILY_FOREIGN_EXTENSION_NAME,                DxvkExtMode::Optional };
    DxvkExt extRasterizationOrderAttachmentAccess = { VK_EXT_RASTERIZATION_ORDER_ATTACHMENT_ACCESS_EXTENSION_NAME, DxvkExtMode::Optional };
    DxvkExt extRobustness2                    = { VK_EXT_ROBUSTNESS_2_EXTENSION_NAME,                       DxvkExtMode::Optional };
    DxvkExt extShaderDemoteToHelperInvocation = { VK_EXT_SHADER_DEMOTE_TO_HELPER_INVOCATION_EXTENSION_NAME, DxvkExtMode::Optional };
    DxvkExt extShaderStencilExport            = { VK_EXT_SHADER_STENCIL_EXPORT_EXTENSION_NAME,              DxvkExtMode::Optional };
    DxvkExt extShaderViewportIndexLayer       = { VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME,        DxvkExtMode::Optional };
    DxvkExt extTransformFeedback              = { VK_EXT_TRANSFORM_FEEDBACK_EXTENSION_NAME,                 DxvkExtMode::Optional };
    DxvkExt extVertexAttributeDivisor         = { VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME,           DxvkExtMode::Optional };
    DxvkExt extVertexInputDynamicState        = { VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME,          DxvkExtMode::Optional };
    DxvkExt khrBufferDeviceAddress            = { VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,              DxvkExtMode::Disabled };
    DxvkExt khrCreateRenderPass2              = { VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,                DxvkExtMode::Optional };
    DxvkExt khrDepthStencilResolve            = { VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,              DxvkExtMode::Optional };
    DxvkExt khrDrawIndirectCount              = { VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME,                DxvkExtMode::Optional };
    DxvkExt khrDriverProperties               = { VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME,                  DxvkExtMode::Optional };
    DxvkExt khrDynamicRendering               = { VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,                  DxvkExtMode::Optional };
    DxvkExt khrDynamicRenderingLocalRead      = { VK_KHR_DYNAMIC_RENDERING_LOCAL_READ_EXTENSION_NAME,        DxvkExtMode::Optional };
    DxvkExt khrExternalMemoryWin32            = { VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME,              DxvkExtMode::Optional };
    DxvkExt khrExternalSemaphoreWin32         = { VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME,           DxvkExtMode::Optional };
    // Linux/Android-native alternatives to the two above. VK_EXT_external_memory_dma_buf
    // depends on VK_KHR_external_memory_fd (it only adds a new handle-type
    // enum value, reusing _fd's functions/structs), so both are declared
    // together. See dxvk_image.cpp for where these actually get used -
    // whether they help at all depends on whether the Vulkan implementation
    // DXVK is actually talking to under Wine/Winlator (winevulkan, not the
    // native Mali driver directly - see the long comment in canShareImage())
    // advertises these to a Windows-target guest in the first place.
    DxvkExt khrExternalMemoryFd               = { VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,                  DxvkExtMode::Optional };
    DxvkExt extExternalMemoryDmaBuf           = { VK_EXT_EXTERNAL_MEMORY_DMA_BUF_EXTENSION_NAME,              DxvkExtMode::Optional };
    DxvkExt extExternalMemoryHost             = { VK_EXT_EXTERNAL_MEMORY_HOST_EXTENSION_NAME,                 DxvkExtMode::Optional };
    DxvkExt extExternalMemoryAcquireUnmodified = { VK_EXT_EXTERNAL_MEMORY_ACQUIRE_UNMODIFIED_EXTENSION_NAME,  DxvkExtMode::Optional };
    DxvkExt khrImageFormatList                = { VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME,                  DxvkExtMode::Optional };
    DxvkExt khrImagelessFramebuffer           = { VK_KHR_IMAGELESS_FRAMEBUFFER_EXTENSION_NAME,               DxvkExtMode::Optional };
    DxvkExt khrIncrementalPresent             = { VK_KHR_INCREMENTAL_PRESENT_EXTENSION_NAME,                 DxvkExtMode::Optional };
    DxvkExt khrMaintenance4                   = { VK_KHR_MAINTENANCE_4_EXTENSION_NAME,                       DxvkExtMode::Optional };
    DxvkExt khrMaintenance5                   = { VK_KHR_MAINTENANCE_5_EXTENSION_NAME,                       DxvkExtMode::Optional };
    DxvkExt khrMaintenance6                   = { VK_KHR_MAINTENANCE_6_EXTENSION_NAME,                       DxvkExtMode::Optional };
    DxvkExt khrMaintenance7                   = { VK_KHR_MAINTENANCE_7_EXTENSION_NAME,                       DxvkExtMode::Optional };
    DxvkExt khrPipelineBinary                 = { VK_KHR_PIPELINE_BINARY_EXTENSION_NAME,                     DxvkExtMode::Optional };
    DxvkExt khrPipelineLibrary                = { VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,                    DxvkExtMode::Optional };
    DxvkExt khrSamplerMirrorClampToEdge       = { VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME,       DxvkExtMode::Optional };
    DxvkExt khrShaderFloatControls            = { VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,              DxvkExtMode::Optional };
    DxvkExt khrSwapchain                      = { VK_KHR_SWAPCHAIN_EXTENSION_NAME,                          DxvkExtMode::Required };
    DxvkExt khrSwapchainMutableFormat         = { VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME,            DxvkExtMode::Optional };
    DxvkExt khrSynchronization2               = { VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,                   DxvkExtMode::Optional };
    DxvkExt khrTimelineSemaphore              = { VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,                 DxvkExtMode::Optional };
    DxvkExt khrZeroInitializeWorkgroupMemory  = { VK_KHR_ZERO_INITIALIZE_WORKGROUP_MEMORY_EXTENSION_NAME,    DxvkExtMode::Optional };
    DxvkExt nvxBinaryImport                   = { VK_NVX_BINARY_IMPORT_EXTENSION_NAME,                      DxvkExtMode::Disabled };
    DxvkExt nvxImageViewHandle                = { VK_NVX_IMAGE_VIEW_HANDLE_EXTENSION_NAME,                  DxvkExtMode::Disabled };

    // NOTE: VK_KHR_maintenance1/2/3 are not listed here because this project's
    // Vulkan floor is already 1.1 (see dxvk_instance.cpp / dxvk_device_filter.cpp),
    // and all three were promoted to core in 1.1 - they are unconditionally
    // available and need no extension string or feature struct.
    //
    // VK_KHR_pipeline_library and VK_EXT_pipeline_creation_feedback and
    // VK_EXT_queue_family_foreign and VK_EXT_global_priority and
    // VK_KHR_swapchain_mutable_format and VK_KHR_incremental_present add no
    // VkPhysicalDeviceFeatures2-chainable struct of their own (they only add
    // create-info structs used at pipeline/queue/swapchain creation time), so
    // there is nothing to enable in createDevice() beyond the extension string.
  };
  
  /**
   * \brief Instance extensions
   * 
   * Lists all Vulkan extensions that are potentially
   * used by DXVK if supported by the implementation.
   */
  struct DxvkInstanceExtensions {
    DxvkExt extDebugUtils                   = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME,                      DxvkExtMode::Optional };
    DxvkExt khrGetSurfaceCapabilities2      = { VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,       DxvkExtMode::Optional };
    DxvkExt khrSurface                      = { VK_KHR_SURFACE_EXTENSION_NAME,                          DxvkExtMode::Required };
  };
  
}
