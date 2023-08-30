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
  uint32_t queueCreateInfoCount, const VkPhysicalDeviceFeatures* deviceFeatures, uint32_t extensionCount, 
  const char* const* extensionNames, uint32_t layerCount, const char* const* layerNames)
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

VkImageCreateInfo cassidy::init::imageCreateInfo(VkImageType type, VkExtent3D extent3D, uint8_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usageFlags)
{
  VkImageCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  info.imageType = type;
  info.extent.width = extent3D.width;
  info.extent.height = extent3D.height;
  info.extent.depth = extent3D.depth;
  info.mipLevels = mipLevels;
  info.arrayLayers = 1;

  info.format = format;
  info.tiling = tiling;

  info.usage = usageFlags;
  info.samples = VK_SAMPLE_COUNT_1_BIT;
  info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  return info;
}

VkImageViewCreateInfo cassidy::init::imageViewCreateInfo(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags,
  uint8_t mipLevels)
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

VmaAllocationCreateInfo cassidy::init::vmaAllocationCreateInfo(VmaMemoryUsage usageFlags, VkMemoryPropertyFlags memoryFlags)
{
  VmaAllocationCreateInfo info = {};
  info.usage = usageFlags;
  info.requiredFlags = memoryFlags;

  return info;
}

VkShaderModuleCreateInfo cassidy::init::shaderModuleCreateInfo(size_t codeSize, uint32_t* code)
{
  VkShaderModuleCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  info.codeSize = codeSize;
  info.pCode = code;

  return info;
}

VkShaderModuleCreateInfo cassidy::init::shaderModuleCreateInfo(const SpirvShaderCode& code)
{
  VkShaderModuleCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  info.codeSize = code.codeSize;
  info.pCode = code.codeBuffer;

  return info;
}

VkPipelineShaderStageCreateInfo cassidy::init::pipelineShaderStageCreateInfo(VkShaderStageFlagBits stage,
  VkShaderModule module)
{
  VkPipelineShaderStageCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  info.stage = stage;
  info.module = module;
  info.pName = "main";
  info.pSpecializationInfo = nullptr;

  return info;
}

VkPipelineVertexInputStateCreateInfo cassidy::init::pipelineVertexInputStateCreateInfo(uint32_t numBindingDescriptions,
  VkVertexInputBindingDescription* bindingDescriptions, uint32_t numVertexAttDescriptions, 
  VkVertexInputAttributeDescription* vertexAttDescriptions)
{
  VkPipelineVertexInputStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  info.vertexBindingDescriptionCount = numBindingDescriptions;
  info.pVertexBindingDescriptions = bindingDescriptions;
  info.vertexAttributeDescriptionCount = numVertexAttDescriptions;
  info.pVertexAttributeDescriptions = vertexAttDescriptions;
  
  return info;
}

VkPipelineInputAssemblyStateCreateInfo cassidy::init::pipelineInputAssemblyStateCreateInfo(VkPrimitiveTopology topology)
{
  VkPipelineInputAssemblyStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  info.topology = topology;
  info.primitiveRestartEnable = VK_FALSE;

  return info;
}

VkPipelineDynamicStateCreateInfo cassidy::init::pipelineDynamicStateCreateinfo(uint32_t numDynamicStates,
  VkDynamicState* dynamicStates)
{
  VkPipelineDynamicStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  info.dynamicStateCount = numDynamicStates;
  info.pDynamicStates = dynamicStates;

  return info;
}

VkPipelineRasterizationStateCreateInfo cassidy::init::pipelineRasterizationStateCreateInfo(VkPolygonMode polygonMode, 
  VkCullModeFlags cullFlags)
{
  VkPipelineRasterizationStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  info.depthClampEnable = VK_FALSE;
  info.rasterizerDiscardEnable = VK_FALSE;
  info.polygonMode = polygonMode;
  info.lineWidth = 1.0f;
  info.cullMode = cullFlags;
  info.frontFace = VK_FRONT_FACE_CLOCKWISE;
  info.depthBiasEnable = VK_FALSE;
  info.depthBiasConstantFactor = 0.0f;
  info.depthBiasClamp = 0.0f;
  info.depthBiasSlopeFactor = 0.0f;

  return info;
}

VkPipelineMultisampleStateCreateInfo cassidy::init::pipelineMultisampleStateCreateInfo(VkSampleCountFlagBits sampleCount)
{
  VkPipelineMultisampleStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  info.sampleShadingEnable = VK_FALSE;
  info.rasterizationSamples = sampleCount;
  info.minSampleShading = 1.0f;
  info.pSampleMask = nullptr;
  info.alphaToCoverageEnable = VK_FALSE;
  info.alphaToOneEnable = VK_FALSE;

  return info;
}

VkPipelineColorBlendAttachmentState cassidy::init::pipelineColorBlendAttachmentState(VkColorComponentFlags colourWriteMask, 
  VkBool32 enableBlend, VkBlendFactor srcBlendFactor, VkBlendFactor dstBlendFactor, VkBlendOp blendOp)
{
  VkPipelineColorBlendAttachmentState state = {};
  state.colorWriteMask = colourWriteMask;
  state.blendEnable = enableBlend;
  state.srcColorBlendFactor = srcBlendFactor;
  state.dstColorBlendFactor = dstBlendFactor;
  state.colorBlendOp = blendOp;
  state.srcAlphaBlendFactor = srcBlendFactor;
  state.dstAlphaBlendFactor = dstBlendFactor;
  state.alphaBlendOp = blendOp;

  return state;
}

VkPipelineColorBlendStateCreateInfo cassidy::init::pipelineColorBlendStateCreateInfo(uint32_t numAttachments,
  VkPipelineColorBlendAttachmentState* attachments, float blendConstant0, float blendConstant1, float blendConstant2,
  float blendConstant3)
{
  VkPipelineColorBlendStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  info.logicOpEnable = VK_FALSE;
  info.logicOp = VK_LOGIC_OP_COPY;
  info.attachmentCount = numAttachments;
  info.pAttachments = attachments;
  info.blendConstants[0] = blendConstant0;
  info.blendConstants[1] = blendConstant1;
  info.blendConstants[2] = blendConstant2;
  info.blendConstants[3] = blendConstant3;

  return info;
}

VkPipelineDepthStencilStateCreateInfo cassidy::init::pipelineDepthStencilStateCreateInfo(VkBool32 enableDepthTest, 
  VkBool32 enableDepthWrite, VkCompareOp depthCompareOp)
{
  VkPipelineDepthStencilStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  info.depthTestEnable = enableDepthTest;
  info.depthWriteEnable = enableDepthWrite;
  info.depthCompareOp = depthCompareOp;
  info.depthBoundsTestEnable = VK_FALSE;
  info.minDepthBounds = 0.0f;
  info.maxDepthBounds = 1.0f;
  info.stencilTestEnable = VK_FALSE;
  info.front = {};
  info.back = {};

  return info;
}

VkViewport cassidy::init::viewport(float x, float y, float width, float height)
{
  VkViewport viewport = {};
  viewport.x = x;
  viewport.y = y;
  viewport.width = width;
  viewport.height = height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  return viewport;
}

VkPipelineViewportStateCreateInfo cassidy::init::pipelineViewportStateCreateInfo(uint32_t numViewports, 
  VkViewport* viewports, uint32_t numScissors, VkRect2D* scissors)
{
  VkPipelineViewportStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  info.viewportCount = numViewports;
  info.pViewports = viewports;
  info.scissorCount = numScissors;
  info.pScissors = scissors;

  return info;
}

VkRect2D cassidy::init::scissor(VkOffset2D offset, VkExtent2D extent)
{
  VkRect2D rect = {};
  rect.offset = offset;
  rect.extent = extent;

  return rect;
}

VkPipelineLayoutCreateInfo cassidy::init::pipelineLayoutCreateInfo(uint32_t numDescSetLayouts,
  VkDescriptorSetLayout* descSetLayouts, uint32_t numPushConstantRanges, VkPushConstantRange* pushConstantRanges)
{
  VkPipelineLayoutCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  info.setLayoutCount = numDescSetLayouts;
  info.pSetLayouts = descSetLayouts;
  info.pushConstantRangeCount = numPushConstantRanges;
  info.pPushConstantRanges = pushConstantRanges;

  return info;
}

VkGraphicsPipelineCreateInfo cassidy::init::graphicsPipelineCreateInfo(uint32_t numShaderStages,
  VkPipelineShaderStageCreateInfo* shaderStages, VkPipelineVertexInputStateCreateInfo* vertexInput,
  VkPipelineInputAssemblyStateCreateInfo* inputAssembly,  VkPipelineViewportStateCreateInfo* viewportState, 
  VkPipelineRasterizationStateCreateInfo* rasteriser, VkPipelineMultisampleStateCreateInfo* multisampling, 
  VkPipelineDepthStencilStateCreateInfo* depthStencil, VkPipelineColorBlendStateCreateInfo* colourBlendState, 
  VkPipelineDynamicStateCreateInfo* dynamicState, VkPipelineLayout pipelineLayout, 
  VkRenderPass renderPass, uint32_t subpass)
{
  VkGraphicsPipelineCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  info.stageCount = numShaderStages;
  info.pStages = shaderStages;
  info.pVertexInputState = vertexInput;
  info.pInputAssemblyState = inputAssembly;
  info.pViewportState = viewportState;
  info.pRasterizationState = rasteriser;
  info.pMultisampleState = multisampling;
  info.pColorBlendState = colourBlendState;
  info.pDepthStencilState = depthStencil;
  info.pDynamicState = dynamicState;

  info.layout = pipelineLayout;
  info.renderPass = renderPass;
  info.subpass = subpass;

  return info;
}

VkAttachmentDescription cassidy::init::attachmentDescription(VkFormat format, VkSampleCountFlagBits samples, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp, VkImageLayout initialLayout, VkImageLayout finalLayout)
{
  VkAttachmentDescription desc = {};
  desc.format = format;
  desc.samples = samples;
  desc.loadOp = loadOp;
  desc.storeOp = storeOp;
  desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  desc.initialLayout = initialLayout;
  desc.finalLayout = finalLayout;

  return desc;
}

VkAttachmentReference cassidy::init::attachmentReference(uint32_t index, VkImageLayout layout)
{
  VkAttachmentReference ref = {};
  ref.attachment = index;
  ref.layout = layout;

  return ref;
}

VkSubpassDescription cassidy::init::subpassDescription(VkPipelineBindPoint bindPoint, uint32_t numColourAttachments, VkAttachmentReference* colourAttachments, VkAttachmentReference* depthStencilAttachment)
{
  VkSubpassDescription desc = {};
  desc.pipelineBindPoint = bindPoint;
  desc.colorAttachmentCount = numColourAttachments;
  desc.pColorAttachments = colourAttachments;
  desc.pDepthStencilAttachment = depthStencilAttachment;

  return desc;
}

VkSubpassDependency cassidy::init::subpassDependency(uint32_t srcSubpass, uint32_t dstSubpass, VkPipelineStageFlags srcStageMask, VkAccessFlagBits srcAccessMask, VkPipelineStageFlags dstStageMask, VkAccessFlags dstAccessMask)
{
  VkSubpassDependency dependency = {};
  dependency.srcSubpass = srcSubpass;
  dependency.dstSubpass = dstSubpass;
  dependency.srcStageMask = srcStageMask;
  dependency.dstStageMask = dstStageMask;
  dependency.srcAccessMask = srcAccessMask;
  dependency.dstAccessMask = dstAccessMask;

  return dependency;
}

VkRenderPassCreateInfo cassidy::init::renderPassCreateInfo(uint32_t numAttachments, VkAttachmentDescription* attachments, uint32_t numSubpasses, VkSubpassDescription* subpasses, uint32_t numSubpassDependencies, VkSubpassDependency* subpassDependencies)
{
  VkRenderPassCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  info.attachmentCount = numAttachments;
  info.pAttachments = attachments;
  info.subpassCount = numSubpasses;
  info.pSubpasses = subpasses;
  info.dependencyCount = numSubpassDependencies;
  info.pDependencies = subpassDependencies;

  return info;
}

VkCommandPoolCreateInfo cassidy::init::commandPoolCreateInfo(VkCommandPoolCreateFlags flags, uint32_t queueFamilyIndex)
{
  VkCommandPoolCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  info.flags = flags;
  info.queueFamilyIndex = queueFamilyIndex;

  return info;
}

VkCommandBufferAllocateInfo cassidy::init::commandBufferAllocInfo(const VkCommandPool& pool, VkCommandBufferLevel level, uint32_t numBuffers)
{
  VkCommandBufferAllocateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  info.commandPool = pool;
  info.level = level;
  info.commandBufferCount = numBuffers;
  
  return info;
}

VkFramebufferCreateInfo cassidy::init::framebufferCreateInfo(const VkRenderPass& renderPass, uint32_t numAttachments, VkImageView* attachments, VkExtent2D extent)
{
  VkFramebufferCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  info.renderPass = renderPass;
  info.attachmentCount = numAttachments;
  info.pAttachments = attachments;
  info.width = extent.width;
  info.height = extent.height;
  info.layers = 1;

  return info;
}

VkSemaphoreCreateInfo cassidy::init::semaphoreCreateInfo(VkSemaphoreCreateFlags flags)
{
  VkSemaphoreCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  info.flags = flags;

  return info;
}

VkFenceCreateInfo cassidy::init::fenceCreateInfo(VkFenceCreateFlags flags)
{
  VkFenceCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  info.flags = flags;
  
  return info;
}

VkCommandBufferBeginInfo cassidy::init::commandBufferBeginInfo(VkCommandBufferUsageFlags flags, const VkCommandBufferInheritanceInfo* inheritanceInfo)
{
  VkCommandBufferBeginInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  info.flags = flags;
  info.pInheritanceInfo = inheritanceInfo;

  return info;
}

VkRenderPassBeginInfo cassidy::init::renderPassBeginInfo(VkRenderPass renderPass, VkFramebuffer framebuffer, VkOffset2D offset, VkExtent2D extent, uint8_t numClearValues, VkClearValue* clearValues)
{
  VkRenderPassBeginInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  info.renderPass = renderPass;
  info.framebuffer = framebuffer;
  
  info.renderArea.offset = offset;
  info.renderArea.extent = extent;

  info.clearValueCount = numClearValues;
  info.pClearValues = clearValues;
  
  return info;
}

VkSubmitInfo cassidy::init::submitInfo(uint32_t numWaitSemaphores, VkSemaphore* waitSemaphores, VkPipelineStageFlags* waitDstStageMask, uint32_t numSignalSemaphores, VkSemaphore* signalSemaphores, uint32_t numCommandBuffers, VkCommandBuffer* commandBuffers)
{
  VkSubmitInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  info.waitSemaphoreCount = numWaitSemaphores;
  info.pWaitSemaphores = waitSemaphores;
  info.pWaitDstStageMask = waitDstStageMask;
  info.signalSemaphoreCount = numSignalSemaphores;
  info.pSignalSemaphores = signalSemaphores;

  info.commandBufferCount = numCommandBuffers;
  info.pCommandBuffers = commandBuffers;

  return info;
}

VkPresentInfoKHR cassidy::init::presentInfo(uint32_t numWaitSemaphores, VkSemaphore* waitSemaphores, uint32_t numSwapchains, VkSwapchainKHR* swapchains, uint32_t* imageIndices)
{
  VkPresentInfoKHR info = {};
  info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  info.waitSemaphoreCount = numWaitSemaphores;
  info.pWaitSemaphores = waitSemaphores;
  
  info.swapchainCount = numSwapchains;
  info.pSwapchains = swapchains;
  info.pImageIndices = imageIndices;
  info.pResults = nullptr;

  return info;
}
