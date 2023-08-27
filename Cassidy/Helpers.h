#pragma once

#include "Types.h"

// Forward declarations:
class SDL_Window;

namespace cassidy::helper
{
  int32_t ratePhysicalDevice(VkPhysicalDevice device);

  VkPhysicalDevice pickPhysicalDevice(VkInstance instance);

  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

  SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

  bool isSwapchainPresentModeSupported(uint32_t numAvailableModes, VkPresentModeKHR* availableModes, VkPresentModeKHR desiredMode);
  bool isSwapchainSurfaceFormatSupported(uint32_t numAvailableFormats, VkSurfaceFormatKHR* availableFormats, VkSurfaceFormatKHR desiredFormat);
  VkExtent2D chooseSwapchainExtent(SDL_Window* window, VkSurfaceCapabilitiesKHR capabilities);
}