#pragma once

#include "Utils/Types.h"

namespace cassidy
{
  class Renderer;

  enum class TextureType : uint8_t
  {
    ALBEDO = 0,
    NORMAL = 1,
    METALLIC = 2,
    ROUGHNESS = 3,
    AO = 4,
    EMISSIVE = 5,
    SPECULAR = 6,
  };

  class Texture
  {
  public:
    cassidy::Texture* load(std::string filepath, VmaAllocator allocator, cassidy::Renderer* rendererRef,
      VkFormat format, VkBool32 shouldGenMipmaps = VK_FALSE);
    cassidy::Texture* create(unsigned char* data, size_t size, VkExtent2D textureDim, VmaAllocator allocator, 
      cassidy::Renderer* rendererRef, VkFormat format, VkBool32 shouldGenMipmaps = VK_FALSE);
    void release(VkDevice device, VmaAllocator allocator);
    void generateMipmaps(VkCommandBuffer cmd, VkFormat format, uint32_t width, uint32_t height, uint8_t mipLevels);

    // Getters/setters: ------------------------------------------------------------------------------------------
    inline VkImage      getImage()      { return m_image.image; }
    inline VkImageView  getImageView()  { return m_image.view; }
    inline LoadResult   getLoadResult() { return m_loadResult; }
    inline VkExtent2D   getDimensions() { return m_dimensions; }

  private:
    void transitionImageLayout(VkCommandBuffer cmd, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint8_t mipLevels);
    void copyBufferToImage(VkCommandBuffer cmd, VkBuffer stagingBuffer, uint32_t width, uint32_t height);

    AllocatedImage m_image;
    VkExtent2D m_dimensions;
    LoadResult m_loadResult = LoadResult::READY_TO_LOAD;
  };
}