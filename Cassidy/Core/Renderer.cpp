#include "Renderer.h"
#include <Core/Engine.h>
#include <Core/ResourceManager.h>
#include <Core/Logger.h>

#include <Utils/DescriptorBuilder.h>
#include <Utils/Helpers.h>
#include <Utils/Initialisers.h>

#include <SDL.h>
#include <SDL_vulkan.h>

#define VMA_IMPLEMENTATION
#include "Vendor/vma/vk_mem_alloc.h"

#include <Vendor/imgui-docking/imgui_impl_sdl2.h>
#include <Vendor/imgui-docking/imgui_impl_vulkan.h>

#include <assimp/postprocess.h>

#include <set>
#include <iostream>

void cassidy::Renderer::init(cassidy::Engine* engine)
{
  m_engineRef = engine;

  initLogicalDevice();
  initSyncObjects();
  initCommandPool();
  initCommandBuffers();
  initResourceManager();
  initSwapchain();
  transitionSwapchainImages();
  initEditorResources();
  initDescriptorSets();
  initImGui();
  initPostProcessResources();
  initPipelines();
  initSwapchainFramebuffers();  // (swapchain framebuffers are dependent on back buffer pipeline's render pass)
  initVertexBuffers();
  initIndexBuffers();

  m_currentFrameIndex = 0;
  m_swapchainImageIndex = 0;
  m_currentFrame = 0;
}

void cassidy::Renderer::draw()
{
  vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrameIndex], VK_TRUE, UINT64_MAX);
  {
    const VkResult acquireImageResult = vkAcquireNextImageKHR(m_device, m_swapchain.swapchain, UINT64_MAX,
      m_imageAvailableSemaphores[m_currentFrameIndex], VK_NULL_HANDLE, &m_swapchainImageIndex);

    if (acquireImageResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
      CS_LOG_CRITICAL("Rebuilding swapchain (acquire image)");
      rebuildSwapchain();
      return;
    }
  }

  vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrameIndex]);

  const FrameData& currentFrameData = getCurrentFrameData();
  const int32_t& currentModelIndex = m_engineRef->getUIContext().selectedModel;
  constexpr ModelManager& modelManager = cassidy::globals::g_resourceManager.modelManager;
  m_currentModel = modelManager.getModelsPtrTable()[currentModelIndex];

  DebugContext& engineDebugContext = m_engineRef->getDebugContext();
  engineDebugContext.currentFrame = m_currentFrame;
  engineDebugContext.currentSwapchainImageIndex = m_swapchainImageIndex;

  updateBuffers(currentFrameData);
  recordViewportCommands(m_swapchainImageIndex);
  recordEditorCommands(m_swapchainImageIndex);
  submitCommandBuffers(m_swapchainImageIndex);

  m_currentFrameIndex = (m_currentFrameIndex + 1) % FRAMES_IN_FLIGHT;
  ++m_currentFrame;
}

void cassidy::Renderer::release()
{
  // Wait on device idle to prevent in-use resources from being destroyed:
  vkDeviceWaitIdle(m_device);

  m_deletionQueue.execute();
  
  CS_LOG_INFO("Renderer shut down!");
}

void cassidy::Renderer::updateBuffers(const FrameData& currentFrameData)
{
  const VmaAllocator allocator = getVmaAllocator();

  MatrixBufferData matrixBufferData;
  matrixBufferData.view = m_engineRef->getCamera().getLookatMatrix();
  matrixBufferData.proj = m_engineRef->getCamera().getPerspectiveMatrix();
  matrixBufferData.viewProj = matrixBufferData.proj * matrixBufferData.view;
  matrixBufferData.invViewProj = glm::inverse(matrixBufferData.viewProj);

  void* matrixBufferDataPtr;
  vmaMapMemory(allocator, currentFrameData.perPassMatrixUniformBuffer.allocation, &matrixBufferDataPtr);
  memcpy(matrixBufferDataPtr, &matrixBufferData, sizeof(MatrixBufferData));
  vmaUnmapMemory(allocator, currentFrameData.perPassMatrixUniformBuffer.allocation);

  LightBufferData lightBufferData;
  lightBufferData.numActiveLights = m_numActiveLights;
  for (uint32_t i = 0; i < NUM_LIGHTS; ++i)
  {
    glm::mat4 lightWorld = glm::rotate(glm::mat4(1.0f), glm::radians(m_lightRotation[i].x), glm::vec3(1.0f, 0.0f, 0.0f));
    lightWorld = glm::rotate(lightWorld, glm::radians(m_lightRotation[i].y), glm::vec3(0.0f, 1.0f, 0.0f));
    lightWorld = glm::rotate(lightWorld, glm::radians(m_lightRotation[i].z), glm::vec3(0.0f, 0.0f, 1.0f));

    lightBufferData.dirLights[i].directionWS = lightWorld * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    lightBufferData.dirLights[i].colour = glm::vec3(1.0f);
    lightBufferData.dirLights[i].ambient = m_lightAmbient[i];
  }

  void* lightBufferDataPtr;
  vmaMapMemory(allocator, currentFrameData.perPassLightUniformBuffer.allocation, &lightBufferDataPtr);
  memcpy(lightBufferDataPtr, &lightBufferData, sizeof(LightBufferData));
  vmaUnmapMemory(allocator, currentFrameData.perPassLightUniformBuffer.allocation);

  glm::mat4 objectWorld = glm::rotate(glm::mat4(1.0f), glm::radians(m_objectRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
  objectWorld = glm::rotate(objectWorld, glm::radians(m_objectRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
  objectWorld = glm::rotate(objectWorld, glm::radians(m_objectRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));  

  PerObjectData perObjectData;
  perObjectData.world = objectWorld;

  char* perObjectDataPtr;
  vmaMapMemory(allocator, m_perObjectUniformBufferDynamic.allocation, (void**)&perObjectDataPtr);
  perObjectDataPtr += m_currentFrameIndex * cassidy::helper::padUniformBufferSize(sizeof(PerObjectData), m_physicalDeviceProperties);
  memcpy(perObjectDataPtr, &perObjectData, sizeof(PerObjectData));
  vmaUnmapMemory(allocator, m_perObjectUniformBufferDynamic.allocation);
}
 
void cassidy::Renderer::recordViewportCommands(uint32_t imageIndex)
{
  const VkCommandBuffer& cmd = m_viewportCommandBuffers[m_currentFrameIndex];

  VK_CHECK(vkResetCommandBuffer(cmd, 0));

  VkCommandBufferBeginInfo beginInfo = cassidy::init::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr);
  vkBeginCommandBuffer(cmd, &beginInfo);

  VkClearValue clearValues[2];
  clearValues[0].color = { std::powf(0.2f, 2.2f), std::powf(0.3f, 2.2f), std::powf(0.3f, 2.2f), 1.0f };
  clearValues[1].depthStencil = { 1.0f, 0 };

  VkRenderPassBeginInfo renderPassInfo = cassidy::init::renderPassBeginInfo(m_viewportRenderPass,
    m_viewportFramebuffers[imageIndex], { 0, 0 }, m_swapchain.extent, 2, clearValues);

  vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_viewportPipeline.getPipeline());

    VkViewport viewport = cassidy::init::viewport(0.0f, 0.0f, m_swapchain.extent.width, m_swapchain.extent.height);
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = cassidy::init::scissor({ 0, 0 }, m_swapchain.extent);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // Bind per-pass descriptor set to slot 0:
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_viewportPipeline.getLayout(),
      0, 1, &getCurrentFrameData().perPassSet, 0, nullptr);

    // Bind per-object dynamic descriptor set to slot 1:
    const uint32_t dynamicUniformOffset = m_currentFrameIndex * cassidy::helper::padUniformBufferSize(sizeof(PerObjectData), m_physicalDeviceProperties);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_viewportPipeline.getLayout(),
      1, 1, &getCurrentFrameData().perObjectSet, 1, &dynamicUniformOffset);
    
    if (!m_currentModel)
    {
      constexpr ModelManager& modelManager = cassidy::globals::g_resourceManager.modelManager;
      modelManager.getModel("Helmet/DamagedHelmet.gltf")->draw(cmd, &m_viewportPipeline);
    }
    else
      m_currentModel->draw(cmd, &m_viewportPipeline);
  }
  vkCmdEndRenderPass(cmd);
  
  // Record post process dispatch commands:
  m_postProcessStack.recordCommands(cmd, imageIndex);

  VK_CHECK(vkEndCommandBuffer(cmd));
}

void cassidy::Renderer::recordEditorCommands(uint32_t imageIndex)
{
  const VkCommandBuffer& cmd = m_commandBuffers[m_currentFrameIndex];

  VK_CHECK(vkResetCommandBuffer(cmd, 0));

  VkCommandBufferBeginInfo beginInfo = cassidy::init::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr);
  vkBeginCommandBuffer(cmd, &beginInfo);

  // Transition editor image to colour attachment for ImGui rendering:
  cassidy::helper::transitionImageLayout(cmd,
    m_editorImages[imageIndex].image, m_swapchain.imageFormat,
    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    1);

  VkClearValue clearValues[2];
  clearValues[0].color = { 0.2f, 0.3f, 0.3f, 1.0f };
  clearValues[1].depthStencil = { 1.0f, 0 };

  VkRenderPassBeginInfo renderPassInfo = cassidy::init::renderPassBeginInfo(m_editorRenderPass,
    m_editorFramebuffers[imageIndex], { 0, 0 }, m_swapchain.extent, 2, clearValues);

  // Draw ImGui contents:
  vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
  vkCmdEndRenderPass(cmd);

  // Transition editor image and swapchain image to transfer layout:
  // (editor render pass has implicit UNDEFINED -> TRANSFER_SRC transition)
  cassidy::helper::transitionImageLayout(cmd,
    m_swapchain.images[imageIndex], m_swapchain.imageFormat,
    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    1);

  const int32_t imageWidth = static_cast<int32_t>(m_swapchain.extent.width);
  const int32_t imageHeight = static_cast<int32_t>(m_swapchain.extent.height);

  // Copy editor image into swapchain image:
  VkImageBlit blit = {
    .srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
    .srcOffsets = { { 0, 0, 0 }, { imageWidth, imageHeight, 1, }
                  },
    .dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
    .dstOffsets = { { 0, 0, 0 }, { imageWidth, imageHeight, 1, }
                  },
  };

  vkCmdBlitImage(cmd,
    m_editorImages[imageIndex].image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    m_swapchain.images[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1, &blit, VK_FILTER_NEAREST);

  // Transition swapchain image to present layout:
  cassidy::helper::transitionImageLayout(cmd,
    m_swapchain.images[imageIndex], m_swapchain.imageFormat,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    VK_ACCESS_NONE, VK_ACCESS_MEMORY_READ_BIT,
    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    1);

  VK_CHECK(vkEndCommandBuffer(cmd));
}

void cassidy::Renderer::submitCommandBuffers(uint32_t imageIndex)
{
  cassidy::TextureLibrary& texLibrary = cassidy::globals::g_resourceManager.textureLibrary;

  VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
  VkCommandBuffer submitBuffers[] = {
    m_viewportCommandBuffers[m_currentFrameIndex],
    m_commandBuffers[m_currentFrameIndex],
    texLibrary.getBlitCommandsList().cmd,
  };

  uint32_t numCmdBuffers = 2;
  if (texLibrary.getBlitCommandsList().numTextureCommandsRecorded > 0)
  {
    cassidy::TextureLibrary::BlitCommandsList& blitCommandsList = texLibrary.getBlitCommandsList();
    std::unique_lock<std::mutex> lock(blitCommandsList.recordingMutex, std::defer_lock);

    if (lock.try_lock())
    {
      CS_LOG_INFO("Merged {0} blit commands into render loop command submission!", blitCommandsList.numTextureCommandsRecorded);
      numCmdBuffers = 3;
      vkEndCommandBuffer(blitCommandsList.cmd);
      blitCommandsList.numTextureCommandsRecorded = 0;
    }
  }

  VkSubmitInfo submitInfo = cassidy::init::submitInfo(1, &m_imageAvailableSemaphores[m_currentFrameIndex], waitStages, 
    1, &m_renderFinishedSemaphores[m_currentFrameIndex], numCmdBuffers, submitBuffers);

  VK_CHECK(vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrameIndex]));

  VkPresentInfoKHR presentInfo = cassidy::init::presentInfo(1, &m_renderFinishedSemaphores[m_currentFrameIndex],
    1, &m_swapchain.swapchain, &imageIndex);

  const VkResult presentResult = vkQueuePresentKHR(m_presentQueue, &presentInfo);

  if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) 
  {
    CS_LOG_CRITICAL("Rebuilding swapchain (present)");
    rebuildSwapchain();
  }
}

AllocatedBuffer cassidy::Renderer::allocateVertexBuffer(const std::vector<Vertex>& vertices)
{
  const VmaAllocator allocator = getVmaAllocator();

  // Build CPU-side staging buffer:
  VkBufferCreateInfo stagingBufferInfo = cassidy::init::bufferCreateInfo(
    static_cast<uint32_t>(vertices.size()) * sizeof(Vertex), VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
  VmaAllocationCreateInfo bufferAllocInfo = cassidy::init::vmaAllocationCreateInfo(
    VMA_MEMORY_USAGE_AUTO_PREFER_HOST, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

  AllocatedBuffer stagingBuffer;

  vmaCreateBuffer(allocator, &stagingBufferInfo, &bufferAllocInfo,
    &stagingBuffer.buffer,
    &stagingBuffer.allocation,
    nullptr);

  // Write vertex data to newly-allocated buffer:
  void* data;
  vmaMapMemory(allocator, stagingBuffer.allocation, &data);
  memcpy(data, vertices.data(), vertices.size() * sizeof(Vertex));
  vmaUnmapMemory(allocator, stagingBuffer.allocation);

  VkBufferCreateInfo vertexBufferInfo = cassidy::init::bufferCreateInfo(
    static_cast<uint32_t>(vertices.size()) * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

  bufferAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

  AllocatedBuffer newBuffer;

  vmaCreateBuffer(allocator, &vertexBufferInfo, &bufferAllocInfo,
    &newBuffer.buffer,
    &newBuffer.allocation,
    nullptr);

  // Execute copy command for CPU-side staging buffer -> GPU-side vertex buffer:
  immediateSubmit([=](VkCommandBuffer cmd) {
    VkBufferCopy copy = {};
    copy.dstOffset = 0;
    copy.srcOffset = 0;
    copy.size = vertices.size() * sizeof(Vertex);
    vkCmdCopyBuffer(cmd, stagingBuffer.buffer, newBuffer.buffer, 1, &copy);
    });

  m_deletionQueue.addFunction([=]() {
    vmaDestroyBuffer(allocator, newBuffer.buffer, newBuffer.allocation);
    });

  vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);

  return newBuffer;
}

AllocatedBuffer cassidy::Renderer::allocateBuffer(uint32_t allocSize, VkBufferUsageFlags usageFlags, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlagBits allocFlags)
{
  VkBufferCreateInfo bufferInfo = cassidy::init::bufferCreateInfo(allocSize, usageFlags);

  VmaAllocationCreateInfo allocInfo = cassidy::init::vmaAllocationCreateInfo(memoryUsage, allocFlags);

  AllocatedBuffer newBuffer;

  vmaCreateBuffer(getVmaAllocator(), &bufferInfo, &allocInfo, &newBuffer.buffer, &newBuffer.allocation, nullptr);

  return newBuffer;
}

void cassidy::Renderer::immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
{
  VkCommandBufferBeginInfo beginInfo = cassidy::init::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    nullptr);

  vkBeginCommandBuffer(m_uploadContext.uploadCommandBuffer, &beginInfo);
  {
    function(m_uploadContext.uploadCommandBuffer);
  }
  vkEndCommandBuffer(m_uploadContext.uploadCommandBuffer);

  VkSubmitInfo submitInfo = cassidy::init::submitInfo(0, nullptr, 0, 0, nullptr, 1, &m_uploadContext.uploadCommandBuffer);
  vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_uploadContext.uploadFence);

  vkWaitForFences(m_device, 1, &m_uploadContext.uploadFence, VK_TRUE, UINT64_MAX);
  vkResetFences(m_device, 1, &m_uploadContext.uploadFence);

  vkResetCommandPool(m_device, m_uploadContext.uploadCommandPool, 0);
}

void cassidy::Renderer::initLogicalDevice()
{
  CS_LOG_INFO("Picking physical device...");
  m_physicalDevice = cassidy::helper::pickPhysicalDevice(m_engineRef->getInstance());
  if (m_physicalDevice == VK_NULL_HANDLE)
  {
    CS_LOG_ERROR("Failed to find physical device!");
    return;
  }
  // Log chosen physical device's properties:
  vkGetPhysicalDeviceProperties(m_physicalDevice, &m_physicalDeviceProperties);

  QueueFamilyIndices indices = cassidy::helper::findQueueFamilies(m_physicalDevice, m_engineRef->getSurface());

  std::vector<VkDeviceQueueCreateInfo> queueInfos;
  std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value(), indices.uploadFamily.value() };

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies)
  {
    VkDeviceQueueCreateInfo queueInfo = cassidy::init::deviceQueueCreateInfo(queueFamily, 1, &queuePriority);
    queueInfos.push_back(queueInfo);
  }

  VkPhysicalDeviceFeatures deviceFeatures = {};
  deviceFeatures.samplerAnisotropy = VK_TRUE;

  VkDeviceCreateInfo deviceInfo = cassidy::init::deviceCreateInfo(
    static_cast<uint32_t>(queueInfos.size()), queueInfos.data(), &deviceFeatures,
    static_cast<uint32_t>(DEVICE_EXTENSIONS.size()), DEVICE_EXTENSIONS.data(), 
    static_cast<uint32_t>(VALIDATION_LAYERS.size()), VALIDATION_LAYERS.data());

  CS_LOG_INFO("Creating logical device...");
  const VkResult deviceCreateResult = vkCreateDevice(m_physicalDevice, &deviceInfo, nullptr, &m_device);
  VK_CHECK(deviceCreateResult);

  vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
  vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);
  vkGetDeviceQueue(m_device, indices.uploadFamily.value(), 0, &m_uploadContext.uploadQueue);

  m_uploadContext.graphicsQueueRef = m_graphicsQueue;

  m_deletionQueue.addFunction([=]() {
    vkDestroyDevice(m_device, nullptr);
  });

  CS_LOG_INFO("Created logical device!");
}

void cassidy::Renderer::initSwapchain()
{
  SwapchainSupportDetails details = cassidy::helper::querySwapchainSupport(m_physicalDevice, m_engineRef->getSurface());

  // If desired format isn't available on the chosen physical device, default to the first available format:
  const VkSurfaceFormatKHR desiredFormat = { VK_FORMAT_B8G8R8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
  VkSurfaceFormatKHR surfaceFormat = cassidy::helper::isSwapchainSurfaceFormatSupported(
    static_cast<uint32_t>(details.formats.size()), details.formats.data(), desiredFormat) ? desiredFormat : details.formats[0];

  // If desired present mode isn't available on the chosen physical device, default to FIFO (guaranteed to be avaiable):
  const VkPresentModeKHR desiredMode = VK_PRESENT_MODE_FIFO_KHR;
  VkPresentModeKHR presentMode = cassidy::helper::isSwapchainPresentModeSupported(
    static_cast<uint32_t>(details.presentModes.size()), details.presentModes.data(), desiredMode) ? desiredMode : VK_PRESENT_MODE_FIFO_KHR;

  VkExtent2D extent = cassidy::helper::chooseSwapchainExtent(m_engineRef->getWindow(), details.capabilities);

  QueueFamilyIndices indices = cassidy::helper::findQueueFamilies(m_physicalDevice, m_engineRef->getSurface());
  uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

  VkSwapchainCreateInfoKHR swapchainInfo = cassidy::init::swapchainCreateInfo(details, indices, m_engineRef->getSurface(),
    surfaceFormat, presentMode, extent, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 2, queueFamilyIndices);

  VK_CHECK(vkCreateSwapchainKHR(m_device, &swapchainInfo, nullptr, &m_swapchain.swapchain));

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
    VK_CHECK(vkCreateImageView(m_device, &imageViewInfo, nullptr, &m_swapchain.imageViews[i]));
  }

  // Create swapchain depth image and image view:
  VkFormat depthFormats[] = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
  const VkFormat& format = cassidy::helper::findSupportedFormat(m_physicalDevice, 3, depthFormats,
    VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

  VkImageCreateInfo depthImageInfo = cassidy::init::imageCreateInfo(VK_IMAGE_TYPE_2D, { m_swapchain.extent.width, m_swapchain.extent.height, 1 },
    1, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

  VmaAllocationCreateInfo vmaAllocInfo = cassidy::init::vmaAllocationCreateInfo(VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, 
    VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);

  vmaCreateImage(getVmaAllocator(), &depthImageInfo, &vmaAllocInfo,
    &m_swapchain.depthImage.image, &m_swapchain.depthImage.allocation, nullptr);

  VkImageViewCreateInfo depthViewInfo = cassidy::init::imageViewCreateInfo(m_swapchain.depthImage.image, format,
    VK_IMAGE_ASPECT_DEPTH_BIT, 1);

  VK_CHECK(vkCreateImageView(m_device, &depthViewInfo, nullptr, &m_swapchain.depthImage.view));

  // Prevent multiple deletion commands on swapchain if it gets rebuilt:
  if (!m_swapchain.hasBeenBuilt)
  {
    m_deletionQueue.addFunction([&]() {
      m_swapchain.release(m_device, getVmaAllocator());
    });
    m_swapchain.hasBeenBuilt = true;
  }

  CS_LOG_INFO("Created swapchain!");
}

void cassidy::Renderer::initResourceManager()
{
  CS_LOG_INFO("Initialising resource manager...");
  cassidy::globals::g_resourceManager.init(this, m_engineRef);

  m_deletionQueue.addFunction([=]() {
    cassidy::globals::g_resourceManager.release(m_device);
    });
}

void cassidy::Renderer::initEditorResources()
{
  initEditorImages();
  initEditorRenderPass();
  initEditorFramebuffers();
  
  m_deletionQueue.addFunction([&]() {
    for (const auto& i : m_editorImages)
    {
      vmaDestroyImage(getVmaAllocator(), i.image, i.allocation);
      vkDestroyImageView(m_device, i.view, nullptr);
    }
    for (const auto& fb : m_editorFramebuffers)
      vkDestroyFramebuffer(m_device, fb, nullptr);
    vkDestroyRenderPass(m_device, m_editorRenderPass, nullptr);
    });
}

void cassidy::Renderer::initEditorImages()
{
  CS_LOG_INFO("Creating editor image objects...");
  m_editorImages.resize(m_swapchain.images.size());

  for (size_t i = 0; i < m_editorImages.size(); ++i)
  {
    AllocatedImage& currentImage = m_editorImages[i];

    const VkFormat editorFormat = m_swapchain.imageFormat;
    const VkExtent3D imageExtent = {
      m_swapchain.extent.width,
      m_swapchain.extent.height,
      1,
    };

    VkImageCreateInfo imageInfo = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .pNext = nullptr,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = editorFormat,
      .extent = imageExtent,
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT  // (graphics pipelines can draw into this image)
                | VK_IMAGE_USAGE_STORAGE_BIT        // (compute shaders can write to this image)
                | VK_IMAGE_USAGE_TRANSFER_DST_BIT   // (this image can have other images copied into it)
                | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,  // (this image can be copied into other images)
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VmaAllocationCreateInfo allocInfo = cassidy::init::vmaAllocationCreateInfo(
      VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);

    vmaCreateImage(getVmaAllocator(), &imageInfo, &allocInfo, &currentImage.image,
      &currentImage.allocation, nullptr);

    m_editorImages[i].format = editorFormat;

    VkImageViewCreateInfo viewInfo = cassidy::init::imageViewCreateInfo(currentImage.image,
      currentImage.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    vkCreateImageView(m_device, &viewInfo, nullptr, &currentImage.view);
  }
  CS_LOG_INFO("Created editor images!");
}

void cassidy::Renderer::initEditorRenderPass()
{
  CS_LOG_INFO("Creating editor render pass...");
  VkAttachmentDescription colourAttachment = cassidy::init::attachmentDescription(
    m_swapchain.imageFormat, VK_SAMPLE_COUNT_1_BIT, 
    VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

  VkAttachmentReference colourAttachmentRef = cassidy::init::attachmentReference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

  VkSubpassDescription subpass = cassidy::init::subpassDescription(VK_PIPELINE_BIND_POINT_GRAPHICS, 1,
    &colourAttachmentRef, nullptr);
 
  VkSubpassDependency dependencies[] = {
    {
      .srcSubpass = VK_SUBPASS_EXTERNAL,
      .dstSubpass = 0,
      .srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
      .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
    },
    {
      .srcSubpass = 0,
      .dstSubpass = VK_SUBPASS_EXTERNAL,
      .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
      .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
      .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
    }
  };

  VkRenderPassCreateInfo renderPassInfo = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .pNext = nullptr,
    .attachmentCount = 1,
    .pAttachments = &colourAttachment,
    .subpassCount = 1,
    .pSubpasses = &subpass,
    .dependencyCount = 2,
    .pDependencies = dependencies,
  };

  VK_CHECK(vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_editorRenderPass));

  CS_LOG_INFO("Created editor render pass!");
}

void cassidy::Renderer::initEditorFramebuffers()
{
  CS_LOG_INFO("Creating editor framebuffers...");
  m_editorFramebuffers.resize(m_swapchain.images.size());

  for (size_t i = 0; i < m_editorFramebuffers.size(); ++i)
  {
    VkFramebufferCreateInfo framebufferInfo = cassidy::init::framebufferCreateInfo(m_editorRenderPass,
      1, &m_editorImages[i].view, m_swapchain.extent);

    vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_editorFramebuffers[i]);
  }
}

void cassidy::Renderer::initPipelines()
{
  CS_LOG_INFO("Creating pipelines...");

  m_helloTrianglePipeline.setDebugName("helloTrianglePipeline");
  m_viewportPipeline.setDebugName("viewportPipeline");

  PipelineBuilder pipelineBuilder(this);
  pipelineBuilder.addShaderStage(VK_SHADER_STAGE_VERTEX_BIT, "helloTriangleVert.spv")
    .addShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, "phongLightingFrag.spv")
    .setRenderPass(m_editorRenderPass)
    .addDescriptorSetLayout(m_perPassSetLayout)
    .addDescriptorSetLayout(m_perObjectSetLayout)
    .addDescriptorSetLayout(m_perMaterialSetLayout)
    .buildGraphicsPipeline(m_helloTrianglePipeline);

  pipelineBuilder.setRenderPass(m_viewportRenderPass)
    .buildGraphicsPipeline(m_viewportPipeline);

  m_deletionQueue.addFunction([=]() {
    m_helloTrianglePipeline.release(m_device);
    m_viewportPipeline.release(m_device);
  });
}

void cassidy::Renderer::initSwapchainFramebuffers()
{
  m_swapchain.framebuffers.resize(m_swapchain.imageViews.size());

  for (uint8_t i = 0; i < m_swapchain.imageViews.size(); ++i)
  {
    VkFramebufferCreateInfo framebufferInfo = cassidy::init::framebufferCreateInfo(m_editorRenderPass,
      1, &m_swapchain.imageViews[i], m_swapchain.extent);

    VK_CHECK(vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapchain.framebuffers[i]));
  }
  CS_LOG_INFO("Created swapchain framebuffers!");
}

void cassidy::Renderer::initCommandPool()
{
  CS_LOG_INFO("Creating command pools...");
  QueueFamilyIndices indices = cassidy::helper::findQueueFamilies(m_physicalDevice, m_engineRef->getSurface());

  // Create command pool for graphics commands:
  VkCommandPoolCreateInfo graphicsPoolInfo = cassidy::init::commandPoolCreateInfo(
    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, indices.graphicsFamily.value());

  VK_CHECK(vkCreateCommandPool(m_device, &graphicsPoolInfo, nullptr, &m_graphicsCommandPool));

  // Create command pool for upload commands:
  VkCommandPoolCreateInfo uploadPoolInfo = cassidy::init::commandPoolCreateInfo(
    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, indices.uploadFamily.value());

  VK_CHECK(vkCreateCommandPool(m_device, &uploadPoolInfo, nullptr, &m_uploadContext.uploadCommandPool));

  m_deletionQueue.addFunction([=]() {
    vkDestroyCommandPool(m_device, m_graphicsCommandPool, nullptr);
    vkDestroyCommandPool(m_device, m_uploadContext.uploadCommandPool, nullptr);
  });

  CS_LOG_INFO("Created command pools!");
}

void cassidy::Renderer::initCommandBuffers()
{
  CS_LOG_INFO("Allocating command buffers...");
  m_commandBuffers.resize(FRAMES_IN_FLIGHT);

  // Allocate command buffers for graphics commands:
  VkCommandBufferAllocateInfo graphicsAllocInfo = cassidy::init::commandBufferAllocInfo(
    m_graphicsCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, FRAMES_IN_FLIGHT);

  vkAllocateCommandBuffers(m_device, &graphicsAllocInfo, m_commandBuffers.data());

  CS_LOG_INFO("Created {0} graphics command buffers!", FRAMES_IN_FLIGHT);

  // Allocate command buffer for blit commands to generate mipmaps:
  VkCommandBufferAllocateInfo blitAllocInfo = cassidy::init::commandBufferAllocInfo(
    m_graphicsCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

  cassidy::TextureLibrary& texLibrary = cassidy::globals::g_resourceManager.textureLibrary;
  vkAllocateCommandBuffers(m_device, &blitAllocInfo, &texLibrary.getBlitCommandsList().cmd);

  CS_LOG_INFO("Created mipmap blit command buffer!");

  // Allocate command buffer for upload commands:
  VkCommandBufferAllocateInfo uploadAllocInfo = cassidy::init::commandBufferAllocInfo(
    m_uploadContext.uploadCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

  vkAllocateCommandBuffers(m_device, &uploadAllocInfo, &m_uploadContext.uploadCommandBuffer);
  CS_LOG_INFO("Created upload command buffer!");
}

void cassidy::Renderer::initSyncObjects()
{
  CS_LOG_INFO("Creating synchonisation objects...");
  m_imageAvailableSemaphores.resize(FRAMES_IN_FLIGHT);
  m_renderFinishedSemaphores.resize(FRAMES_IN_FLIGHT);
  m_inFlightFences.resize(FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo semaphoreInfo = cassidy::init::semaphoreCreateInfo(0);
  VkFenceCreateInfo fenceInfo = cassidy::init::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

  for (uint8_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
  {
    VK_CHECK(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]));
    VK_CHECK(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]));
    VK_CHECK(vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]));

    m_deletionQueue.addFunction([=]() {
      vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
      vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
      vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
    });
  }

  fenceInfo.flags = 0;
  VK_CHECK(vkCreateFence(m_device, &fenceInfo, nullptr, &m_uploadContext.uploadFence));

  m_deletionQueue.addFunction([=]() {
    vkDestroyFence(m_device, m_uploadContext.uploadFence, nullptr);
  });

  CS_LOG_INFO("Created synchronisation objects!");
}

void cassidy::Renderer::initDescriptorSets()
{
  initUniformBuffers();
  
  CS_LOG_INFO("Building descriptor sets...");
  constexpr uint32_t NUM_BINDINGS = 3;
  VkDescriptorSetLayoutBinding bindings[NUM_BINDINGS];
  for (uint32_t i = 0; i < 3; ++i)
  {
    bindings[i] =
    {
        .binding = i,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = &cassidy::globals::m_linearTextureSampler,
    };
  };

  VkDescriptorSetLayoutCreateInfo materialLayoutInfo = cassidy::init::descriptorSetLayoutCreateInfo(NUM_BINDINGS, bindings);
  m_perMaterialSetLayout = cassidy::globals::g_descLayoutCache.createDescLayout(&materialLayoutInfo);

  for (uint8_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
  {
    VkDescriptorBufferInfo matrixBufferInfo = cassidy::init::descriptorBufferInfo(
      m_frameData[i].perPassMatrixUniformBuffer.buffer, 0, sizeof(MatrixBufferData));
    VkDescriptorBufferInfo lightBufferInfo = cassidy::init::descriptorBufferInfo(
      m_frameData[i].perPassLightUniformBuffer.buffer, 0, sizeof(LightBufferData));

    VkDescriptorBufferInfo perObjectBufferInfo = cassidy::init::descriptorBufferInfo(
      m_perObjectUniformBufferDynamic.buffer, 0, sizeof(PerObjectData));

    cassidy::DescriptorBuilder::begin(&cassidy::globals::g_descAllocator, &cassidy::globals::g_descLayoutCache)
      .bindBuffer(0, &matrixBufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
      .bindBuffer(1, &lightBufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
      .build(m_frameData[i].perPassSet, m_perPassSetLayout);

    cassidy::DescriptorBuilder::begin(&cassidy::globals::g_descAllocator, &cassidy::globals::g_descLayoutCache)
      .bindBuffer(0, &perObjectBufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT)
      .build(m_frameData[i].perObjectSet, m_perObjectSetLayout);
  }

  m_deletionQueue.addFunction([=]() {
    cassidy::globals::g_descLayoutCache.release();
    cassidy::globals::g_descAllocator.release();
    });
  CS_LOG_INFO("Built descriptor sets!");
}

void cassidy::Renderer::initVertexBuffers()
{
  CS_LOG_INFO("Creating vertex buffers...");
  const VmaAllocator allocator = cassidy::globals::g_resourceManager.getVmaAllocator();
  m_triangleMesh.allocateVertexBuffers(m_uploadContext.uploadCommandBuffer, allocator, this);
  m_backpackMesh.allocateVertexBuffers(m_uploadContext.uploadCommandBuffer, allocator, this);

  m_deletionQueue.addFunction([=]() {
    const VmaAllocator allocator = cassidy::globals::g_resourceManager.getVmaAllocator();
    
    m_triangleMesh.release(m_device, allocator);
    m_backpackMesh.release(m_device, allocator);
    });

  CS_LOG_INFO("Created vertex buffers!");
}

void cassidy::Renderer::initIndexBuffers()
{
  CS_LOG_INFO("Creating index buffers...");
  const VmaAllocator allocator = cassidy::globals::g_resourceManager.getVmaAllocator();
  m_backpackMesh.allocateIndexBuffers(m_uploadContext.uploadCommandBuffer, allocator, this);
  m_triangleMesh.allocateIndexBuffers(m_uploadContext.uploadCommandBuffer, allocator, this);
  CS_LOG_INFO("Created index buffers!");
}

void cassidy::Renderer::initUniformBuffers()
{
  CS_LOG_INFO("Allocating uniform buffers...");
  const uint32_t objectBufferSize = FRAMES_IN_FLIGHT * cassidy::helper::padUniformBufferSize(sizeof(PerObjectData),
    m_physicalDeviceProperties);
  m_perObjectUniformBufferDynamic = allocateBuffer(objectBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

  for (uint8_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
  {
    m_frameData[i].perPassMatrixUniformBuffer = allocateBuffer(sizeof(MatrixBufferData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    m_frameData[i].perPassLightUniformBuffer = allocateBuffer(sizeof(LightBufferData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
  }

  m_deletionQueue.addFunction([=]() {
    const VmaAllocator allocator = cassidy::globals::g_resourceManager.getVmaAllocator();
    vmaDestroyBuffer(allocator, m_perObjectUniformBufferDynamic.buffer, m_perObjectUniformBufferDynamic.allocation);

    for (uint8_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
      vmaDestroyBuffer(allocator, m_frameData[i].perPassMatrixUniformBuffer.buffer,
        m_frameData[i].perPassMatrixUniformBuffer.allocation);
      vmaDestroyBuffer(allocator, m_frameData[i].perPassLightUniformBuffer.buffer,
        m_frameData[i].perPassLightUniformBuffer.allocation);
    }
  });
  CS_LOG_INFO("Allocated uniform buffers!");
}

void cassidy::Renderer::initImGui()
{
  CS_LOG_INFO("Initialising ImGui...");
  const uint32_t NUM_SIZES = 11;
  VkDescriptorPoolSize poolSizes[NUM_SIZES] =
  {
    { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
    { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
  };

  VkDescriptorPoolCreateInfo poolInfo = cassidy::init::descriptorPoolCreateInfo(
    NUM_SIZES, poolSizes, 1000 * NUM_SIZES);
  poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

  VkDescriptorPool imGuiPool;
  VK_CHECK(vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &imGuiPool));

  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange; // Handle cursor show/hide functionality ourselves.

  ImGui_ImplSDL2_InitForVulkan(m_engineRef->getWindow());

  ImGui_ImplVulkan_InitInfo initInfo = {};
  initInfo.Instance = m_engineRef->getInstance();
  initInfo.PhysicalDevice = m_physicalDevice;
  initInfo.Device = m_device;
  initInfo.Queue = m_graphicsQueue;
  initInfo.DescriptorPool = imGuiPool;
  initInfo.MinImageCount = static_cast<uint32_t>(m_swapchain.images.size());
  initInfo.ImageCount = static_cast<uint32_t>(m_swapchain.images.size());
  initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

  ImGui_ImplVulkan_Init(&initInfo, m_editorRenderPass);

  // Hacky fix for ImGui font upload requiring queue w/ graphics support:
  UploadContext imguiUploadContext =
  {
    .uploadCommandPool = m_graphicsCommandPool,
    .uploadCommandBuffer = m_commandBuffers[0],
    .uploadFence = m_uploadContext.uploadFence,
    .uploadQueue = m_graphicsQueue,
  };

  cassidy::helper::immediateSubmit(m_device, imguiUploadContext,
    [&](VkCommandBuffer cmd) {
    ImGui_ImplVulkan_CreateFontsTexture(cmd);
  });
  CS_LOG_INFO("Initialised ImGui Vulkan backend!");

  //m_editorFilebrowser = ImGui::FileBrowser(ImGuiFileBrowserFlags_MultipleSelection);
  m_editorFilebrowser.SetTitle("File browser");
  m_editorFilebrowser.SetTypeFilters({ ".obj", ".gltf", ".glb", ".fbx" });

  // Create sampler used for swapchain image in viewport:
  m_viewportSampler = cassidy::helper::createTextureSampler(m_device, m_physicalDeviceProperties,
    VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_FALSE);

  initViewportCommandPool();
  initViewportCommandBuffers();
  initViewportImages();
  initViewportRenderPass();
  initViewportFramebuffers();

  // Solution for full ImGui setup w/ viewport: https://github.com/ocornut/imgui/issues/5110

  ImGui_ImplVulkan_DestroyFontUploadObjects();

  m_deletionQueue.addFunction([&, imGuiPool]() {
    const VmaAllocator allocator = cassidy::globals::g_resourceManager.getVmaAllocator();
    
    for (auto fb : m_viewportFramebuffers)
      vkDestroyFramebuffer(m_device, fb, nullptr);
   
    for (uint32_t i = 0; i < m_viewportImages.size(); ++i)
    {
      vmaDestroyImage(allocator, m_viewportImages[i].image,
        m_viewportImages[i].allocation);
      vkDestroyImageView(m_device, m_viewportImages[i].view, nullptr);
    }

    vmaDestroyImage(allocator, m_viewportDepthImage.image,
      m_viewportDepthImage.allocation);
    vkDestroyImageView(m_device, m_viewportDepthImage.view, nullptr);

    vkDestroyCommandPool(m_device, m_viewportCommandPool, nullptr);
    vkDestroyRenderPass(m_device, m_viewportRenderPass, nullptr);

    vkDestroyDescriptorPool(m_device, imGuiPool, nullptr);
    vkDestroySampler(m_device, m_viewportSampler, nullptr);

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
  });

  CS_LOG_INFO("ImGui initialised!");
}

void cassidy::Renderer::initViewportImages()
{
  CS_LOG_INFO("Creating viewport image objects...");
  m_viewportImages.resize(m_swapchain.images.size());
  const VmaAllocator allocator = cassidy::globals::g_resourceManager.getVmaAllocator();

  for (size_t i = 0; i < m_viewportImages.size(); ++i)
  {
    const VkFormat format = m_swapchain.imageFormat;
    const VkExtent3D imageExtent = {
      m_swapchain.extent.width,
      m_swapchain.extent.height,
      1,
    };

    VkImageCreateInfo imageInfo = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .pNext = nullptr,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = format,
      .extent = imageExtent,
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VmaAllocationCreateInfo allocInfo = cassidy::init::vmaAllocationCreateInfo(
      VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);

    vmaCreateImage(allocator, &imageInfo, &allocInfo, &m_viewportImages[i].image,
      &m_viewportImages[i].allocation, nullptr);

    m_viewportImages[i].format = format;

    // Transition viewport image layout to IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
    cassidy::helper::immediateSubmit(m_device, m_uploadContext,
      [=](VkCommandBuffer cmd) {
        cassidy::helper::transitionImageLayout(m_uploadContext.uploadCommandBuffer,
          m_viewportImages[i].image, format,
          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
          VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_MEMORY_READ_BIT,
          VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
          1);
      });

    VkImageViewCreateInfo viewInfo = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext = nullptr,
      .image = m_viewportImages[i].image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = format,
      .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1, },
    };

    VK_CHECK(vkCreateImageView(m_device, &viewInfo, nullptr, &m_viewportImages[i].view));
  }

  // Create swapchain depth image and image view:
  VkFormat depthFormats[] = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
  const VkFormat& depthFormat = cassidy::helper::findSupportedFormat(m_physicalDevice, 3, depthFormats,
    VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

  VkImageCreateInfo depthImageInfo = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .pNext = nullptr,
    .imageType = VK_IMAGE_TYPE_2D,
    .format = depthFormat,
    .extent = { m_swapchain.extent.width, m_swapchain.extent.height, 1 },
    .mipLevels = 1,
    .arrayLayers = 1,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .tiling = VK_IMAGE_TILING_OPTIMAL,
    .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };

  VmaAllocationCreateInfo vmaAllocInfo = cassidy::init::vmaAllocationCreateInfo(
    VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);

  vmaCreateImage(allocator, &depthImageInfo, &vmaAllocInfo,
    &m_viewportDepthImage.image, &m_viewportDepthImage.allocation, nullptr);

  VkImageViewCreateInfo depthViewInfo = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .pNext = nullptr,
    .image = m_viewportDepthImage.image,
    .viewType = VK_IMAGE_VIEW_TYPE_2D,
    .format = depthFormat,
    .subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1, },
  };
  
  VK_CHECK(vkCreateImageView(m_device, &depthViewInfo, nullptr, &m_viewportDepthImage.view));

  m_viewportDescSets.resize(m_swapchain.images.size());
  for (uint32_t i = 0; i < m_viewportDescSets.size(); ++i)
  {
    m_viewportDescSets[i] = ImGui_ImplVulkan_AddTexture(m_viewportSampler,
      m_viewportImages[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  }
  CS_LOG_INFO("Created viewport images!");

  // (Deletion of viewport resources is handled in ImGui deletion queue function)
}

void cassidy::Renderer::initViewportRenderPass()
{
  CS_LOG_INFO("Creating viewport render pass...");
  VkAttachmentDescription colourAttachment = {
    .format = m_swapchain.imageFormat,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  };

  VkFormat depthFormatCandidates[] = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
  const VkFormat depthFormat = cassidy::helper::findSupportedFormat(m_physicalDevice, 3, depthFormatCandidates,
    VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

  VkAttachmentDescription depthAttachment = {
    .format = depthFormat,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
  };

  VkAttachmentReference colourRef = cassidy::init::attachmentReference(0, 
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  VkAttachmentReference depthRef = cassidy::init::attachmentReference(1, 
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

  VkSubpassDescription subpass = cassidy::init::subpassDescription(
    VK_PIPELINE_BIND_POINT_GRAPHICS, 1, &colourRef, &depthRef);

  VkSubpassDependency dependencies[] = {
    {
      .srcSubpass = VK_SUBPASS_EXTERNAL,
      .dstSubpass = 0,
      .srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
      .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
    },
    {
      .srcSubpass = 0,
      .dstSubpass = VK_SUBPASS_EXTERNAL,
      .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
      .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
      .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
    },
  };

  VkAttachmentDescription attachments[] = { colourAttachment, depthAttachment };

  VkRenderPassCreateInfo renderPassInfo = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .pNext = nullptr,
    .attachmentCount = 2,
    .pAttachments = attachments,
    .subpassCount = 1,
    .pSubpasses = &subpass,
    .dependencyCount = 2,
    .pDependencies = dependencies,
  };

  VK_CHECK(vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_viewportRenderPass));
  CS_LOG_INFO("Created viewport render pass!");
}

void cassidy::Renderer::initViewportCommandPool()
{
  CS_LOG_INFO("Creating viewport command pool...");
  QueueFamilyIndices indices = cassidy::helper::findQueueFamilies(m_physicalDevice, m_engineRef->getSurface());

  VkCommandPoolCreateInfo info = cassidy::init::commandPoolCreateInfo(
    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, indices.graphicsFamily.value());

  VK_CHECK(vkCreateCommandPool(m_device, &info, nullptr, &m_viewportCommandPool));
  CS_LOG_INFO("Created viewport command pool!");
}

void cassidy::Renderer::initViewportCommandBuffers()
{
  CS_LOG_INFO("Allocating viewport command buffers...");
  m_viewportCommandBuffers.resize(FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocInfo = cassidy::init::commandBufferAllocInfo(
    m_viewportCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, FRAMES_IN_FLIGHT);

  vkAllocateCommandBuffers(m_device, &allocInfo, m_viewportCommandBuffers.data());
  CS_LOG_INFO("Allocated {0} viewport commmand buffers!", m_viewportCommandBuffers.size());
}

void cassidy::Renderer::initViewportFramebuffers()
{
  CS_LOG_INFO("Creating viewport framebuffers...");
  m_viewportFramebuffers.resize(m_swapchain.imageViews.size());

  for (size_t i = 0; i < m_viewportFramebuffers.size(); ++i)
  {
    VkImageView imageViews[] = {
      m_viewportImages[i].view,
      m_viewportDepthImage.view,
    };

    VkFramebufferCreateInfo info = cassidy::init::framebufferCreateInfo(m_viewportRenderPass,
      2, imageViews, m_swapchain.extent);

    VK_CHECK(vkCreateFramebuffer(m_device, &info, nullptr, &m_viewportFramebuffers[i]));
  }
  CS_LOG_INFO("Created viewport framebuffers!");
}

void cassidy::Renderer::initPostProcessResources()
{
  constexpr size_t NUM_DEFAULT_EFFECTS = 1;
  constexpr DescriptorAllocator& alloc = cassidy::globals::g_descAllocator;
  constexpr DescriptorLayoutCache& cache = cassidy::globals::g_descLayoutCache;
  m_postProcessStack.init(NUM_DEFAULT_EFFECTS, this);

  initPostProcessPipelines();

  const cassidy::ComputePipeline pipelines[NUM_DEFAULT_EFFECTS] = {
    m_gammaCorrectPipeline,
  };

  for (size_t i = 0; i < NUM_DEFAULT_EFFECTS; ++i)
  {
    PostProcessResources res = {};
    res.resultsImages.resize(m_swapchain.images.size());
    res.descriptorSets.resize(m_swapchain.images.size());

    for (uint8_t j = 0; j < m_swapchain.images.size(); ++j)
    {
      res.resultsImages[j] = createPostProcessImage();

      VkDescriptorImageInfo resultsImageInfo = cassidy::init::descriptorImageInfo(
        VK_IMAGE_LAYOUT_GENERAL, res.resultsImages[j].view, cassidy::globals::m_linearTextureSampler);
      VkDescriptorImageInfo viewportImageInfo = cassidy::init::descriptorImageInfo(
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_viewportImages[j].view, cassidy::globals::m_linearTextureSampler);

      cassidy::DescriptorBuilder::begin(&alloc, &cache)
        .bindImage(0, &resultsImageInfo, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
        .bindImage(1, &viewportImageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
        .build(res.descriptorSets[j]);
    }
    res.pipeline = pipelines[i];

    m_postProcessStack.push(res);
  }

  for (uint32_t i = 0; i < m_viewportDescSets.size(); ++i)
  {
    m_viewportDescSets[i] = ImGui_ImplVulkan_AddTexture(m_viewportSampler,
      m_postProcessStack.get(0).resultsImages[i].view, VK_IMAGE_LAYOUT_GENERAL);
  }

  m_deletionQueue.addFunction([=]() {
    m_postProcessStack.release();
    });
}

AllocatedImage cassidy::Renderer::createPostProcessImage()
{
  CS_LOG_INFO("Creating post process image...");
  AllocatedImage newImage;
  const VmaAllocator allocator = cassidy::globals::g_resourceManager.getVmaAllocator();

  const VkFormat format = m_swapchain.imageFormat;
  const VkExtent3D imageExtent = {
    m_swapchain.extent.width,
    m_swapchain.extent.height,
    1,
  };

  VkImageCreateInfo imageInfo = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .pNext = nullptr,
    .imageType = VK_IMAGE_TYPE_2D,
    .format = format,
    .extent = imageExtent,
    .mipLevels = 1,
    .arrayLayers = 1,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .tiling = VK_IMAGE_TILING_OPTIMAL,
    .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };

  VmaAllocationCreateInfo allocInfo = cassidy::init::vmaAllocationCreateInfo(
    VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);

  vmaCreateImage(allocator, &imageInfo, &allocInfo, &newImage.image,
    &newImage.allocation, nullptr);

  newImage.format = format;

  // Transition viewport image layout to IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
  cassidy::helper::immediateSubmit(m_device, m_uploadContext,
    [=](VkCommandBuffer cmd) {
      cassidy::helper::transitionImageLayout(m_uploadContext.uploadCommandBuffer,
        newImage.image, format,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
        VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_MEMORY_READ_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        1);
    });

  VkImageViewCreateInfo viewInfo = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .pNext = nullptr,
    .image = newImage.image,
    .viewType = VK_IMAGE_VIEW_TYPE_2D,
    .format = format,
    .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1, },
  };

  VK_CHECK(vkCreateImageView(m_device, &viewInfo, nullptr, &newImage.view));

  CS_LOG_INFO("Created post process image!");
  return newImage;
}

void cassidy::Renderer::initPostProcessPipelines()
{
  constexpr DescriptorLayoutCache& cache = cassidy::globals::g_descLayoutCache;

  VkDescriptorSetLayoutBinding resultsImageBinding = cassidy::init::descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1,
    VK_SHADER_STAGE_COMPUTE_BIT, &cassidy::globals::m_linearTextureSampler);
  VkDescriptorSetLayoutBinding viewportImageBinding = cassidy::init::descriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
    VK_SHADER_STAGE_COMPUTE_BIT, &cassidy::globals::m_linearTextureSampler);

  VkDescriptorSetLayoutBinding bindings[] = {
    resultsImageBinding,
    viewportImageBinding,
  };

  VkDescriptorSetLayoutCreateInfo layoutInfo = cassidy::init::descriptorSetLayoutCreateInfo(2, bindings);

  VkDescriptorSetLayout gammaCorrectLayout = cache.createDescLayout(&layoutInfo);

  m_gammaCorrectPipeline.setDebugName("gammaCorrectPipeline");

  cassidy::PipelineBuilder pipelineBuilder(this);

  pipelineBuilder.addShaderStage(VK_SHADER_STAGE_COMPUTE_BIT, "gammaCorrectComp.spv")
    .addDescriptorSetLayout(gammaCorrectLayout)
    .buildComputePipeline(m_gammaCorrectPipeline);
}

void cassidy::Renderer::transitionSwapchainImages()
{
  CS_LOG_INFO("Transitioning swapchain images to PRESENT_SRC_KHR...");
  // Manually transition swapchain images to PRESENT_SRC_KHR since there's no render pass 
  // with implicit layout transition anymore:
  cassidy::helper::immediateSubmit(m_device, m_uploadContext, [=](VkCommandBuffer cmd) {
    for (size_t i = 0; i < m_swapchain.images.size(); ++i)
      cassidy::helper::transitionImageLayout(cmd, m_swapchain.images[i], m_swapchain.imageFormat,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        0, 0,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        1);
    });
  CS_LOG_INFO("Transitioned swapchain images!");
}

VmaAllocator cassidy::Renderer::getVmaAllocator()
{
  return cassidy::globals::g_resourceManager.getVmaAllocator();
}

void cassidy::Renderer::rebuildSwapchain()
{
  int width;
  int height;

  SDL_Window* window = m_engineRef->getWindow();
  SDL_GetWindowSize(window, &width, &height);
  const VmaAllocator allocator = cassidy::globals::g_resourceManager.getVmaAllocator();

  vkWaitForFences(m_device,static_cast<uint32_t>(m_inFlightFences.size()), m_inFlightFences.data(), VK_TRUE, UINT64_MAX);
  m_swapchain.release(m_device, allocator);

  // Release ImGui editor resources:
  for (const auto& f : m_editorFramebuffers)
    vkDestroyFramebuffer(m_device, f, nullptr);
  for (const auto& i : m_editorImages)
    vmaDestroyImage(allocator, i.image, i.allocation);
  for (const auto& i : m_editorImages)
    vkDestroyImageView(m_device, i.view, nullptr);

  // Release viewport resources:
  for (const auto& f : m_viewportFramebuffers)
    vkDestroyFramebuffer(m_device, f, nullptr);
  for (const auto& i : m_viewportImages)
    vmaDestroyImage(allocator, i.image, i.allocation);
  vmaDestroyImage(allocator, m_viewportDepthImage.image, m_viewportDepthImage.allocation);
  for (const auto& i : m_viewportImages)
    vkDestroyImageView(m_device, i.view, nullptr);
  vkDestroyImageView(m_device, m_viewportDepthImage.view, nullptr);

  for (size_t i = 0; i < m_viewportDescSets.size(); ++i)
    ImGui_ImplVulkan_RemoveTexture(m_viewportDescSets[i]);

  initSwapchain();
  transitionSwapchainImages();
  initSwapchainFramebuffers();
  initEditorImages();
  initEditorFramebuffers();
  initViewportImages();
  initViewportFramebuffers();
  
  // TODO: Recreate post process stack images and descriptor sets on swapchain rebuild
  initPostProcessResources();
}
