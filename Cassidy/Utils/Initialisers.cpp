#include "Initialisers.h"

VkApplicationInfo cassidy::init::applicationInfo(const char* appName, uint8_t engineVersionVariant, uint8_t engineVersionMaj,
  uint8_t engineVersionMin, uint8_t engineVersionPatch, uint32_t apiVersion)
{
  VkApplicationInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  info.pApplicationName = appName;
  info.applicationVersion = VK_MAKE_API_VERSION(
    engineVersionVariant, engineVersionMaj, engineVersionMin, engineVersionPatch);
  info.pEngineName = "Cassidy";
  info.engineVersion = info.applicationVersion;
  info.apiVersion = apiVersion;

  return info;
}

VkInstanceCreateInfo cassidy::init::instanceCreateInfo(VkApplicationInfo* appInfo, uint32_t numExtensions,
  const char* const* extensionNames, uint32_t numLayers, const char* const* layerNames,
  VkDebugUtilsMessengerCreateInfoEXT* debugCreateInfo)
{
  VkInstanceCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  info.pApplicationInfo = appInfo;
  info.enabledExtensionCount = numExtensions;
  info.ppEnabledExtensionNames = extensionNames;

  // If debug messenger info is specified because validation layers are desired, attach messenger to instance:
  if (debugCreateInfo)
  {
    info.enabledLayerCount = numLayers;
    info.ppEnabledLayerNames = layerNames;
    info.pNext = debugCreateInfo;
  }
  else
  {
    info.enabledLayerCount = 0;
    info.pNext = nullptr;
  }

  return info;
}

VkDebugUtilsMessengerCreateInfoEXT cassidy::init::debugMessengerCreateInfo(VkDebugUtilsMessageSeverityFlagBitsEXT severityFlags,
  VkDebugUtilsMessageTypeFlagsEXT typeFlags, PFN_vkDebugUtilsMessengerCallbackEXT callbackFunc)
{
  VkDebugUtilsMessengerCreateInfoEXT info = {};
  info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  info.messageSeverity = severityFlags;
  info.messageType = typeFlags;
  info.pfnUserCallback = callbackFunc;
  info.pUserData = nullptr;

  return info;
}

VkDeviceQueueCreateInfo cassidy::init::deviceQueueCreateInfo(uint32_t queueFamilyIndex, uint32_t queueCount, 
  float* queuePriority)
{
  VkDeviceQueueCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  info.queueFamilyIndex = queueFamilyIndex;
  info.queueCount = queueCount;
  info.pQueuePriorities = queuePriority;

  return info;
}

VkDeviceCreateInfo cassidy::init::deviceCreateInfo(const VkDeviceQueueCreateInfo* queueCreateInfos, 
  uint32_t queueCreateInfoCount, const VkPhysicalDeviceFeatures* deviceFeatures, uint32_t extensionCount, const char* const* extensionNames,
  uint32_t layerCount, const char* const* layerNames)
{
  VkDeviceCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  info.pQueueCreateInfos = queueCreateInfos;
  info.queueCreateInfoCount = queueCreateInfoCount;
  info.pEnabledFeatures = deviceFeatures;
  info.enabledExtensionCount = extensionCount;
  info.ppEnabledExtensionNames = extensionNames;

  if (layerNames)
  {
    info.enabledLayerCount = layerCount;
    info.ppEnabledLayerNames = layerNames;
  }
  else
    info.enabledLayerCount = 0;

  return info;
}

VkSwapchainCreateInfoKHR cassidy::init::swapchainCreateInfo(SwapchainSupportDetails details, QueueFamilyIndices indices,
  VkSurfaceKHR surface, VkSurfaceFormatKHR surfaceFormat, VkPresentModeKHR presentMode, VkExtent2D extent,
  VkImageUsageFlags usageFlags)
{
  VkSwapchainCreateInfoKHR info = {};
  info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  info.surface = surface;

  // Hold one more image than necessary, so the renderer can write to another image while the driver is busy:
  uint32_t imageCount = details.capabilities.minImageCount + 1;

  if (details.capabilities.maxImageCount > 0 && imageCount > details.capabilities.maxImageCount)
    imageCount = details.capabilities.maxImageCount;

  info.minImageCount = imageCount;
  info.imageFormat = surfaceFormat.format;
  info.imageColorSpace = surfaceFormat.colorSpace;
  info.imageExtent = extent;
  info.imageArrayLayers = 1;
  info.imageUsage = usageFlags;

  info.preTransform = details.capabilities.currentTransform;
  info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  info.presentMode = presentMode;
  info.clipped = VK_TRUE;

  info.oldSwapchain = VK_NULL_HANDLE;

  if (indices.graphicsFamily != indices.presentFamily)
  {
    const uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    info.queueFamilyIndexCount = 2;
    info.pQueueFamilyIndices = queueFamilyIndices;
  }
  else
    info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

  return info;
}

VkImageViewCreateInfo cassidy::init::imageViewCreateInfo(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint8_t mipLevels)
{
  VkImageViewCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  info.image = image;
  info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  info.format = format;
  info.subresourceRange.aspectMask = aspectFlags;
  info.subresourceRange.baseMipLevel = 0;
  info.subresourceRange.levelCount = mipLevels;
  info.subresourceRange.baseArrayLayer = 0;
  info.subresourceRange.layerCount = 1;

  return info;
}
