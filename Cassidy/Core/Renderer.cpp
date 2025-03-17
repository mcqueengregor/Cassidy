#include "Renderer.h"
#include "Core/Engine.h"
#include "Core/TextureLibrary.h"
#include "Utils/DescriptorBuilder.h"
#include "Utils/Helpers.h"
#include "Utils/Initialisers.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#define VMA_IMPLEMENTATION
#include "Vendor/vma/vk_mem_alloc.h"

#include <Vendor/imgui-docking/imgui.h>
#include <Vendor/imgui-docking/imgui_impl_sdl2.h>
#include <Vendor/imgui-docking/imgui_impl_vulkan.h>

#include <set>
#include <iostream>

void cassidy::Renderer::init(cassidy::Engine* engine)
{
  m_engineRef = engine;

  initLogicalDevice();
  initMemoryAllocator();
  initSwapchain();
  initEditorImages();
  initEditorRenderPass();
  initEditorFramebuffers();
  initCommandPool();
  initCommandBuffers();
  initSyncObjects();
  transitionSwapchainImages();
  initMeshes();
  initDescriptorSets();
  initImGui();
  initPipelines();
  initSwapchainFramebuffers();  // (swapchain framebuffers are dependent on back buffer pipeline's render pass)
  initVertexBuffers();
  initIndexBuffers();

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

  const FrameData& currentFrameData = getCurrentFrameData();

  updateBuffers(currentFrameData);
  recordViewportCommands(swapchainImageIndex);
  createImGuiCommands(swapchainImageIndex);
  recordEditorCommands(swapchainImageIndex);
  submitCommandBuffers(swapchainImageIndex);

  m_currentFrameIndex = (m_currentFrameIndex + 1) % FRAMES_IN_FLIGHT;
}

void cassidy::Renderer::release()
{
  // Wait on device idle to prevent in-use resources from being destroyed:
  vkDeviceWaitIdle(m_device);

  m_deletionQueue.execute();
  std::cout << "Renderer shut down!\n" << std::endl;
}

void cassidy::Renderer::updateBuffers(const FrameData& currentFrameData)
{
  PerPassData perPassData;
  perPassData.view = m_engineRef->getCamera().getLookatMatrix();
  perPassData.proj = m_engineRef->getCamera().getPerspectiveMatrix();
  perPassData.viewProj = perPassData.proj * perPassData.view;
  perPassData.invViewProj = glm::inverse(perPassData.viewProj);

  void* perPassDataPtr;
  vmaMapMemory(m_allocator, currentFrameData.perPassUniformBuffer.allocation, &perPassDataPtr);
  memcpy(perPassDataPtr, &perPassData, sizeof(PerPassData));
  vmaUnmapMemory(m_allocator, currentFrameData.perPassUniformBuffer.allocation);

  PerObjectData perObjectData;
  perObjectData.world = glm::mat4(1.0f);

  char* perObjectDataPtr;
  vmaMapMemory(m_allocator, m_perObjectUniformBufferDynamic.allocation, (void**)&perObjectDataPtr);
  perObjectDataPtr += m_currentFrameIndex * cassidy::helper::padUniformBufferSize(sizeof(PerObjectData), m_physicalDeviceProperties);
  memcpy(perObjectDataPtr, &perObjectData, sizeof(PerObjectData));
  vmaUnmapMemory(m_allocator, m_perObjectUniformBufferDynamic.allocation);
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
 
void cassidy::Renderer::recordViewportCommands(uint32_t imageIndex)
{
  const VkCommandBuffer& cmd = m_viewportCommandBuffers[m_currentFrameIndex];

  VK_CHECK(vkResetCommandBuffer(cmd, 0));

  VkCommandBufferBeginInfo beginInfo = cassidy::init::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr);
  vkBeginCommandBuffer(cmd, &beginInfo);

  VkClearValue clearValues[2];
  clearValues[0].color = { 0.2f, 0.3f, 0.3f, 1.0f };
  clearValues[1].depthStencil = { 1.0f, 0 };

  VkRenderPassBeginInfo renderPassInfo = cassidy::init::renderPassBeginInfo(m_viewportRenderPass,
    m_viewportFramebuffers[imageIndex], { 0, 0 }, m_swapchain.extent, 2, clearValues);

  vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_viewportPipeline.getGraphicsPipeline());

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

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_helloTrianglePipeline.getLayout(),
      2, 1, &getCurrentFrameData().m_backpackMaterialSet, 0, nullptr);

    vkCmdPushConstants(cmd, m_helloTrianglePipeline.getLayout(), VK_SHADER_STAGE_FRAGMENT_BIT,
      sizeof(DefaultPushConstants), sizeof(PhongLightingPushConstants), &m_phongLightingPushConstants);
    
    m_backpackMesh.draw(cmd);
  }

  vkCmdEndRenderPass(cmd);
  VK_CHECK(vkEndCommandBuffer(cmd));
}

void cassidy::Renderer::submitCommandBuffers(uint32_t imageIndex)
{
  VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
  VkCommandBuffer submitBuffers[] = {
    m_viewportCommandBuffers[m_currentFrameIndex],
    m_commandBuffers[m_currentFrameIndex],
  };

  VkSubmitInfo submitInfo = cassidy::init::submitInfo(1, &m_imageAvailableSemaphores[m_currentFrameIndex], waitStages, 
    1, &m_renderFinishedSemaphores[m_currentFrameIndex], 2, submitBuffers);

  VK_CHECK(vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrameIndex]));

  VkPresentInfoKHR presentInfo = cassidy::init::presentInfo(1, &m_renderFinishedSemaphores[m_currentFrameIndex],
    1, &m_swapchain.swapchain, &imageIndex);

  const VkResult presentResult = vkQueuePresentKHR(m_presentQueue, &presentInfo);

  if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) 
  {
    rebuildSwapchain();
  }
}

AllocatedBuffer cassidy::Renderer::allocateVertexBuffer(const std::vector<Vertex>& vertices)
{
  // Build CPU-side staging buffer:
  VkBufferCreateInfo stagingBufferInfo = cassidy::init::bufferCreateInfo(
    static_cast<uint32_t>(vertices.size()) * sizeof(Vertex), VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
  VmaAllocationCreateInfo bufferAllocInfo = cassidy::init::vmaAllocationCreateInfo(
    VMA_MEMORY_USAGE_AUTO_PREFER_HOST, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

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

  VkBufferCreateInfo vertexBufferInfo = cassidy::init::bufferCreateInfo(
    static_cast<uint32_t>(vertices.size()) * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

  bufferAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

  AllocatedBuffer newBuffer;

  vmaCreateBuffer(m_allocator, &vertexBufferInfo, &bufferAllocInfo, 
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
    vmaDestroyBuffer(m_allocator, newBuffer.buffer, newBuffer.allocation);
  });

  vmaDestroyBuffer(m_allocator, stagingBuffer.buffer, stagingBuffer.allocation);

  return newBuffer;
}

AllocatedBuffer cassidy::Renderer::allocateBuffer(uint32_t allocSize, VkBufferUsageFlags usageFlags, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlagBits allocFlags)
{
  VkBufferCreateInfo bufferInfo = cassidy::init::bufferCreateInfo(allocSize, usageFlags);

  VmaAllocationCreateInfo allocInfo = cassidy::init::vmaAllocationCreateInfo(memoryUsage, allocFlags);

  AllocatedBuffer newBuffer;

  vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo, &newBuffer.buffer, &newBuffer.allocation, nullptr);

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

void cassidy::Renderer::createImGuiCommands(uint32_t imageIndex)
{
  ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
  {
    ImGui::ShowDemoWindow();

    ImGui::Begin("Cassidy main");
    {
      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      ImGui::Text("Engine stats:");
      {
        ImGui::Text("Frametime: %fms", m_engineRef->getDeltaTimeSecs() * 1000.0f);

        std::string texLibraryHeaderText = "Texture library size: " + std::to_string(TextureLibrary::getNumLoadedTextures());

        if (ImGui::TreeNode(texLibraryHeaderText.c_str()))
        {
          const auto& textureLibrary = TextureLibrary::getTextureLibraryMap();
          for (const auto& texture : textureLibrary)
          {
            std::string_view textureFilename = texture.first;
            size_t lastBackSlash = textureFilename.find_last_of('/');
            ++lastBackSlash;  // Advance one character forward to isolate the filename.
            std::string_view textureFilenameSub = textureFilename.substr(lastBackSlash, textureFilename.size() - lastBackSlash);
            ImGui::Text(textureFilenameSub.data());
          }
          ImGui::TreePop();
        }
      }

      const char* const textures[] = {
        "Diffuse",
        "Specular",
        "Normal"
      };

      ImGui::ListBox("Textures", (int*)(&m_phongLightingPushConstants.texToDisplay), textures, 3);
      ImGui::Text("Current index: %u", m_phongLightingPushConstants.texToDisplay);
    }
    ImGui::End();

    if (ImGui::Begin("Viewport"))
    {
      ImVec2 viewportSize = ImGui::GetContentRegionAvail();
      ImVec2 newViewportSize = viewportSize;

      float viewportAspect = viewportSize.y / viewportSize.x;
      float swapchainAspect = static_cast<float>(m_swapchain.extent.height) / static_cast<float>(m_swapchain.extent.width);

      // Force viewport aspect ratio to be equal to swapchain extent's aspect ratio:
      if (viewportAspect > swapchainAspect)
      {
        newViewportSize.y = std::floor(viewportSize.x * swapchainAspect);
      }

      viewportAspect = viewportSize.x / viewportSize.y;
      swapchainAspect = static_cast<float>(m_swapchain.extent.width) / static_cast<float>(m_swapchain.extent.height);

      if (viewportAspect > swapchainAspect)
      {
        newViewportSize.x = std::floor(viewportSize.y * swapchainAspect);
      }

      // Centre viewport image in window:
      ImVec2 cursorPos = ImGui::GetCursorPos();
      cursorPos.x += (viewportSize.x - newViewportSize.x) * 0.5f;
      cursorPos.y += (viewportSize.y - newViewportSize.y) * 0.5f;

      ImGui::SetCursorPos(cursorPos);
      ImGui::Image(m_viewportDescSets[imageIndex], newViewportSize);
    }
    ImGui::End();
  }
  
  ImGui::Render();
}

void cassidy::Renderer::initLogicalDevice()
{
  m_physicalDevice = cassidy::helper::pickPhysicalDevice(m_engineRef->getInstance());
  if (m_physicalDevice == VK_NULL_HANDLE)
  {
    std::cout << "ERROR: Failed to find physical device!\n" << std::endl;
    return;
  }
  
  // Log chosen physical device's properties:
  vkGetPhysicalDeviceProperties(m_physicalDevice, &m_physicalDeviceProperties);

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

  VkDeviceCreateInfo deviceInfo = cassidy::init::deviceCreateInfo(
    static_cast<uint32_t>(queueInfos.size()), queueInfos.data(), &deviceFeatures,
    static_cast<uint32_t>(DEVICE_EXTENSIONS.size()), DEVICE_EXTENSIONS.data(), 
    static_cast<uint32_t>(VALIDATION_LAYERS.size()), VALIDATION_LAYERS.data());

  const VkResult deviceCreateResult = vkCreateDevice(m_physicalDevice, &deviceInfo, nullptr, &m_device);
  VK_CHECK(deviceCreateResult);

  vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
  vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);

  m_uploadContext.graphicsQueueRef = m_graphicsQueue;

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

  vmaCreateImage(m_allocator, &depthImageInfo, &vmaAllocInfo, 
    &m_swapchain.depthImage.image, &m_swapchain.depthImage.allocation, nullptr);

  VkImageViewCreateInfo depthViewInfo = cassidy::init::imageViewCreateInfo(m_swapchain.depthImage.image, format,
    VK_IMAGE_ASPECT_DEPTH_BIT, 1);

  VK_CHECK(vkCreateImageView(m_device, &depthViewInfo, nullptr, &m_swapchain.depthImage.view));

  // Prevent multiple deletion commands on swapchain if it gets rebuilt:
  if (!m_swapchain.hasBeenBuilt)
  {
    m_deletionQueue.addFunction([&]() {
      m_swapchain.release(m_device, m_allocator);
    });
    m_swapchain.hasBeenBuilt = true;
  }

  std::cout << "Created swapchain!\n" << std::endl;
}

void cassidy::Renderer::initEditorImages()
{
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

    vmaCreateImage(m_allocator, &imageInfo, &allocInfo, &currentImage.image,
      &currentImage.allocation, nullptr);

    m_editorImages[i].format = editorFormat;

    VkImageViewCreateInfo viewInfo = cassidy::init::imageViewCreateInfo(currentImage.image,
      currentImage.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    vkCreateImageView(m_device, &viewInfo, nullptr, &currentImage.view);
  }

  m_deletionQueue.addFunction([=]() {
    for (const auto& i : m_editorImages)
    {
      vmaDestroyImage(m_allocator, i.image, i.allocation);
      vkDestroyImageView(m_device, i.view, nullptr);
    }
    });
}

void cassidy::Renderer::initEditorRenderPass()
{
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

  m_deletionQueue.addFunction([=]() {
    vkDestroyRenderPass(m_device, m_editorRenderPass, nullptr);
    });

  std::cout << "Created editor render pass!" << std::endl;
}

void cassidy::Renderer::initEditorFramebuffers()
{
  m_editorFramebuffers.resize(m_swapchain.images.size());

  for (size_t i = 0; i < m_editorFramebuffers.size(); ++i)
  {
    VkFramebufferCreateInfo framebufferInfo = cassidy::init::framebufferCreateInfo(m_editorRenderPass,
      1, &m_editorImages[i].view, m_swapchain.extent);

    vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_editorFramebuffers[i]);
  }

  m_deletionQueue.addFunction([=]() {
    for (const auto& fb : m_editorFramebuffers)
      vkDestroyFramebuffer(m_device, fb, nullptr);
    });
}

void cassidy::Renderer::initPipelines()
{
  m_helloTrianglePipeline.init(this)
    .setRenderPass(m_editorRenderPass)
    .addPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(DefaultPushConstants))
    .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(DefaultPushConstants), sizeof(PhongLightingPushConstants))
    .addDescriptorSetLayout(m_perPassSetLayout)
    .addDescriptorSetLayout(m_perObjectSetLayout)
    .addDescriptorSetLayout(m_perMaterialSetLayout)
    .buildGraphicsPipeline("helloTriangleVert.spv", "phongLightingFrag.spv");

  m_viewportPipeline.init(this)
    .setRenderPass(m_viewportRenderPass)
    .addPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(DefaultPushConstants))
    .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(DefaultPushConstants), sizeof(PhongLightingPushConstants))
    .addDescriptorSetLayout(m_perPassSetLayout)
    .addDescriptorSetLayout(m_perObjectSetLayout)
    .addDescriptorSetLayout(m_perMaterialSetLayout)
    .buildGraphicsPipeline("helloTriangleVert.spv", "phongLightingFrag.spv");

  m_deletionQueue.addFunction([=]() {
    m_helloTrianglePipeline.release();
    m_viewportPipeline.release();
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
  std::cout << "Created swapchain framebuffers!\n" << std::endl;
}

void cassidy::Renderer::initCommandPool()
{
  QueueFamilyIndices indices = cassidy::helper::findQueueFamilies(m_physicalDevice, m_engineRef->getSurface());

  // Create command pool for graphics commands:
  VkCommandPoolCreateInfo graphicsPoolInfo = cassidy::init::commandPoolCreateInfo(
    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, indices.graphicsFamily.value());

  VK_CHECK(vkCreateCommandPool(m_device, &graphicsPoolInfo, nullptr, &m_graphicsCommandPool));

  // Create command pool for upload commands:
  VkCommandPoolCreateInfo uploadPoolInfo = cassidy::init::commandPoolCreateInfo(
    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, indices.graphicsFamily.value());

  VK_CHECK(vkCreateCommandPool(m_device, &uploadPoolInfo, nullptr, &m_uploadContext.uploadCommandPool));

  m_deletionQueue.addFunction([=]() {
    vkDestroyCommandPool(m_device, m_graphicsCommandPool, nullptr);
    vkDestroyCommandPool(m_device, m_uploadContext.uploadCommandPool, nullptr);
  });

  std::cout << "Created command pools!\n" << std::endl;
}

void cassidy::Renderer::initCommandBuffers()
{
  m_commandBuffers.resize(FRAMES_IN_FLIGHT);

  // Allocate command buffers for graphics commands:
  VkCommandBufferAllocateInfo graphicsAllocInfo = cassidy::init::commandBufferAllocInfo(
    m_graphicsCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, FRAMES_IN_FLIGHT);

  vkAllocateCommandBuffers(m_device, &graphicsAllocInfo, m_commandBuffers.data());
  std::cout << "Allocated " << std::to_string(FRAMES_IN_FLIGHT) << " graphics command buffers!\n";

  // Allocate command buffer for upload commands:
  VkCommandBufferAllocateInfo uploadAllocInfo = cassidy::init::commandBufferAllocInfo(
    m_uploadContext.uploadCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

  vkAllocateCommandBuffers(m_device, &uploadAllocInfo, &m_uploadContext.uploadCommandBuffer);
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

  std::cout << "Created synchronisation objects!\n" << std::endl;
}

void cassidy::Renderer::initMeshes()
{
  TextureLibrary::init(&m_allocator, this);

  m_triangleMesh.setVertices(triangleVertices);
  m_triangleMesh.setIndices(triangleIndices);

  m_backpackMesh.loadModel("Backpack/backpack.obj", m_allocator, this);
  m_backpackAlbedo.load(MESH_ABS_FILEPATH   + std::string("Backpack/diffuse.jpg"),  m_allocator, this, VK_FORMAT_R8G8B8A8_SRGB, VK_TRUE);
  m_backpackSpecular.load(MESH_ABS_FILEPATH + std::string("Backpack/specular.jpg"), m_allocator, this, VK_FORMAT_R8_UNORM, VK_TRUE);
  m_backpackNormal.load(MESH_ABS_FILEPATH   + std::string("Backpack/normal.png"),   m_allocator, this, VK_FORMAT_R8G8B8A8_UNORM, VK_TRUE);

  m_linearSampler = cassidy::helper::createTextureSampler(m_device, m_physicalDeviceProperties, VK_FILTER_LINEAR,
    VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_TRUE);

  m_deletionQueue.addFunction([=]() {
    m_backpackAlbedo.release(m_device, m_allocator);
    m_backpackSpecular.release(m_device, m_allocator);
    m_backpackNormal.release(m_device, m_allocator);

    TextureLibrary::releaseAll(m_device, m_allocator);
    vkDestroySampler(m_device, m_linearSampler, nullptr);
  });
}

void cassidy::Renderer::initDescriptorSets()
{
  cassidy::globals::g_descAllocator.init(m_device);
  cassidy::globals::g_descLayoutCache.init(m_device);

  initUniformBuffers();

  for (uint8_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
  {
    VkDescriptorBufferInfo perPassBufferInfo = cassidy::init::descriptorBufferInfo(
      m_frameData[i].perPassUniformBuffer.buffer, 0, sizeof(PerPassData));

    VkDescriptorBufferInfo perObjectBufferInfo = cassidy::init::descriptorBufferInfo(
      m_perObjectUniformBufferDynamic.buffer, 0, sizeof(PerObjectData));
    VkDescriptorImageInfo perObjectImageInfo = cassidy::init::descriptorImageInfo(
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_backpackAlbedo.getImageView(), m_linearSampler);

    VkDescriptorImageInfo perMaterialAlbedoInfo = cassidy::init::descriptorImageInfo(
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_backpackAlbedo.getImageView(), m_linearSampler);
    VkDescriptorImageInfo perMaterialSpecularInfo = cassidy::init::descriptorImageInfo(
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_backpackSpecular.getImageView(), m_linearSampler);
    VkDescriptorImageInfo perMaterialNormalInfo = cassidy::init::descriptorImageInfo(
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_backpackNormal.getImageView(), m_linearSampler);

    cassidy::DescriptorBuilder::begin(&cassidy::globals::g_descAllocator, &cassidy::globals::g_descLayoutCache)
      .bindBuffer(0, &perPassBufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
      .build(m_frameData[i].perPassSet, m_perPassSetLayout);

    cassidy::DescriptorBuilder::begin(&cassidy::globals::g_descAllocator, &cassidy::globals::g_descLayoutCache)
      .bindBuffer(0, &perObjectBufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT)
      .bindImage(1, &perObjectImageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
      .build(m_frameData[i].perObjectSet, m_perObjectSetLayout);

    cassidy::DescriptorBuilder::begin(&cassidy::globals::g_descAllocator, &cassidy::globals::g_descLayoutCache)
      .bindImage(0, &perMaterialAlbedoInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
      .bindImage(1, &perMaterialSpecularInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
      .bindImage(2, &perMaterialNormalInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
      .build(m_frameData[i].m_backpackMaterialSet, m_perMaterialSetLayout);
  }

  m_deletionQueue.addFunction([=]() {
    cassidy::globals::g_descLayoutCache.release();
    cassidy::globals::g_descAllocator.release();
    });
}

void cassidy::Renderer::initVertexBuffers()
{
  m_triangleMesh.allocateVertexBuffers(m_uploadContext.uploadCommandBuffer, m_allocator, this);
  m_backpackMesh.allocateVertexBuffers(m_uploadContext.uploadCommandBuffer, m_allocator, this);

  m_deletionQueue.addFunction([=]() {
    m_triangleMesh.release(m_device, m_allocator);
    m_backpackMesh.release(m_device, m_allocator);
  });

  std::cout << "Created vertex buffers!\n" << std::endl;
}

void cassidy::Renderer::initIndexBuffers()
{
  m_backpackMesh.allocateIndexBuffers(m_uploadContext.uploadCommandBuffer, m_allocator, this);
  m_triangleMesh.allocateIndexBuffers(m_uploadContext.uploadCommandBuffer, m_allocator, this);
}

void cassidy::Renderer::initUniformBuffers()
{
  const uint32_t objectBufferSize = FRAMES_IN_FLIGHT * cassidy::helper::padUniformBufferSize(sizeof(PerObjectData),
    m_physicalDeviceProperties);
  m_perObjectUniformBufferDynamic = allocateBuffer(objectBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

  m_deletionQueue.addFunction([=]() {
    vmaDestroyBuffer(m_allocator, m_perObjectUniformBufferDynamic.buffer, m_perObjectUniformBufferDynamic.allocation);
  });

  for (uint8_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
  {
    m_frameData[i].perPassUniformBuffer = allocateBuffer(sizeof(PerPassData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
    
    m_deletionQueue.addFunction([=]() {
      vmaDestroyBuffer(m_allocator, m_frameData[i].perPassUniformBuffer.buffer, m_frameData[i].perPassUniformBuffer.allocation);
    });
  }
}


void cassidy::Renderer::initImGui()
{
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

  cassidy::helper::immediateSubmit(m_device, m_uploadContext,
    [&](VkCommandBuffer cmd) {
    ImGui_ImplVulkan_CreateFontsTexture(cmd);
  });

  // Create sampler used for swapchain image in viewport:
  m_viewportSampler = cassidy::helper::createTextureSampler(m_device, m_physicalDeviceProperties,
    VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_FALSE);

  initViewportCommandPool();
  initViewportCommandBuffers();
  initViewportImages();
  initViewportRenderPass();
  initViewportFramebuffers();

  // Solution for full ImGui setup w/ viewport: https://github.com/ocornut/imgui/issues/5110

  ImGui_ImplVulkan_DestroyFontUploadObjects();

  m_deletionQueue.addFunction([&, imGuiPool]() {
    for (auto fb : m_viewportFramebuffers)
      vkDestroyFramebuffer(m_device, fb, nullptr);
   
    for (uint32_t i = 0; i < m_viewportImages.size(); ++i)
    {
      vmaDestroyImage(m_allocator, m_viewportImages[i].image, 
        m_viewportImages[i].allocation);
      vkDestroyImageView(m_device, m_viewportImages[i].view, nullptr);
    }

    vmaDestroyImage(m_allocator, m_viewportDepthImage.image,
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

  std::cout << "ImGui initialised!\n" << std::endl;
}

void cassidy::Renderer::initViewportImages()
{
  m_viewportImages.resize(m_swapchain.images.size());

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
      .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VmaAllocationCreateInfo allocInfo = cassidy::init::vmaAllocationCreateInfo(
      VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);

    vmaCreateImage(m_allocator, &imageInfo, &allocInfo, &m_viewportImages[i].image,
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

  vmaCreateImage(m_allocator, &depthImageInfo, &vmaAllocInfo,
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

  m_viewportDescSets.resize(m_swapchain.imageViews.size());
  for (uint32_t i = 0; i < m_viewportImages.size(); ++i)
  {
    m_viewportDescSets[i] = ImGui_ImplVulkan_AddTexture(m_viewportSampler,
      m_viewportImages[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  }

  // (Deletion of viewport resources is handled in ImGui deletion queue function)
}

void cassidy::Renderer::initViewportRenderPass()
{
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

  std::cout << "Created viewport render pass!" << std::endl;
}

void cassidy::Renderer::initViewportCommandPool()
{
  QueueFamilyIndices indices = cassidy::helper::findQueueFamilies(m_physicalDevice, m_engineRef->getSurface());

  VkCommandPoolCreateInfo info = cassidy::init::commandPoolCreateInfo(
    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, indices.graphicsFamily.value());

  VK_CHECK(vkCreateCommandPool(m_device, &info, nullptr, &m_viewportCommandPool));
}

void cassidy::Renderer::initViewportCommandBuffers()
{
  m_viewportCommandBuffers.resize(FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocInfo = cassidy::init::commandBufferAllocInfo(
    m_viewportCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, FRAMES_IN_FLIGHT);

  vkAllocateCommandBuffers(m_device, &allocInfo, m_viewportCommandBuffers.data());
}

void cassidy::Renderer::initViewportFramebuffers()
{
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
}

void cassidy::Renderer::transitionSwapchainImages()
{
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
}

void cassidy::Renderer::rebuildSwapchain()
{
  int width;
  int height;

  SDL_Window* window = m_engineRef->getWindow();
  SDL_GetWindowSize(window, &width, &height);

  vkWaitForFences(m_device,static_cast<uint32_t>(m_inFlightFences.size()), m_inFlightFences.data(), VK_TRUE, UINT64_MAX);
  m_swapchain.release(m_device, m_allocator);

  // Release ImGui viewport resources:
  for (const auto& f : m_viewportFramebuffers)
    vkDestroyFramebuffer(m_device, f, nullptr);
  for (const auto& i : m_viewportImages)
    vmaDestroyImage(m_allocator, i.image, i.allocation);
  vmaDestroyImage(m_allocator, m_viewportDepthImage.image, m_viewportDepthImage.allocation);
  for (const auto& i : m_viewportImages)
    vkDestroyImageView(m_device, i.view, nullptr);
  vkDestroyImageView(m_device, m_viewportDepthImage.view, nullptr);

  for (size_t i = 0; i < m_viewportDescSets.size(); ++i)
    ImGui_ImplVulkan_RemoveTexture(m_viewportDescSets[i]);

  initSwapchain();
  initSwapchainFramebuffers();
  initViewportImages();
  initViewportFramebuffers();
}
