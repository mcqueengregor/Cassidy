#pragma once

#include "Utils/Types.h"

namespace cassidy::init
{
  // Application info:
  VkApplicationInfo applicationInfo(
    const char* appName, 
    uint8_t engineVersionVariant, 
    uint8_t engineVersionMaj,
    uint8_t engineVersionMin, 
    uint8_t engineVersionPatch, 
    uint32_t apiVersion
  );

  // Instance create info:
  VkInstanceCreateInfo instanceCreateInfo(
    VkApplicationInfo* appInfo, 
    uint32_t numExtensions,
    const char* const* extensionNames, 
    uint32_t numLayers = 0, 
    const char* const* layerNames = nullptr,
    VkDebugUtilsMessengerCreateInfoEXT* debugCreateInfo = nullptr
  );

  // Debug messenger create info:
  VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo(
    VkDebugUtilsMessageSeverityFlagBitsEXT severityFlags,
    VkDebugUtilsMessageTypeFlagsEXT typeFlags,
    PFN_vkDebugUtilsMessengerCallbackEXT callbackFunc
  );

  // Logical device + queue create infos:
  VkDeviceQueueCreateInfo deviceQueueCreateInfo(
    uint32_t queueFamilyIndex, 
    uint32_t queueCount, 
    float* queuePriority
  );
  VkDeviceCreateInfo deviceCreateInfo(
    const VkDeviceQueueCreateInfo* queueCreateInfos,
    uint32_t queueCreateInfoCount,
    const VkPhysicalDeviceFeatures* deviceFeatures, 
    uint32_t extensionCount, 
    const char* const* extensionNames, 
    uint32_t layerCount = 0, 
    const char* const* layerNames = nullptr
  );

  // Swapchain create info:
  VkSwapchainCreateInfoKHR swapchainCreateInfo(
    SwapchainSupportDetails details, 
    QueueFamilyIndices indices,
    VkSurfaceKHR surface,
    VkSurfaceFormatKHR surfaceFormat, 
    VkPresentModeKHR presentMode, 
    VkExtent2D extent,
    VkImageUsageFlags usageFlags,
    uint32_t numQueueFamilyIndices,
    uint32_t* const queueFamilyIndices
  );

  // Image and image view create infos:
  VkImageCreateInfo imageCreateInfo(
    VkImageType type,
    VkExtent3D extent3D,
    uint8_t mipLevels,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usageFlags
  );
  VkImageViewCreateInfo imageViewCreateInfo(
    VkImage image,
    VkFormat format, 
    VkImageAspectFlags aspectFlags, 
    uint8_t mipLevels
  );

  // VMA memory allocation create info:
  VmaAllocationCreateInfo vmaAllocationCreateInfo(
    VmaMemoryUsage usageFlags,
    VmaAllocationCreateFlagBits allocFlags
  );

  // Shader module create info:
  VkShaderModuleCreateInfo shaderModuleCreateInfo(
    size_t codeSize, 
    uint32_t* code
  );
  VkShaderModuleCreateInfo shaderModuleCreateInfo(
    const SpirvShaderCode& code
  );

  // Pipeline create infos:
  VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo(
    VkShaderStageFlagBits stage,
    VkShaderModule module
  );
  VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo(
    uint32_t numBindingDescriptions,
    VkVertexInputBindingDescription* bindingDescriptions,
    uint32_t numVertexAttDescriptions,
    VkVertexInputAttributeDescription* vertexAttDescriptions
  );
  VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo(
    VkPrimitiveTopology topology
  );
  VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateinfo(
    uint32_t numDynamicStates,
    VkDynamicState* dynamicStates
  );
  VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo(
    VkPolygonMode polygonMode,
    VkCullModeFlags cullFlags
  );
  VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo(
    VkSampleCountFlagBits sampleCount
    );
  VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState(
    VkColorComponentFlags colourWriteMask,
    VkBool32 enableBlend,
    VkBlendFactor srcBlendFactor,
    VkBlendFactor dstBlendFactor,
    VkBlendOp blendOp
  );
  VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo(
    uint32_t numAttachments,
    VkPipelineColorBlendAttachmentState* attachments,
    float blendConstant0,
    float blendConstant1,
    float blendConstant2,
    float blendConstant3
  );
  VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo(
    VkBool32 enableDepthTest,
    VkBool32 enableDepthWrite,
    VkCompareOp depthCompareOp
  );
  VkViewport viewport(
    float x, 
    float y, 
    float width, 
    float height
  );
  VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo(
    uint32_t numViewports,
    VkViewport* viewports,
    uint32_t numScissors,
    VkRect2D* scissors
  );
  VkRect2D scissor(
    VkOffset2D offset, 
    VkExtent2D extent
  );
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo(
    uint32_t numDescSetLayouts,
    VkDescriptorSetLayout* descSetLayouts,
    uint32_t numPushConstantRanges,
    VkPushConstantRange* pushConstantRanges
  );
  VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo(
    uint32_t numShaderStages,
    VkPipelineShaderStageCreateInfo* shaderStages,
    VkPipelineVertexInputStateCreateInfo* vertexInput,
    VkPipelineInputAssemblyStateCreateInfo* inputAssembly,
    VkPipelineViewportStateCreateInfo* viewportState,
    VkPipelineRasterizationStateCreateInfo* rasteriser,
    VkPipelineMultisampleStateCreateInfo* multisampling,
    VkPipelineDepthStencilStateCreateInfo* depthStencil,
    VkPipelineColorBlendStateCreateInfo* colourBlendState,
    VkPipelineDynamicStateCreateInfo* dynamicState,
    VkPipelineLayout pipelineLayout,
    VkRenderPass renderPass,
    uint32_t subpass
  );

  // Push constant range:
  VkPushConstantRange pushConstantRange(
    VkShaderStageFlags stageFlags,
    uint32_t offset,
    uint32_t size
    );

  // Render pass create infos:
  VkAttachmentDescription attachmentDescription(VkFormat format, 
    VkSampleCountFlagBits samples, 
    VkAttachmentLoadOp loadOp,
    VkAttachmentStoreOp storeOp, 
    VkImageLayout initialLayout,
    VkImageLayout finalLayout
  );
  VkAttachmentReference attachmentReference(
    uint32_t index,
    VkImageLayout layout
  );
  VkSubpassDescription subpassDescription(
    VkPipelineBindPoint bindPoint,
    uint32_t numColourAttachments,
    VkAttachmentReference* colourAttachments,
    VkAttachmentReference* depthStencilAttachment
  );
  VkSubpassDependency subpassDependency(
    uint32_t srcSubpass,
    uint32_t dstSubpass,
    VkPipelineStageFlags srcStageMask,
    VkAccessFlagBits srcAccessMask,
    VkPipelineStageFlags dstStageMask,
    VkAccessFlags dstAccessMask
  );
  VkRenderPassCreateInfo renderPassCreateInfo(
    uint32_t numAttachments,
    VkAttachmentDescription* attachments,
    uint32_t numSubpasses,
    VkSubpassDescription* subpasses,
    uint32_t numSubpassDependencies,
    VkSubpassDependency* subpassDependencies
  );

  // Command buffer/pool create infos:
  VkCommandPoolCreateInfo commandPoolCreateInfo(
    VkCommandPoolCreateFlags flags,
    uint32_t queueFamilyIndex
  );
  VkCommandBufferAllocateInfo commandBufferAllocInfo(
    const VkCommandPool& pool,
    VkCommandBufferLevel level,
    uint32_t numBuffers
  );

  // Framebuffer create info:
  VkFramebufferCreateInfo framebufferCreateInfo(
    const VkRenderPass& renderPass,
    uint32_t numAttachments,
    VkImageView* attachments,
    VkExtent2D extent
  );

  // Semaphore and fence create infos:
  VkSemaphoreCreateInfo semaphoreCreateInfo(
    VkSemaphoreCreateFlags flags
  );
  VkFenceCreateInfo fenceCreateInfo(
    VkFenceCreateFlags flags
  );

  // Command structs:
  VkCommandBufferBeginInfo commandBufferBeginInfo(
    VkCommandBufferUsageFlags flags,
    const VkCommandBufferInheritanceInfo* inheritanceInfo
  );
  VkRenderPassBeginInfo renderPassBeginInfo(
    VkRenderPass renderPass,
    VkFramebuffer framebuffer,
    VkOffset2D offset,
    VkExtent2D extent,
    uint8_t numClearValues,
    VkClearValue* clearValues
  );

  // Command submit/present structs:
  VkSubmitInfo submitInfo(
    uint32_t numWaitSemaphores,
    VkSemaphore* waitSemaphores,
    VkPipelineStageFlags* waitDstStageMask,
    uint32_t numSignalSemaphores,
    VkSemaphore* signalSemaphores,
    uint32_t numCommandBuffers,
    VkCommandBuffer* commandBuffers
  );
  VkPresentInfoKHR presentInfo(
    uint32_t numWaitSemaphores,
    VkSemaphore* waitSemaphores,
    uint32_t numSwapchains,
    VkSwapchainKHR* swapchains,
    uint32_t* imageIndices
  );

  // Buffer create infos:
  VkBufferCreateInfo bufferCreateInfo(
    uint32_t size,
    VkBufferUsageFlags usage
  );
 
  // Descriptor structs:
  VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(
    uint32_t bindingIndex,
    VkDescriptorType descriptorType,
    uint32_t numDescriptors,
    VkShaderStageFlags stageFlags,
    const VkSampler* immutableSamplers
  );
  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(
    uint32_t numBindings,
    const VkDescriptorSetLayoutBinding* bindings
  );
  VkDescriptorPoolCreateInfo descriptorPoolCreateInfo(
    uint32_t numPoolSizes,
    const VkDescriptorPoolSize* poolSizes,
    uint32_t maxSets
  );
  VkDescriptorSetAllocateInfo descriptorSetAllocateInfo(
    VkDescriptorPool descriptorPool,
    uint32_t numDescriptorSets,
    const VkDescriptorSetLayout* setLayouts
  );
  VkDescriptorBufferInfo descriptorBufferInfo(
    VkBuffer bufferHandle,
    uint32_t offset,
    uint32_t range
  );
  VkDescriptorImageInfo descriptorImageInfo(
    VkImageLayout imageLayout,
    VkImageView imageView,
    VkSampler sampler
  );
  VkWriteDescriptorSet writeDescriptorSet(
    VkDescriptorSet dstSet,
    uint32_t dstBinding,
    VkDescriptorType descriptorType,
    uint32_t numDescriptors,
    const VkDescriptorBufferInfo* bufferInfo
  );
  VkWriteDescriptorSet writeDescriptorSet(
    VkDescriptorSet dstSet,
    uint32_t dstBinding,
    VkDescriptorType descriptorType,
    uint32_t numDescriptors,
    const VkDescriptorImageInfo* imageInfo
  );
}