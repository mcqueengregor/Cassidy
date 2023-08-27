#include "Helpers.h"
#include <SDL.h>
#include <SDL_vulkan.h>

#include <map>
#include <vector>
#include <algorithm>
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

// Find queue families that exist on the given physical device:
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

SwapchainSupportDetails cassidy::helper::querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
  SwapchainSupportDetails details;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

  if (formatCount > 0)
  {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
  
  if (presentModeCount > 0)
  {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
  }

  return details;
}

bool cassidy::helper::isSwapchainPresentModeSupported(uint32_t numAvailableModes, VkPresentModeKHR* availableModes, VkPresentModeKHR desiredMode)
{
  for (uint32_t i = 0; i < numAvailableModes; ++i)
  {
    if (availableModes[i] == desiredMode)
      return true;
  }
  return false;
}

bool cassidy::helper::isSwapchainSurfaceFormatSupported(uint32_t numAvailableFormats, VkSurfaceFormatKHR* availableFormats, VkSurfaceFormatKHR desiredFormat)
{
  for (uint32_t i = 0; i < numAvailableFormats; ++i)
  {
    if (availableFormats[i].format == desiredFormat.format && availableFormats[i].colorSpace == desiredFormat.colorSpace)
      return true;
  }
  return false;
}

VkExtent2D cassidy::helper::chooseSwapchainExtent(SDL_Window* window, VkSurfaceCapabilitiesKHR capabilities)
{
  // Account for window managers that set width and height to the max value of uint32_t (~4 billion):
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    return capabilities.currentExtent;

  int width;
  int height;

  SDL_Vulkan_GetDrawableSize(window, &width, &height);

  VkExtent2D actualExtent = {
    static_cast<uint32_t>(width),
    static_cast<uint32_t>(height)
  };

  actualExtent.width = std::clamp(actualExtent.width, 
    capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
  actualExtent.height = std::clamp(actualExtent.height,
    capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

  return actualExtent;
}

