#pragma once

#include "Utils/Types.h"
#include <functional>
#include <iostream>

#define VK_CHECK(x)													                    \
do																	                            \
{																	                              \
	VkResult err = x;										                      		\
	if (err)														                          \
	{																                              \
		std::cout << "DETECTED VULKAN ERROR: " << err << std::endl;	\
		abort();													                          \
	}																                              \
} while (0)															                        \

// Forward declarations:
struct SDL_Window;

namespace cassidy::helper
{
  int32_t ratePhysicalDevice(VkPhysicalDevice device);

  VkPhysicalDevice pickPhysicalDevice(VkInstance instance);

  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

  SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

  void queryAvailableExtensions(VkPhysicalDevice physicalDevice, const char* layerName, std::vector<VkExtensionProperties>& availableExtensions);

  bool isSwapchainPresentModeSupported(uint32_t numAvailableModes, VkPresentModeKHR* availableModes, VkPresentModeKHR desiredMode);
  bool isSwapchainSurfaceFormatSupported(uint32_t numAvailableFormats, VkSurfaceFormatKHR* availableFormats, VkSurfaceFormatKHR desiredFormat);
  VkExtent2D chooseSwapchainExtent(SDL_Window* window, VkSurfaceCapabilitiesKHR capabilities);

  VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, uint32_t numFormats, VkFormat* formats, VkImageTiling tiling, VkFormatFeatureFlags features);

  size_t padUniformBufferSize(size_t originalSize, const VkPhysicalDeviceProperties& gpuProperties);

  VkSampler createTextureSampler(const VkDevice& device, const VkPhysicalDeviceProperties& physicalDeviceProperties, VkFilter filter, VkSamplerAddressMode wrapMode, uint8_t numMips, bool useAniso);

  void immediateSubmit(VkDevice device, UploadContext& uploadContext, std::function<void(VkCommandBuffer cmd)>&& function);
  void transitionImageLayout(VkCommandBuffer cmd, VkImage image, VkFormat format,
    VkImageLayout oldLayout, VkImageLayout newLayout,
    VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, 
    VkPipelineStageFlags srcStageFlags, VkPipelineStageFlags dstStageFlags,
    uint8_t mipLevels);
}