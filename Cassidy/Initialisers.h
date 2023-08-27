#pragma once

#include <vulkan/vulkan.h>
#include "Types.h"

namespace cassidy::init
{
  // Application info:
  VkApplicationInfo applicationInfo(const char* appName, uint8_t engineVersionVariant, uint8_t engineVersionMaj,
    uint8_t engineVersionMin, uint8_t engineVersionPatch, uint32_t apiVersion);

  // Instance create info:
  VkInstanceCreateInfo instanceCreateInfo(VkApplicationInfo* appInfo, uint32_t numExtensions,
    const char* const* extensionNames, uint32_t numLayers = 0, const char* const* layerNames = nullptr,
    VkDebugUtilsMessengerCreateInfoEXT* debugCreateInfo = nullptr);

  // Debug messenger create info:
  VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo(VkDebugUtilsMessageSeverityFlagBitsEXT severityFlags,
    VkDebugUtilsMessageTypeFlagsEXT typeFlags, PFN_vkDebugUtilsMessengerCallbackEXT callbackFunc);

  // Logical device + queue create infos:
  VkDeviceQueueCreateInfo deviceQueueCreateInfo(uint32_t queueFamilyIndex, uint32_t queueCount, float* queuePriority);
  VkDeviceCreateInfo deviceCreateInfo(const VkDeviceQueueCreateInfo* queueCreateInfos, uint32_t queueCreateInfoCount,
    const VkPhysicalDeviceFeatures* deviceFeatures, uint32_t extensionCount, const char* const* extensionNames, 
    uint32_t layerCount = 0, const char* const* layerNames = nullptr);

  // Swapchain create info:
  VkSwapchainCreateInfoKHR swapchainCreateInfo(SwapchainSupportDetails details, QueueFamilyIndices indices, VkSurfaceKHR surface,
    VkSurfaceFormatKHR surfaceFormat, VkPresentModeKHR presentMode, VkExtent2D extent, VkImageUsageFlags usageFlags);

  // Image view create info:
  VkImageViewCreateInfo imageViewCreateInfo(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint8_t mipLevels);
}