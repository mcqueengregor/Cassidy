#include "Core/Texture.h"

#include <Utils/Initialisers.h>
#include <Core/Renderer.h>

#define STB_IMAGE_IMPLEMENTATION
#include <Vendor/stb/stb_image.h>

#include <iostream>

bool cassidy::Texture::load(std::string filepath, VmaAllocator allocator, cassidy::Renderer* rendererRef, 
  VkFormat format, VkBool32 shouldGenMipmaps)
{
  int texWidth, texHeight, numChannels;
  stbi_uc* data = stbi_load(filepath.c_str(), &texWidth, &texHeight, &numChannels, STBI_rgb_alpha);

  if (!data)
  {
    std::cout << "ERROR: Failed to load texture (" << filepath << ")!" << std::endl;
    return false;
  }

  VkDeviceSize textureSize = texWidth * texHeight * STBI_rgb_alpha;

  VkBufferCreateInfo stagingBufferInfo = cassidy::init::bufferCreateInfo(textureSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
  VmaAllocationCreateInfo bufferAllocInfo = cassidy::init::vmaAllocationCreateInfo(VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

  AllocatedBuffer stagingBuffer;

  vmaCreateBuffer(allocator, &stagingBufferInfo, &bufferAllocInfo,
    &stagingBuffer.buffer,
    &stagingBuffer.allocation,
    nullptr);

  void* mappingData;
  vmaMapMemory(allocator, stagingBuffer.allocation, &mappingData);
  memcpy(mappingData, data, static_cast<size_t>(textureSize));
  vmaUnmapMemory(allocator, stagingBuffer.allocation);

  stbi_image_free(data);

  VkExtent3D imageExtent = {
    static_cast<uint32_t>(texWidth),
    static_cast<uint32_t>(texHeight),
    1
  };

  const uint32_t mipLevels = shouldGenMipmaps == VK_TRUE ? 
    static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1 : 1;

  VkImageCreateInfo imageInfo = cassidy::init::imageCreateInfo(VK_IMAGE_TYPE_2D, imageExtent, mipLevels, format, 
    VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  VmaAllocationCreateInfo imageAllocInfo = {};
  imageAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

  vmaCreateImage(allocator, &imageInfo, &imageAllocInfo, &m_image.image, &m_image.allocation, nullptr);

  rendererRef->immediateSubmit([=](VkCommandBuffer cmd)
  {
    transitionImageLayout(cmd, format, VK_IMAGE_LAYOUT_UNDEFINED, 
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
    copyBufferToImage(cmd, stagingBuffer.buffer, texWidth, texHeight);

    if (shouldGenMipmaps == VK_TRUE)
      generateMipmaps(cmd, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels);
    else
      transitionImageLayout(cmd, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels);
  });

  VkImageViewCreateInfo viewInfo = cassidy::init::imageViewCreateInfo(m_image.image, VK_FORMAT_R8G8B8A8_SRGB,
    VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
  vkCreateImageView(rendererRef->getLogicalDevice(), &viewInfo, nullptr, &m_imageView);

  vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);

  return true;
}

void cassidy::Texture::release(VkDevice device, VmaAllocator allocator)
{
  vmaDestroyImage(allocator, m_image.image, m_image.allocation);
  vkDestroyImageView(device, m_imageView, nullptr);
}

void cassidy::Texture::transitionImageLayout(VkCommandBuffer cmd, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint8_t mipLevels)
{
  VkImageSubresourceRange range = {};
  range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  range.baseMipLevel = 0;
  range.levelCount = mipLevels;
  range.baseArrayLayer = 0;
  range.layerCount = 1;

  VkImageMemoryBarrier barrier = {};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = m_image.image;
  barrier.subresourceRange = range;

  VkPipelineStageFlags srcStage;
  VkPipelineStageFlags dstStage;

  // Setup barrier command for undefined layout (texture) -> transfer to GPU layout transition:
  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
  {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  // Setup barrier command for transfer to GPU layout -> GPU-side shader read-only layout transition:
  else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
  {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }

  vkCmdPipelineBarrier(cmd,
    srcStage, dstStage,
    0, 
    0, nullptr,
    0, nullptr,
    1, &barrier);
}

void cassidy::Texture::copyBufferToImage(VkCommandBuffer cmd, VkBuffer stagingBuffer, uint32_t width, uint32_t height)
{
  VkBufferImageCopy region = {};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;

  region.imageOffset = { 0, 0, 0 };
  region.imageExtent = { width, height, 1 };

  vkCmdCopyBufferToImage(cmd, stagingBuffer, m_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void cassidy::Texture::generateMipmaps(VkCommandBuffer cmd, VkFormat format, uint32_t width, uint32_t height, uint8_t mipLevels)
{

}
