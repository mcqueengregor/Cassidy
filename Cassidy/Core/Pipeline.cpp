#include "Pipeline.h"
#include <Core/Renderer.h>
#include <Core/Logger.h>
#include <Utils/Initialisers.h>
#include <Utils/Helpers.h>
#include <Utils/Types.h>

#include <fstream>

cassidy::Pipeline& cassidy::Pipeline::init(cassidy::Renderer* renderer)
{
  m_rendererRef = renderer;

  return *this;
}

void cassidy::Pipeline::release()
{
  vkDestroyPipeline(m_rendererRef->getLogicalDevice(), m_graphicsPipeline, nullptr);
  vkDestroyPipelineLayout(m_rendererRef->getLogicalDevice(), m_pipelineLayout, nullptr);
}

cassidy::Pipeline& cassidy::Pipeline::addPushConstantRange(VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size)
{
  m_pushConstantRanges.emplace_back(cassidy::init::pushConstantRange(stageFlags, offset, size));

  return *this;
}

cassidy::Pipeline& cassidy::Pipeline::addDescriptorSetLayout(VkDescriptorSetLayout setLayout)
{
  m_descSetLayouts.push_back(setLayout);

  return *this;
}

cassidy::Pipeline& cassidy::Pipeline::setRenderPass(VkRenderPass renderPass)
{
  m_renderPass = renderPass;

  return *this;
}

// Build graphics pipeline with vertex and fragment shaders (in SPIR-V format) contained in the "Shaders" folder.
// SHADER_ABS_FILEPATH contains the absolute filepath of the "Shaders" folder, both filepath arguments should be relative to said folder!
void cassidy::Pipeline::buildGraphicsPipeline(const std::string& vertexFilepath, const std::string& fragmentFilepath)
{
  SpirvShaderCode vertexCode = loadSpirv(SHADER_ABS_FILEPATH + vertexFilepath);
  SpirvShaderCode fragmentCode = loadSpirv(SHADER_ABS_FILEPATH + fragmentFilepath);

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
    &bindingDescription, static_cast<uint32_t>(attributeDescriptions.size()), attributeDescriptions.data());

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = cassidy::init::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

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

  VkPipelineLayoutCreateInfo pipelineLayout = cassidy::init::pipelineLayoutCreateInfo(
    static_cast<uint32_t>(m_descSetLayouts.size()), m_descSetLayouts.data(), 
    static_cast<uint32_t>(m_pushConstantRanges.size()), m_pushConstantRanges.data());

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

  CS_LOG_INFO("Created graphics pipeline!");
}

SpirvShaderCode cassidy::Pipeline::loadSpirv(const std::string& filepath)
{
  std::ifstream file(filepath, std::ios::ate | std::ios::binary);

  if (!file.is_open())
  {
    CS_LOG_ERROR("Could not load SPIR-V file! ({0})", filepath.c_str());
    return { 0, nullptr };
  }

  size_t fileSize = static_cast<size_t>(file.tellg());
  char* buffer = new char[fileSize];

  file.seekg(0);
  file.read(buffer, fileSize);
  file.close();

  return { fileSize, reinterpret_cast<uint32_t*>(buffer) };
}