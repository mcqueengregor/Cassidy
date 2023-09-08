#include "Renderer.h"
#include "Core/Engine.h"
#include "Utils/Helpers.h"
#include "Utils/Initialisers.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#define VMA_IMPLEMENTATION
#include "Vendor/vma/vk_mem_alloc.h"

#include <set>
#include <iostream>

void cassidy::Renderer::init(cassidy::Engine* engine)
{
  m_engineRef = engine;

  initLogicalDevice();
  initMemoryAllocator();
  initSwapchain();
  initPipelines();
  initSwapchainFramebuffers();  // (swapchain framebuffers are dependent on back buffer pipeline's render pass)
  initCommandPool();
  initCommandBuffers();
  initSyncObjects();

  m_currentFrameIndex = 0;
}

void cassidy::Renderer::draw()
{
  vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrameIndex], VK_TRUE, UINT64_MAX);

  uint32_t swapchainImageIndex;
  {
    const VkResult acquireImageResult = vkAcquireNextImageKHR(m_device, m_swapchain.swapchain, UINT64_MAX,
      m_imageAvailableSemaphores[m_currentFrameIndex], VK_NULL_HANDLE, &swapchainImageIndex);

    if (acquireImageResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
      rebuildSwapchain();
      return;
    }
  }

  vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrameIndex]);

  recordCommandBuffers(swapchainImageIndex);
  submitCommandBuffers(swapchainImageIndex);

  m_currentFrameIndex = (m_currentFrameIndex + 1) & FRAMES_IN_FLIGHT;
}

void cassidy::Renderer::release()
{
  vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrameIndex], VK_TRUE, UINT64_MAX);

  m_deletionQueue.execute();
  std::cout << "Renderer shut down!\n" << std::endl;
}

void cassidy::Renderer::recordCommandBuffers(uint32_t imageIndex)
{
  const VkCommandBuffer& cmd = m_commandBuffers[m_currentFrameIndex];

  vkResetCommandBuffer(cmd, 0);

  VkCommandBufferBeginInfo beginInfo = cassidy::init::commandBufferBeginInfo(0, nullptr);
  vkBeginCommandBuffer(cmd, &beginInfo);

  VkClearValue clearValues[2];
  clearValues[0].color = { 0.2f, 0.3f, 0.3f, 1.0f };
  clearValues[1].depthStencil = { 1.0f, 0 };

  VkRenderPassBeginInfo renderPassInfo = cassidy::init::renderPassBeginInfo(m_helloTrianglePipeline.getRenderPass(),
    m_swapchain.framebuffers[imageIndex], { 0, 0 }, m_swapchain.extent, 2, clearValues);

  vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_helloTrianglePipeline.getGraphicsPipeline());

    VkViewport viewport = cassidy::init::viewport(0.0f, 0.0f, m_swapchain.extent.width, m_swapchain.extent.height);
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = cassidy::init::scissor({ 0, 0 }, m_swapchain.extent);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdDraw(cmd, 3, 1, 0, 0);
  }
  vkCmdEndRenderPass(cmd);
  vkEndCommandBuffer(cmd);
}

void cassidy::Renderer::submitCommandBuffers(uint32_t imageIndex)
{
  VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
  VkSubmitInfo submitInfo = cassidy::init::submitInfo(1, &m_imageAvailableSemaphores[m_currentFrameIndex], waitStages, 
    1, &m_renderFinishedSemaphores[m_currentFrameIndex], 1, &m_commandBuffers[m_currentFrameIndex]);

  vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrameIndex]);

  VkPresentInfoKHR presentInfo = cassidy::init::presentInfo(1, &m_renderFinishedSemaphores[m_currentFrameIndex],
    1, &m_swapchain.swapchain, &imageIndex);

  const VkResult presentResult = vkQueuePresentKHR(m_presentQueue, &presentInfo);

  if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) 
  {
    rebuildSwapchain();
  }
}

void cassidy::Renderer::allocateBuffer()
{
  VkBufferCreateInfo bufferInfo = cassidy::init::bufferCreateInfo(triangleVertices.size() * sizeof(Vertex),
    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
  VmaAllocationCreateInfo bufferAllocInfo = cassidy::init::vmaAllocationCreateInfo(VMA_MEMORY_USAGE_GPU_ONLY,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);


}

void cassidy::Renderer::initLogicalDevice()
{
  m_physicalDevice = cassidy::helper::pickPhysicalDevice(m_engineRef->getInstance());
  if (m_physicalDevice == VK_NULL_HANDLE)
  {
    std::cout << "ERROR: Failed to find physical device!\n" << std::endl;
    return;
  }

  QueueFamilyIndices indices = cassidy::helper::findQueueFamilies(m_physicalDevice, m_engineRef->getSurface());

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

void cassidy::Renderer::initMemoryAllocator()
{
  VmaAllocatorCreateInfo info = {};
  info.physicalDevice = m_physicalDevice;
  info.device = m_device;
  info.instance = m_engineRef->getInstance();

  vmaCreateAllocator(&info, &m_allocator);

  m_deletionQueue.addFunction([=]() {
    vmaDestroyAllocator(m_allocator);
  });
}

void cassidy::Renderer::initSwapchain()
{
  SwapchainSupportDetails details = cassidy::helper::querySwapchainSupport(m_physicalDevice, m_engineRef->getSurface());

  // If desired format isn't available on the chosen physical device, default to the first available format:
  const VkSurfaceFormatKHR desiredFormat = { VK_FORMAT_B8G8R8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
  VkSurfaceFormatKHR surfaceFormat = cassidy::helper::isSwapchainSurfaceFormatSupported(details.formats.size(), details.formats.data(), 
    desiredFormat) ? desiredFormat : details.formats[0];

  // If desired present mode isn't available on the chosen physical device, default to FIFO (guaranteed to be avaiable):
  const VkPresentModeKHR desiredMode = VK_PRESENT_MODE_MAILBOX_KHR;
  VkPresentModeKHR presentMode = cassidy::helper::isSwapchainPresentModeSupported(details.presentModes.size(), details.presentModes.data(), 
    desiredMode) ? desiredMode : VK_PRESENT_MODE_FIFO_KHR;

  VkExtent2D extent = cassidy::helper::chooseSwapchainExtent(m_engineRef->getWindow(), details.capabilities);

  QueueFamilyIndices indices = cassidy::helper::findQueueFamilies(m_physicalDevice, m_engineRef->getSurface());
  VkSwapchainCreateInfoKHR swapchainInfo = cassidy::init::swapchainCreateInfo(details, indices, m_engineRef->getSurface(),
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

  // Create swapchain depth image and image view:
  VkFormat depthFormats[] = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
  const VkFormat& format = cassidy::helper::findSupportedFormat(m_physicalDevice, 3, depthFormats,
    VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

  VkImageCreateInfo depthImageInfo = cassidy::init::imageCreateInfo(VK_IMAGE_TYPE_2D, { m_swapchain.extent.width, m_swapchain.extent.height, 1 },
    1, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

  VmaAllocationCreateInfo vmaAllocInfo = cassidy::init::vmaAllocationCreateInfo(VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  vmaCreateImage(m_allocator, &depthImageInfo, &vmaAllocInfo, 
    &m_swapchain.depthImage.image, &m_swapchain.depthImage.allocation, nullptr);

  VkImageViewCreateInfo depthViewInfo = cassidy::init::imageViewCreateInfo(m_swapchain.depthImage.image, format,
    VK_IMAGE_ASPECT_DEPTH_BIT, 1);

  vkCreateImageView(m_device, &depthViewInfo, nullptr, &m_swapchain.depthView);

  m_deletionQueue.addFunction([&]() {
    m_swapchain.release(m_device, m_allocator);
  });

  std::cout << "Created swapchain!\n" << std::endl;
}

void cassidy::Renderer::initPipelines()
{
  m_helloTrianglePipeline.init(this, "../Shaders/helloTriangleVert.spv", "../Shaders/helloTriangleFrag.spv");

  m_deletionQueue.addFunction([=]() {
    m_helloTrianglePipeline.release();
  });
}

void cassidy::Renderer::initSwapchainFramebuffers()
{
  m_swapchain.framebuffers.resize(m_swapchain.imageViews.size());

  for (uint8_t i = 0; i < m_swapchain.imageViews.size(); ++i)
  {
    VkImageView attachments[] = { m_swapchain.imageViews[i], m_swapchain.depthView };

    VkFramebufferCreateInfo framebufferInfo = cassidy::init::framebufferCreateInfo(m_helloTrianglePipeline.getRenderPass(),
      2, attachments, m_swapchain.extent);

    vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapchain.framebuffers[i]);
  }
  std::cout << "Created swapchain framebuffers!\n" << std::endl;
}

void cassidy::Renderer::initCommandPool()
{
  QueueFamilyIndices indices = cassidy::helper::findQueueFamilies(m_physicalDevice, m_engineRef->getSurface());

  VkCommandPoolCreateInfo poolInfo = cassidy::init::commandPoolCreateInfo(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    indices.graphicsFamily.value());

  vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool);

  m_deletionQueue.addFunction([=]() {
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
  });

  std::cout << "Created command pool!\n" << std::endl;
}

void cassidy::Renderer::initCommandBuffers()
{
  m_commandBuffers.resize(FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocInfo = cassidy::init::commandBufferAllocInfo(m_commandPool,
    VK_COMMAND_BUFFER_LEVEL_PRIMARY, FRAMES_IN_FLIGHT);

  vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data());

  std::cout << "Allocated " << std::to_string(FRAMES_IN_FLIGHT) << " command buffers!\n" << std::endl;
}

void cassidy::Renderer::initSyncObjects()
{
  m_imageAvailableSemaphores.resize(FRAMES_IN_FLIGHT);
  m_renderFinishedSemaphores.resize(FRAMES_IN_FLIGHT);
  m_inFlightFences.resize(FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo semaphoreInfo = cassidy::init::semaphoreCreateInfo(0);
  VkFenceCreateInfo fenceInfo = cassidy::init::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

  for (uint8_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
  {
    vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]);
    vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]);
    vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]);

    m_deletionQueue.addFunction([=]() {
      vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
      vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
      vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
    });
  }

  std::cout << "Created synchronisation objects!\n" << std::endl;
}

void cassidy::Renderer::initVertexBuffers()
{
}

void cassidy::Renderer::rebuildSwapchain()
{
  int width;
  int height;

  SDL_Window* window = m_engineRef->getWindow();
  SDL_GetWindowSize(window, &width, &height);

  m_swapchain.release(m_device, m_allocator);
  initSwapchain();
  initSwapchainFramebuffers();
}