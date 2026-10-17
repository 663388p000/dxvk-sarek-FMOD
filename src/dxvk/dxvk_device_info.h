#pragma once

#include "dxvk_include.h"

namespace dxvk {

  /**
   * \brief Device info
   * 
   * Stores core properties and a bunch of extension-specific
   * properties, if the respective extensions are available.
   * Structures for unsupported extensions will be undefined,
   * so before using them, check whether they are supported.
   */
  struct DxvkDeviceInfo {
    VkPhysicalDeviceProperties2                               core;
    VkPhysicalDeviceIDProperties                              coreDeviceId;
    VkPhysicalDeviceSubgroupProperties                        coreSubgroup;
    VkPhysicalDeviceConservativeRasterizationPropertiesEXT    extConservativeRasterization;
    VkPhysicalDeviceCustomBorderColorPropertiesEXT            extCustomBorderColor;
    VkPhysicalDeviceRobustness2PropertiesEXT                  extRobustness2;
    VkPhysicalDeviceTransformFeedbackPropertiesEXT            extTransformFeedback;
    VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT       extVertexAttributeDivisor;
    VkPhysicalDeviceDepthStencilResolvePropertiesKHR          khrDepthStencilResolve;
    VkPhysicalDeviceDriverPropertiesKHR                       khrDeviceDriverProperties;
    VkPhysicalDeviceFloatControlsPropertiesKHR                khrShaderFloatControls;
    VkPhysicalDeviceTimelineSemaphorePropertiesKHR            khrTimelineSemaphore;
  };


  /**
   * \brief Device features
   * 
   * Stores core features and extension-specific features.
   * If the respective extensions are not available, the
   * extended features will be marked as unsupported.
   */
  struct DxvkDeviceFeatures {
    VkPhysicalDeviceFeatures2                                 core;
    VkPhysicalDeviceShaderDrawParametersFeatures              shaderDrawParameters;
    VkPhysicalDevice4444FormatsFeaturesEXT                    ext4444Formats;
    VkPhysicalDeviceCustomBorderColorFeaturesEXT              extCustomBorderColor;
    VkPhysicalDeviceDepthClipEnableFeaturesEXT                extDepthClipEnable;
    VkPhysicalDeviceDynamicRenderingUnusedAttachmentsFeaturesEXT extDynamicRenderingUnusedAttachments;
    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT           extExtendedDynamicState;
    VkPhysicalDeviceExtendedDynamicState2FeaturesEXT          extExtendedDynamicState2;
    VkPhysicalDeviceExtendedDynamicState3FeaturesEXT          extExtendedDynamicState3;
    VkPhysicalDeviceGlobalPriorityQueryFeaturesEXT            extGlobalPriorityQuery;
    VkPhysicalDeviceHostQueryResetFeaturesEXT                 extHostQueryReset;
    VkPhysicalDeviceImageCompressionControlFeaturesEXT        extImageCompressionControl;
    VkPhysicalDeviceImageCompressionControlSwapchainFeaturesEXT extImageCompressionControlSwapchain;
    VkPhysicalDeviceMemoryPriorityFeaturesEXT                 extMemoryPriority;
    VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT extMultisampledRenderToSingleSampled;
    VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT             extNonSeamlessCubeMap;
    VkPhysicalDevicePipelineCreationCacheControlFeaturesEXT   extPipelineCreationCacheControl;
    VkPhysicalDevicePipelineProtectedAccessFeaturesEXT        extPipelineProtectedAccess;
    VkPhysicalDevicePipelineRobustnessFeaturesEXT             extPipelineRobustness;
    VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesEXT extRasterizationOrderAttachmentAccess;
    VkPhysicalDeviceRobustness2FeaturesEXT                    extRobustness2;
    VkPhysicalDeviceShaderDemoteToHelperInvocationFeaturesEXT extShaderDemoteToHelperInvocation;
    VkPhysicalDeviceTransformFeedbackFeaturesEXT              extTransformFeedback;
    VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT         extVertexAttributeDivisor;
    VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT        extVertexInputDynamicState;
    VkPhysicalDeviceBufferDeviceAddressFeaturesKHR            khrBufferDeviceAddress;
    VkPhysicalDeviceDynamicRenderingFeaturesKHR               khrDynamicRendering;
    VkPhysicalDeviceDynamicRenderingLocalReadFeaturesKHR      khrDynamicRenderingLocalRead;
    VkPhysicalDeviceImagelessFramebufferFeaturesKHR           khrImagelessFramebuffer;
    VkPhysicalDeviceMaintenance4FeaturesKHR                   khrMaintenance4;
    VkPhysicalDeviceMaintenance5FeaturesKHR                   khrMaintenance5;
    VkPhysicalDeviceMaintenance6FeaturesKHR                   khrMaintenance6;
    VkPhysicalDeviceMaintenance7FeaturesKHR                   khrMaintenance7;
    VkPhysicalDevicePipelineBinaryFeaturesKHR                 khrPipelineBinary;
    VkPhysicalDeviceSynchronization2FeaturesKHR               khrSynchronization2;
    VkPhysicalDeviceTimelineSemaphoreFeaturesKHR              khrTimelineSemaphore;
    VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeaturesKHR  khrZeroInitializeWorkgroupMemory;
  };

  // NOTE: a handful of the requested extensions add no VkPhysicalDeviceFeatures2-
  // chainable struct at all (VK_KHR_pipeline_library, VK_EXT_pipeline_creation_feedback,
  // VK_EXT_queue_family_foreign, VK_EXT_global_priority, VK_EXT_image_drm_format_modifier,
  // VK_KHR_swapchain_mutable_format, VK_KHR_incremental_present,
  // VK_EXT_external_memory_acquire_unmodified) - they only add structs used at
  // pipeline/queue/image/swapchain creation time or chained into barriers
  // (VkExternalMemoryAcquireUnmodifiedEXT), so there is nothing to add here for
  // them; see dxvk_extensions.h for the full list.

}
