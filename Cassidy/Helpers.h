#pragma once

#include "Types.h"

namespace cassidy::helper
{
  int32_t ratePhysicalDevice(VkPhysicalDevice device);

  VkPhysicalDevice pickPhysicalDevice(VkInstance instance);

  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
}