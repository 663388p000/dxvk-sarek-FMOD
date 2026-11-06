#include "dxvk_device.h"

namespace dxvk {
  
  DxvkUnboundResources::DxvkUnboundResources(DxvkDevice* dev)
  : m_sampler       (createSampler(dev)),
    m_buffer        (createBuffer(dev)),
    m_bufferView    (createBufferView(dev, m_buffer)),
    m_image1D       (createImage(dev, VK_IMAGE_TYPE_1D, 1)),
    m_image2D       (createImage(dev, VK_IMAGE_TYPE_2D, 6)),
    m_image3D       (createImage(dev, VK_IMAGE_TYPE_3D, 1)),
    m_viewsSampled  (createImageViews(dev, VK_FORMAT_R32_SFLOAT)),
    m_viewsStorage  (createImageViews(dev, VK_FORMAT_R32_UINT)) {
    
  }
  
  
  DxvkUnboundResources::~DxvkUnboundResources() {
    
  }
  
  
  void DxvkUnboundResources::clearResources(DxvkDevice* dev) {
    const Rc<DxvkContext> ctx = dev->createContext();
    ctx->beginRecording(dev->createCommandList());
    
    this->clearBuffer(ctx, m_buffer);

    // The 1D dummy is optional, see createImage
    if (m_image1D != nullptr)
      this->clearImage(ctx, m_image1D);

    this->clearImage(ctx, m_image2D);
    this->clearImage(ctx, m_image3D);
    
    dev->submitCommandList(
      ctx->endRecording(),
      VK_NULL_HANDLE,
      VK_NULL_HANDLE);
  }
  
  
  Rc<DxvkSampler> DxvkUnboundResources::createSampler(DxvkDevice* dev) {
    DxvkSamplerCreateInfo info;
    info.minFilter      = VK_FILTER_LINEAR;
    info.magFilter      = VK_FILTER_LINEAR;
    info.mipmapMode     = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info.mipmapLodBias  = 0.0f;
    info.mipmapLodMin   = -256.0f;
    info.mipmapLodMax   =  256.0f;
    info.useAnisotropy  = VK_FALSE;
    info.maxAnisotropy  = 1.0f;
    info.addressModeU   = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeV   = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeW   = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.compareToDepth = VK_FALSE;
    info.compareOp      = VK_COMPARE_OP_NEVER;
    info.borderColor    = VkClearColorValue();
    info.usePixelCoord  = VK_FALSE;
    info.nonSeamless    = VK_FALSE;
    
    return dev->createSampler(info);
  }
  
  
  Rc<DxvkBuffer> DxvkUnboundResources::createBuffer(DxvkDevice* dev) {
    DxvkBufferCreateInfo info;
    info.size       = MaxUniformBufferSize;
    info.usage      = VK_BUFFER_USAGE_TRANSFER_DST_BIT
                    | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
                    | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
                    | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
                    | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
                    | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT
                    | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
    info.stages     = VK_PIPELINE_STAGE_TRANSFER_BIT
                    | dev->getShaderPipelineStages();
    info.access     = VK_ACCESS_UNIFORM_READ_BIT
                    | VK_ACCESS_SHADER_READ_BIT
                    | VK_ACCESS_SHADER_WRITE_BIT;

    // The transform feedback usage flag only exists if the extension
    // is enabled; passing it to vkCreateBuffer otherwise is invalid
    // usage and strict drivers reject the buffer, which would take
    // the whole device down with it.
    if (dev->extensions().extTransformFeedback)
      info.usage |= VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT;

    return dev->createBuffer(info,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  }
  
  
  Rc<DxvkBufferView> DxvkUnboundResources::createBufferView(
          DxvkDevice*     dev,
    const Rc<DxvkBuffer>& buffer) {
    DxvkBufferViewCreateInfo info;
    info.format      = VK_FORMAT_R32_UINT;
    info.rangeOffset = 0;
    info.rangeLength = buffer->info().size;
    
    return dev->createBufferView(buffer, info);
  }
  
  
  Rc<DxvkImage> DxvkUnboundResources::createImage(
          DxvkDevice*     dev,
          VkImageType     type,
          uint32_t        layers) {
    DxvkImageCreateInfo info;
    info.type        = type;
    info.format      = VK_FORMAT_R32_UINT;
    info.flags       = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
    info.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    info.extent      = { 1, 1, 1 };
    info.numLayers   = layers;
    info.mipLevels   = 1;
    info.usage       = VK_IMAGE_USAGE_TRANSFER_DST_BIT
                     | VK_IMAGE_USAGE_SAMPLED_BIT
                     | VK_IMAGE_USAGE_STORAGE_BIT;
    info.stages      = VK_PIPELINE_STAGE_TRANSFER_BIT
                     | dev->getShaderPipelineStages();
    info.access      = VK_ACCESS_SHADER_READ_BIT;
    info.layout      = VK_IMAGE_LAYOUT_GENERAL;
    info.tiling      = VK_IMAGE_TILING_OPTIMAL;
    
    if (type == VK_IMAGE_TYPE_2D)
      info.flags       |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    // A dummy image only stands in for a resource the game did not bind, so
    // a driver that is picky about one combination of flags and usage must
    // not be able to take the whole device down with it. Ask for everything
    // first, exactly as before, and fall back to simpler variants only if
    // the driver rejects that. The views on top of the image follow
    // whatever was granted, see createImageView.
    struct Variant {
      const char*         what;
      VkImageCreateFlags  dropFlags;
      VkImageUsageFlags   dropUsage;
    };

    static const Variant variants[] = {
      { "as requested",                              0,                                  0                          },
      { "without STORAGE usage",                     0,                                  VK_IMAGE_USAGE_STORAGE_BIT },
      { "without MUTABLE_FORMAT",                    VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT, 0                          },
      { "without MUTABLE_FORMAT and STORAGE usage",  VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT, VK_IMAGE_USAGE_STORAGE_BIT },
    };

    const char* typeName = type == VK_IMAGE_TYPE_1D ? "1D"
                         : type == VK_IMAGE_TYPE_2D ? "2D" : "3D";

    // The views use an all-zero swizzle, so the size of a dummy image is
    // never visible to a shader apart from textureSize(). 1x1 is what has
    // always been used and is tried first. Some drivers take a 1-texel
    // image for a degenerate case of their own layout rules (pitch or tile
    // alignment) and reject it with an error that has nothing to do with
    // the image, so each variant is tried once more with a larger extent.
    static const uint32_t extents[] = { 1, 16 };

    std::string firstFailure;

    for (uint32_t extent : extents) {
      for (const Variant& variant : variants) {
        DxvkImageCreateInfo attempt = info;
        attempt.flags &= ~variant.dropFlags;
        attempt.usage &= ~variant.dropUsage;
        attempt.extent = {
          extent,
          type == VK_IMAGE_TYPE_1D ? 1u : extent,
          type == VK_IMAGE_TYPE_3D ? extent : 1u };

        try {
          Rc<DxvkImage> image = dev->createImage(attempt,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

          if (!firstFailure.empty()) {
            Logger::warn(str::format(
              "DxvkUnboundResources: The driver rejected the ", typeName,
              " dummy image as requested, using it ", variant.what,
              extent > 1 ? str::format(" and ", extent, " texels wide") : std::string(),
              " instead"));
          }

          return image;
        } catch (const DxvkError& e) {
          if (firstFailure.empty())
            firstFailure = e.message();
        }
      }
    }

    // Games rarely touch 1D resources, and createImageViews points the 1D
    // slots at the 2D dummy if there is none. The other two are needed.
    if (type == VK_IMAGE_TYPE_1D) {
      Logger::warn(str::format(
        "DxvkUnboundResources: No variant of the 1D dummy image could be created, ",
        "unbound 1D slots will use the 2D dummy instead. First failure:\n",
        firstFailure));

      return nullptr;
    }

    throw DxvkError(std::move(firstFailure));
  }
  
  
  Rc<DxvkImageView> DxvkUnboundResources::createImageView(
          DxvkDevice*     dev,
    const Rc<DxvkImage>&  image,
          VkFormat        format,
          VkImageViewType type,
          uint32_t        layers) {
    // Normally the image has everything and this changes nothing. If the
    // driver only accepted a reduced variant (see createImage), a view must
    // not claim usage its image lacks, and may only use another format
    // if the image was created as mutable-format.
    const DxvkImageCreateInfo& imageInfo = image->info();

    DxvkImageViewCreateInfo info;
    info.type         = type;
    info.format       = (imageInfo.flags & VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT)
                      ? format : imageInfo.format;
    info.usage        = (VK_IMAGE_USAGE_SAMPLED_BIT
                       | VK_IMAGE_USAGE_STORAGE_BIT) & imageInfo.usage;
    info.aspect       = VK_IMAGE_ASPECT_COLOR_BIT;
    info.minLevel     = 0;
    info.numLevels    = 1;
    info.minLayer     = 0;
    info.numLayers    = layers;
    info.swizzle      = VkComponentMapping {
      VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO,
      VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO };
    
    return dev->createImageView(image, info);
  }
  
  
  DxvkUnboundResources::UnboundViews DxvkUnboundResources::createImageViews(DxvkDevice* dev, VkFormat format) {
    UnboundViews result;

    if (m_image1D != nullptr) {
      result.view1D    = createImageView(dev, m_image1D, format, VK_IMAGE_VIEW_TYPE_1D,         1);
      result.view1DArr = createImageView(dev, m_image1D, format, VK_IMAGE_VIEW_TYPE_1D_ARRAY,   1);
    }

    result.view2D      = createImageView(dev, m_image2D, format, VK_IMAGE_VIEW_TYPE_2D,         1);
    result.view2DArr   = createImageView(dev, m_image2D, format, VK_IMAGE_VIEW_TYPE_2D_ARRAY,   1);
    result.viewCube    = createImageView(dev, m_image2D, format, VK_IMAGE_VIEW_TYPE_CUBE,       6);

    // Cube array views are only valid if the imageCubeArray feature is
    // enabled. That is not the case for D3D11 feature levels below 10_1
    // on a driver that validates view creation, so do not take device
    // creation down with us here. getImageView falls back to the plain
    // cube view, which is all a shader without the capability can use.
    if (dev->features().core.features.imageCubeArray)
      result.viewCubeArr = createImageView(dev, m_image2D, format, VK_IMAGE_VIEW_TYPE_CUBE_ARRAY, 6);

    result.view3D      = createImageView(dev, m_image3D, format, VK_IMAGE_VIEW_TYPE_3D,         1);

    // No 1D dummy image, see createImage. The accessors never hand out
    // a null view, so give the 1D slots the 2D views, which at least
    // are valid objects that read as zero.
    if (m_image1D == nullptr) {
      result.view1D    = result.view2D;
      result.view1DArr = result.view2DArr;
    }

    return result;
  }


  const DxvkImageView* DxvkUnboundResources::getImageView(VkImageViewType type, bool sampled) const {
    auto views = sampled ? &m_viewsSampled : &m_viewsStorage;

    switch (type) {
      case VK_IMAGE_VIEW_TYPE_1D:         return views->view1D.ptr();
      case VK_IMAGE_VIEW_TYPE_1D_ARRAY:   return views->view1DArr.ptr();
      // When implicit samplers are unbound -- we assume 2D in the shader.
      case VK_IMAGE_VIEW_TYPE_MAX_ENUM:
      case VK_IMAGE_VIEW_TYPE_2D:         return views->view2D.ptr();
      case VK_IMAGE_VIEW_TYPE_2D_ARRAY:   return views->view2DArr.ptr();
      case VK_IMAGE_VIEW_TYPE_CUBE:       return views->viewCube.ptr();
      case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY: return views->viewCubeArr != nullptr
                                                 ? views->viewCubeArr.ptr()
                                                 : views->viewCube.ptr();
      case VK_IMAGE_VIEW_TYPE_3D:         return views->view3D.ptr();
      default:                            return nullptr;
    }
  }
  
  
  void DxvkUnboundResources::clearBuffer(
    const Rc<DxvkContext>&  ctx,
    const Rc<DxvkBuffer>&   buffer) {
    ctx->initBuffer(buffer);
  }
  
  
  void DxvkUnboundResources::clearImage(
    const Rc<DxvkContext>&  ctx,
    const Rc<DxvkImage>&    image) {
    ctx->initImage(image,
      VkImageSubresourceRange {
        VK_IMAGE_ASPECT_COLOR_BIT,
        0, image->info().mipLevels,
        0, image->info().numLayers },
      VK_IMAGE_LAYOUT_UNDEFINED);
  }
  
}
