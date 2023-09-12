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
  initVertexBuffers();

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

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_triangleVertexBuffer.buffer, &offset);

    // Build world and viewProj matrices, upload to GPU via push constants:
    const glm::vec3 camPos = glm::vec3(0.0f, 0.0f, -3.0f);
    const glm::mat4 view = glm::translate(glm::mat4(1.0f), camPos);

    glm::mat4 proj = glm::perspective(glm::radians(70.0f), 
      static_cast<float>(m_engineRef->getWindowDim().x) / static_cast<float>(m_engineRef->getWindowDim().y), 
      0.1f, 300.0f);
    proj[1][1] *= -1.0f;  // Invert projection's transformation to y-coord to match Vulkan's expectations.

    glm::mat4 world = glm::mat4(1.0f);

    const DefaultPushConstants pushConstants = { world, proj * view };

    vkCmdPushConstants(cmd, m_helloTrianglePipeline.getLayout(),
      VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(DefaultPushConstants), &pushConstants);

    vkCmdDraw(cmd, triangleVertices.size(), 1, 0, 0);
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

AllocatedBuffer cassidy::Renderer::allocateBuffer(const std::vector<Vertex>& vertices)
{
  // Build CPU-side staging buffer:
  VkBufferCreateInfo stagingBufferInfo = cassidy::init::bufferCreateInfo(vertices.size() * sizeof(Vertex),
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
  VmaAllocationCreateInfo bufferAllocInfo = cassidy::init::vmaAllocationCreateInfo(VMA_MEMORY_USAGE_CPU_ONLY,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  AllocatedBuffer stagingBuffer;

  vmaCreateBuffer(m_allocator, &stagingBufferInfo, &bufferAllocInfo,
    &stagingBuffer.buffer,
    &stagingBuffer.allocation,
    nullptr);

  // Write vertex data to newly-allocated buffer:
  void* data;
  vmaMapMemory(m_allocator, stagingBuffer.allocation, &data);
  memcpy(data, vertices.data(), vertices.size() * sizeof(Vertex));
  vmaUnmapMemory(m_allocator, stagingBuffer.allocation);

  VkBufferCreateInfo vertexBufferInfo = cassidy::init::bufferCreateInfo(vertices.size() * sizeof(Vertex),
    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

  bufferAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  AllocatedBuffer newBuffer;

  vmaCreateBuffer(m_allocator, &vertexBufferInfo, &bufferAllocInfo, 
    &newBuffer.buffer,
    &newBuffer.allocation,
    nullptr);

  // Execute copy command for CPU-side staging buffer -> GPU-side vertex buffer:
  uploadBuffer([=](VkCommandBuffer cmd) {
    VkBufferCopy copy = {};
    copy.dstOffset = 0;
    copy.srcOffset = 0;
    copy.size = vertices.size() * sizeof(Vertex);
    vkCmdCopyBuffer(cmd, stagingBuffer.buffer, newBuffer.buffer, 1, &copy);
  });

  m_deletionQueue.addFunction([=]() {
    vmaDestroyBuffer(m_allocator, newBuffer.buffer, newBuffer.allocation);
  });

  vmaDestroyBuffer(m_allocator, stagingBuffer.buffer, stagingBuffer.allocation);

  return newBuffer;
}

void cassidy::Renderer::uploadBuffer(std::function<void(VkCommandBuffer cmd)>&& function)
{
  VkCommandBufferBeginInfo beginInfo = cassidy::init::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    nullptr);

  vkBeginCommandBuffer(m_uploadCommandBuffer, &beginInfo);
  {
    function(m_uploadCommandBuffer);
  }
  vkEndCommandBuffer(m_uploadCommandBuffer);

  VkSubmitInfo submitInfo = cassidy::init::submitInfo(0, nullptr, 0, 0, nullptr, 1, &m_uploadCommandBuffer);
  vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_uploadFence);

  vkWaitForFences(m_device, 1, &m_uploadFence, VK_TRUE, UINT64_MAX);
  vkResetFences(m_device, 1, &m_uploadFence);

  vkResetCommandPool(m_device, m_uploadCommandPool, 0);
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

  // Create command pool for graphics commands:
  VkCommandPoolCreateInfo graphicsPoolInfo = cassidy::init::commandPoolCreateInfo(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    indices.graphicsFamily.value());

  vkCreateCommandPool(m_device, &graphicsPoolInfo, nullptr, &m_graphicsCommandPool);

  // Create command pool for upload commands:
  VkCommandPoolCreateInfo uploadPoolInfo = cassidy::init::commandPoolCreateInfo(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    indices.graphicsFamily.value());

  vkCreateCommandPool(m_device, &uploadPoolInfo, nullptr, &m_uploadCommandPool);

  m_deletionQueue.addFunction([=]() {
    vkDestroyCommandPool(m_device, m_graphicsCommandPool, nullptr);
    vkDestroyCommandPool(m_device, m_uploadCommandPool, nullptr);
  });

  std::cout << "Created command pools!\n" << std::endl;
}

void cassidy::Renderer::initCommandBuffers()
{
  m_commandBuffers.resize(FRAMES_IN_FLIGHT);

  // Allocate command buffers for graphics commands:
  VkCommandBufferAllocateInfo graphicsAllocInfo = cassidy::init::commandBufferAllocInfo(m_graphicsCommandPool,
    VK_COMMAND_BUFFER_LEVEL_PRIMARY, FRAMES_IN_FLIGHT);

  vkAllocateCommandBuffers(m_device, &graphicsAllocInfo, m_commandBuffers.data());
  std::cout << "Allocated " << std::to_string(FRAMES_IN_FLIGHT) << " graphics command buffers!\n";

  // Allocate command buffer for upload commands:
  VkCommandBufferAllocateInfo uploadAllocInfo = cassidy::init::commandBufferAllocInfo(m_uploadCommandPool,
    VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

  vkAllocateCommandBuffers(m_device, &uploadAllocInfo, &m_uploadCommandBuffer);
  std::cout << "Allocated upload command buffer!\n" << std::endl;
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

  fenceInfo.flags = 0;
  vkCreateFence(m_device, &fenceInfo, nullptr, &m_uploadFence);

  m_deletionQueue.addFunction([=]() {
    vkDestroyFence(m_device, m_uploadFence, nullptr);
  });

  std::cout << "Created synchronisation objects!\n" << std::endl;
}

void cassidy::Renderer::initVertexBuffers()
{
  m_triangleVertexBuffer = allocateBuffer(triangleVertices);

  std::cout << "Created vertex buffers!\n" << std::endl;
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
