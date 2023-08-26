#include "Helpers.h"
#include <map>
#include <vector>
#include <iostream>

// Assign a score to a physical device given its capabilities:
int32_t cassidy::helper::ratePhysicalDevice(VkPhysicalDevice device)
{
  int32_t score = 0;
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(device, &properties);
  VkPhysicalDeviceFeatures features;
  vkGetPhysicalDeviceFeatures(device, &features);

  score += 1000 * static_cast<int>(properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
  score += 100 * static_cast<int>(properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU);
  score += properties.limits.maxImageDimension2D;
  score += properties.limits.maxImageDimension3D;
  score += properties.limits.maxImageDimensionCube;
  score += properties.limits.maxComputeWorkGroupSize[0];

  score *= static_cast<int>(features.geometryShader);

  std::cout << properties.deviceName << " - " << score << " points" << std::endl;

  return score;
}

// Iterate through existing physical devices and return the highest-rated one, if it's good enough:
VkPhysicalDevice cassidy::helper::pickPhysicalDevice(VkInstance instance)
{
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
  
  if (deviceCount == 0)
  {
    std::cout << "ERROR: No GPUs with Vulkan support found!\n" << std::endl;
    return VK_NULL_HANDLE;
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

  std::multimap<int, VkPhysicalDevice> candidates;

  std::cout << "Available devices: \n";

  // Build list of possible physical devices:
  for (const VkPhysicalDevice& d : devices)
  {
    int32_t score = cassidy::helper::ratePhysicalDevice(d);
    candidates.insert(std::make_pair(score, d));
  }

  if (candidates.rbegin()->first > 0)
  {
    VkPhysicalDeviceProperties selectedDeviceProperties;
    vkGetPhysicalDeviceProperties(candidates.rbegin()->second, &selectedDeviceProperties);

    std::cout << "Selected physical device: " << selectedDeviceProperties.deviceName << std::endl;
    return candidates.rbegin()->second;
  }
  else
  {
    std::cout << "ERROR: Failed to find a suitable GPU out of " << deviceCount << " candidates!\n" << std::endl;
    return VK_NULL_HANDLE;
  }
}

// 
QueueFamilyIndices cassidy::helper::findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
{
  QueueFamilyIndices indices;

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
  
  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

  int32_t i = 0;
  for (const VkQueueFamilyProperties& qf : queueFamilies)
  {
    if (qf.queueFlags & VK_QUEUE_GRAPHICS_BIT)
      indices.graphicsFamily = i;

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

    if (presentSupport)
      indices.presentFamily = i;

    ++i;
  }

  return indices;
}

