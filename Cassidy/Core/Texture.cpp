#include "Core/Texture.h"

#include <Utils/Initialisers.h>
#include <Utils/Helpers.h>
#include <Core/Renderer.h>

#define STB_IMAGE_IMPLEMENTATION
#include <Vendor/stb/stb_image.h>

#include <iostream>

cassidy::Texture* cassidy::Texture::load(std::string filepath, VmaAllocator allocator, cassidy::Renderer* rendererRef, 
  VkFormat format, VkBool32 shouldGenMipmaps)
{
  int texWidth, texHeight, numChannels;

  int requiredComponents = 0;

  if (format == VK_FORMAT_R8_UNORM
    || format == VK_FORMAT_R8_SRGB)
  {
    requiredComponents = STBI_grey;
  }
  else if (format == VK_FORMAT_R8G8B8A8_SRGB
    || format == VK_FORMAT_R8G8B8A8_UNORM)
  {
    requiredComponents = STBI_rgb_alpha;
  }

  stbi_uc* data = stbi_load(filepath.c_str(), &texWidth, &texHeight, &numChannels, requiredComponents);

  if (!data) return nullptr;

  VkDeviceSize textureSize = texWidth * texHeight * requiredComponents;

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

  // Check support for linear filtering necessary for generating mipmaps, skip mipmap generation if it isn't:
  VkFormatProperties formatProperties;
  vkGetPhysicalDeviceFormatProperties(rendererRef->getPhysicalDevice(), format, &formatProperties);

  if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
    shouldGenMipmaps = VK_FALSE;

  const uint32_t mipLevels = shouldGenMipmaps == VK_TRUE ? 
    static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1 : 1;

  // If generating mipmaps for this texture, add TRANSFER_SRC to usage flags:
  VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  if (shouldGenMipmaps == VK_TRUE) usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

  VkImageCreateInfo imageInfo = cassidy::init::imageCreateInfo(VK_IMAGE_TYPE_2D, imageExtent, mipLevels,
    format, VK_IMAGE_TILING_OPTIMAL, usage);

  VmaAllocationCreateInfo imageAllocInfo = {};
  imageAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

  vmaCreateImage(allocator, &imageInfo, &imageAllocInfo, &m_image.image, &m_image.allocation, nullptr);

  cassidy::helper::immediateSubmit(rendererRef->getLogicalDevice(),
    rendererRef->getUploadContext(), [=](VkCommandBuffer cmd)
  {
    cassidy::helper::transitionImageLayout(cmd, m_image.image, format, 
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
      0, VK_ACCESS_TRANSFER_WRITE_BIT,
      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
      mipLevels);

    copyBufferToImage(cmd, stagingBuffer.buffer, texWidth, texHeight);

    if (shouldGenMipmaps == VK_TRUE)
      generateMipmaps(cmd, format, texWidth, texHeight, mipLevels);
    else
      cassidy::helper::transitionImageLayout(cmd, m_image.image, format,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        mipLevels);
    });

  VkImageViewCreateInfo viewInfo = cassidy::init::imageViewCreateInfo(m_image.image, format,
    VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
  vkCreateImageView(rendererRef->getLogicalDevice(), &viewInfo, nullptr, &m_image.view);

  vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);

  return this;
}

void cassidy::Texture::release(VkDevice device, VmaAllocator allocator)
{
  vmaDestroyImage(allocator, m_image.image, m_image.allocation);
  vkDestroyImageView(device, m_image.view, nullptr);
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
  VkImageSubresourceRange range = {
    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    .levelCount = 1,
    .baseArrayLayer = 0,
    .layerCount = 1,
  };

  VkImageMemoryBarrier barrier = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .image = m_image.image,
    .subresourceRange = range,
  };

  int32_t mipWidth = width;
  int32_t mipHeight = height;

  // Blit lower mip level into level above it at progressively halving resolutions:
  for (uint8_t i = 1; i < mipLevels; ++i)
  {
    // Transition layout of current mip level to TRANSFER_SRC for blit:
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(cmd,
      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
      0,
      0, nullptr,
      0, nullptr,
      1, &barrier);

    const VkImageSubresourceLayers srcLayers = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .mipLevel = static_cast<uint32_t>(i - 1),
      .baseArrayLayer = 0,
      .layerCount = 1,
    };
    const VkImageSubresourceLayers dstLayers = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .mipLevel = i,
      .baseArrayLayer = 0,
      .layerCount = 1,
    };

    VkImageBlit blit = {
      .srcSubresource = srcLayers,
      .srcOffsets = { { 0, 0, 0, }, { mipWidth, mipHeight, 1} },
      .dstSubresource = dstLayers,
      .dstOffsets = { { 0, 0, 0 }, { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 } },
    };

    vkCmdBlitImage(cmd,
      m_image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      m_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1, &blit, VK_FILTER_LINEAR);

    // Transition layout of current mip level to SHADER_READ_ONLY_OPTIMAL:
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd, 
      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
      0,
      0, nullptr,
      0, nullptr,
      1, &barrier);

    if (mipWidth > 1) mipWidth /= 2;
    if (mipHeight > 1) mipHeight /= 2;
  }

  // Transition final mip level:
  barrier.subresourceRange.baseMipLevel = mipLevels - 1;
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(cmd,
    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    0,
    0, nullptr,
    0, nullptr,
    1, &barrier);
}
