#pragma once

#include "Utils/Types.h"

// Forward declarations:
struct SDL_Window;

namespace cassidy::helper
{
  int32_t ratePhysicalDevice(VkPhysicalDevice device);

  VkPhysicalDevice pickPhysicalDevice(VkInstance instance);

  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

  SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

  bool isSwapchainPresentModeSupported(uint32_t numAvailableModes, VkPresentModeKHR* availableModes, VkPresentModeKHR desiredMode);
  bool isSwapchainSurfaceFormatSupported(uint32_t numAvailableFormats, VkSurfaceFormatKHR* availableFormats, VkSurfaceFormatKHR desiredFormat);
  VkExtent2D chooseSwapchainExtent(SDL_Window* window, VkSurfaceCapabilitiesKHR capabilities);

  VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, uint32_t numFormats, VkFormat* formats, VkImageTiling tiling, VkFormatFeatureFlags features);

  size_t padUniformBufferSize(size_t originalSize, const VkPhysicalDeviceProperties& gpuProperties);

  VkSampler createTextureSampler(const VkDevice& device, const VkPhysicalDeviceProperties& physicalDeviceProperties, VkFilter filter, VkSamplerAddressMode wrapMode, uint8_t numMips, bool useAniso);
}