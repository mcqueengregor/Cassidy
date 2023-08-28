#pragma once

#include <vulkan/vulkan.h>
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
    VkImageUsageFlags usageFlags
  );

  // Image view create info:
  VkImageViewCreateInfo imageViewCreateInfo(
    VkImage image,
    VkFormat format, 
    VkImageAspectFlags aspectFlags, 
    uint8_t mipLevels
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
    VkPipelineViewportStateCreateInfo* viewportState,
    VkPipelineRasterizationStateCreateInfo* rasteriser,
    VkPipelineMultisampleStateCreateInfo* multisampling,
    VkPipelineDepthStencilStateCreateInfo* depthStencil,
    VkPipelineColorBlendStateCreateInfo* colorBlendState,
    VkPipelineDynamicStateCreateInfo* dynamicState,
    VkPipelineLayout pipelineLayout,
    VkRenderPass renderPass,
    uint32_t subpass
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
}