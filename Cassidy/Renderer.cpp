#include "Renderer.h"
#include "Engine.h"
#include "Helpers.h"
#include "Initialisers.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <set>
#include <iostream>

void cassidy::Renderer::init(cassidy::Engine* engine)
{
  m_engineRef = engine;

  initLogicalDevice();
  initSwapchain();
}

void cassidy::Renderer::draw()
{

}

void cassidy::Renderer::release()
{
  m_deletionQueue.execute();
  std::cout << "Renderer shut down!\n" << std::endl;
}

void cassidy::Renderer::initLogicalDevice()
{
  m_physicalDevice = cassidy::helper::pickPhysicalDevice(m_engineRef->GetInstance());
  if (m_physicalDevice == VK_NULL_HANDLE)
  {
    std::cout << "ERROR: Failed to find physical device!\n" << std::endl;
    return;
  }

  QueueFamilyIndices indices = cassidy::helper::findQueueFamilies(m_physicalDevice, m_engineRef->GetSurface());

  std::vector<VkDeviceQueueCreateInfo> queueInfos;
  std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies)
  {
    VkDeviceQueueCreateInfo queueInfo = cassidy::init::deviceQueueCreateInfo(queueFamily, 1, &queuePriority);
    queueInfos.push_back(queueInfo);
  }

  VkPhysicalDeviceFeatures deviceFeatures = {};
  deviceFeatures.samplerAnisotropy = VK_TRUE;

  VkDeviceCreateInfo deviceInfo = cassidy::init::deviceCreateInfo(queueInfos.data(), queueInfos.size(),
    &deviceFeatures, DEVICE_EXTENSIONS.size(), DEVICE_EXTENSIONS.data(), VALIDATION_LAYERS.size(),
    VALIDATION_LAYERS.data());

  vkCreateDevice(m_physicalDevice, &deviceInfo, nullptr, &m_device);

  vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
  vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);

  m_deletionQueue.addFunction([=]() {
    vkDestroyDevice(m_device, nullptr);
  });

  std::cout << "Created logical device!\n" << std::endl;
}

void cassidy::Renderer::initSwapchain()
{
  SwapchainSupportDetails details = cassidy::helper::querySwapchainSupport(m_physicalDevice, m_engineRef->GetSurface());

  // If desired format isn't available on the chosen physical device, default to the first available format:
  const VkSurfaceFormatKHR desiredFormat = { VK_FORMAT_B8G8R8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
  VkSurfaceFormatKHR surfaceFormat = cassidy::helper::isSwapchainSurfaceFormatSupported(details.formats.size(), details.formats.data(), desiredFormat) ?
    desiredFormat : details.formats[0];

  // If desired present mode isn't available on the chosen physical device, default to FIFO (guaranteed to be avaiable):
  const VkPresentModeKHR desiredMode = VK_PRESENT_MODE_MAILBOX_KHR;
  VkPresentModeKHR presentMode = cassidy::helper::isSwapchainPresentModeSupported(details.presentModes.size(), details.presentModes.data(), desiredMode) ?
    desiredMode : VK_PRESENT_MODE_FIFO_KHR;

  VkExtent2D extent = cassidy::helper::chooseSwapchainExtent(m_engineRef->GetWindow(), details.capabilities);

  QueueFamilyIndices indices = cassidy::helper::findQueueFamilies(m_physicalDevice, m_engineRef->GetSurface());
  VkSwapchainCreateInfoKHR swapchainInfo = cassidy::init::swapchainCreateInfo(details, indices, m_engineRef->GetSurface(),
    surfaceFormat, presentMode, extent, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

  vkCreateSwapchainKHR(m_device, &swapchainInfo, nullptr, &m_swapchain.swapchain);

  // Retrieve handles to swapchain images:
  uint32_t imageCount = 0;
  vkGetSwapchainImagesKHR(m_device, m_swapchain.swapchain, &imageCount, nullptr);
  m_swapchain.images.resize(imageCount);
  vkGetSwapchainImagesKHR(m_device, m_swapchain.swapchain, &imageCount, m_swapchain.images.data());

  m_swapchain.imageFormat = surfaceFormat.format;
  m_swapchain.extent = extent;

  // Create swapchain image views:
  m_swapchain.imageViews.resize(m_swapchain.images.size());

  for (uint32_t i = 0; i < m_swapchain.images.size(); ++i)
  {
    const VkImage& image = m_swapchain.images[i];
    const VkFormat& format = m_swapchain.imageFormat;

    VkImageViewCreateInfo imageViewInfo = cassidy::init::imageViewCreateInfo(image, format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    vkCreateImageView(m_device, &imageViewInfo, nullptr, &m_swapchain.imageViews[i]);
  }

  m_deletionQueue.addFunction([=]() {
    m_swapchain.release(m_device);
  });

  std::cout << "Created swapchain!\n" << std::endl;
}
