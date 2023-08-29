#include "Pipeline.h"
#include "Core/Renderer.h"
#include "Utils/Initialisers.h"
#include "Utils/Helpers.h"
#include "Utils/Types.h"

#include <fstream>
#include <iostream>

cassidy::Pipeline::Pipeline(cassidy::Renderer* renderer, const std::string& vertexFilepath, const std::string& fragmentFilepath)
{
  init(renderer, vertexFilepath, fragmentFilepath);
}

void cassidy::Pipeline::init(cassidy::Renderer* renderer, const std::string& vertexFilepath, const std::string& fragmentFilepath)
{
  m_rendererRef = renderer;

  initRenderPass();
  initGraphicsPipeline(vertexFilepath, fragmentFilepath);
}

void cassidy::Pipeline::release()
{
  vkDestroyPipeline(m_rendererRef->getLogicalDevice(), m_graphicsPipeline, nullptr);
  vkDestroyPipelineLayout(m_rendererRef->getLogicalDevice(), m_pipelineLayout, nullptr);
  vkDestroyRenderPass(m_rendererRef->getLogicalDevice(), m_renderPass, nullptr);
}

SpirvShaderCode cassidy::Pipeline::loadSpirv(const std::string& filepath)
{
  std::ifstream file(filepath, std::ios::ate | std::ios::binary);

  if (!file.is_open())
  {
    std::cout << "ERROR: Could not load SPIR-V file! (" << filepath << ")\n" << std::endl;
    return { 0, nullptr };
  }

  size_t fileSize = static_cast<size_t>(file.tellg());
  char* buffer = new char[fileSize];

  file.seekg(0);
  file.read(buffer, fileSize);
  file.close();

  return { fileSize, reinterpret_cast<uint32_t*>(buffer) };
}

void cassidy::Pipeline::initRenderPass()
{
  VkAttachmentDescription colourAttachment = cassidy::init::attachmentDescription(m_rendererRef->getSwapchain().imageFormat,
    VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  VkFormat depthFormatCandidates[] = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
  const VkFormat depthFormat = cassidy::helper::findSupportedFormat(m_rendererRef->getPhysicalDevice(), 3, depthFormatCandidates,
    VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

  VkAttachmentDescription depthAttachment = cassidy::init::attachmentDescription(depthFormat, VK_SAMPLE_COUNT_1_BIT,
    VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

  VkAttachmentReference colourAttachmentRef = cassidy::init::attachmentReference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  VkAttachmentReference depthAttachmentRef = cassidy::init::attachmentReference(1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

  VkSubpassDescription subpass = cassidy::init::subpassDescription(VK_PIPELINE_BIND_POINT_GRAPHICS, 1, 
    &colourAttachmentRef, &depthAttachmentRef);

  VkSubpassDependency dependency = cassidy::init::subpassDependency(VK_SUBPASS_EXTERNAL, 0, 
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 
    VK_ACCESS_NONE,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 
    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);

  VkAttachmentDescription attachments[] = { colourAttachment, depthAttachment };

  VkRenderPassCreateInfo renderPassInfo = cassidy::init::renderPassCreateInfo(2, attachments, 1, &subpass, 1, &dependency);

  vkCreateRenderPass(m_rendererRef->getLogicalDevice(), &renderPassInfo, nullptr, &m_renderPass);

  std::cout << "Created pipeline render pass!" << std::endl;
}

void cassidy::Pipeline::initGraphicsPipeline(const std::string& vertexFilepath, const std::string& fragmentFilepath)
{
  SpirvShaderCode vertexCode = loadSpirv(vertexFilepath);
  SpirvShaderCode fragmentCode = loadSpirv(fragmentFilepath);

  // If either shader couldn't be loaded, early-out (structs automatically delete heap-allocated binaries):
  if (!vertexCode.codeBuffer || !fragmentCode.codeBuffer)
    return;

  VkShaderModuleCreateInfo vertexModuleInfo = cassidy::init::shaderModuleCreateInfo(vertexCode);
  VkShaderModuleCreateInfo fragmentModuleInfo = cassidy::init::shaderModuleCreateInfo(fragmentCode);

  VkShaderModule vertexModule;
  VkShaderModule fragmentModule;

  vkCreateShaderModule(m_rendererRef->getLogicalDevice(), &vertexModuleInfo, nullptr, &vertexModule);
  vkCreateShaderModule(m_rendererRef->getLogicalDevice(), &fragmentModuleInfo, nullptr, &fragmentModule);

  // Build pipeline:
  VkPipelineShaderStageCreateInfo shaderStages[] = {
    cassidy::init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexModule),
    cassidy::init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentModule)
  };

  auto bindingDescription = Vertex::getBindingDesc();
  auto attributeDescriptions = Vertex::getAttributeDescs();

  VkPipelineVertexInputStateCreateInfo vertexInput = cassidy::init::pipelineVertexInputStateCreateInfo(1,
    &bindingDescription, 3, attributeDescriptions.data());

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = cassidy::init::pipelineInputAssemblyStateCreateInfo
  (VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

  VkViewport viewport = cassidy::init::viewport(0.0f, 0.0f, 
    m_rendererRef->getSwapchain().extent.width, m_rendererRef->getSwapchain().extent.height);

  VkRect2D scissor = cassidy::init::scissor({ 0, 0 }, m_rendererRef->getSwapchain().extent);

  VkPipelineDynamicStateCreateInfo dynamicState = cassidy::init::pipelineDynamicStateCreateinfo(
    static_cast<uint32_t>(cassidy::Renderer::DYNAMIC_STATES.size()), cassidy::Renderer::DYNAMIC_STATES.data());

  VkPipelineViewportStateCreateInfo viewportState = cassidy::init::pipelineViewportStateCreateInfo(1, &viewport, 1, &scissor);

  VkPipelineRasterizationStateCreateInfo rasteriser = cassidy::init::pipelineRasterizationStateCreateInfo(
    VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT);

  VkPipelineMultisampleStateCreateInfo multisampling = cassidy::init::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

  VkPipelineColorBlendAttachmentState colourBlendAttach = cassidy::init::pipelineColorBlendAttachmentState(
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    VK_FALSE, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD);

  VkPipelineColorBlendStateCreateInfo colourBlendState = cassidy::init::pipelineColorBlendStateCreateInfo(1, &colourBlendAttach,
    0.0f, 0.0f, 0.0f, 0.0f);

  VkPipelineDepthStencilStateCreateInfo depthStencil = cassidy::init::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE,
    VK_COMPARE_OP_LESS);

  VkPipelineLayoutCreateInfo pipelineLayout = cassidy::init::pipelineLayoutCreateInfo(0, nullptr, 0, nullptr);

  vkCreatePipelineLayout(m_rendererRef->getLogicalDevice(), &pipelineLayout, nullptr, &m_pipelineLayout);

  VkGraphicsPipelineCreateInfo pipelineInfo = cassidy::init::graphicsPipelineCreateInfo(
    2, shaderStages,
    &vertexInput, &inputAssembly,
    &viewportState, &rasteriser, 
    &multisampling, &depthStencil,
    &colourBlendState, &dynamicState, 
    m_pipelineLayout, m_renderPass, 0);

  vkCreateGraphicsPipelines(m_rendererRef->getLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline);

  vkDestroyShaderModule(m_rendererRef->getLogicalDevice(), vertexModule, nullptr);
  vkDestroyShaderModule(m_rendererRef->getLogicalDevice(), fragmentModule, nullptr);

  std::cout << "Created graphics pipeline!\n" << std::endl;
}

