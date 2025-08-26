#include "Pipeline.h"
#include <Core/Renderer.h>
#include <Core/Logger.h>
#include <Utils/Initialisers.h>
#include <Utils/Helpers.h>
#include <Utils/Types.h>

#include <fstream>

void cassidy::Pipeline::release(VkDevice device)
{
  vkDestroyPipeline(device, m_graphicsPipeline, nullptr);
  vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
}

void cassidy::GraphicsPipeline::buildGraphicsPipeline(
  uint32_t numDescSetLayouts, VkDescriptorSetLayout* descSetLayouts, 
  uint32_t numPushConstantRanges, VkPushConstantRange* pushConstantRanges, 
  uint32_t numShaderStages, VkPipelineShaderStageCreateInfo* shaderStages, 
  VkPipelineVertexInputStateCreateInfo* vertexInputStateInfo, VkPipelineInputAssemblyStateCreateInfo* inputAssemblyStateInfo, 
  VkPipelineViewportStateCreateInfo* viewportStateInfo, VkPipelineRasterizationStateCreateInfo* rasterisationStateInfo, 
  VkPipelineMultisampleStateCreateInfo* multisampleStateInfo, VkPipelineDepthStencilStateCreateInfo* depthStencilStateInfo, 
  VkPipelineColorBlendAttachmentState* colourBlendAttachState, VkPipelineDynamicStateCreateInfo* dynamicStateInfo, 
  VkRenderPass renderPass, uint32_t subpass,
  cassidy::Renderer* rendererRef)
{
  CS_LOG_INFO("Building graphics pipeline ({0})...", m_debugName);

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = cassidy::init::pipelineLayoutCreateInfo(
    numDescSetLayouts, descSetLayouts, numPushConstantRanges, pushConstantRanges);

  vkCreatePipelineLayout(rendererRef->getLogicalDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout);

  VkPipelineColorBlendStateCreateInfo colourBlendState = cassidy::init::pipelineColorBlendStateCreateInfo(
    1, colourBlendAttachState, 0.0f, 0.0f, 0.0f, 0.0f);

  VkGraphicsPipelineCreateInfo graphicsPipelineInfo = cassidy::init::graphicsPipelineCreateInfo(
    2, shaderStages,
    vertexInputStateInfo, inputAssemblyStateInfo,
    viewportStateInfo, rasterisationStateInfo,
    multisampleStateInfo, depthStencilStateInfo,
    &colourBlendState, dynamicStateInfo,
    m_pipelineLayout, renderPass, 0);

  vkCreateGraphicsPipelines(rendererRef->getLogicalDevice(), VK_NULL_HANDLE, 1, &graphicsPipelineInfo, nullptr, &m_pipeline);
}

void cassidy::ComputePipeline::buildComputePipeline(
  uint32_t numDescSetLayouts, VkDescriptorSetLayout* descSetLayouts, 
  uint32_t numPushConstantRanges, VkPushConstantRange* pushConstantRanges, 
  VkPipelineShaderStageCreateInfo* computeShaderStage, 
  cassidy::Renderer* rendererRef)
{
  CS_LOG_INFO("Building compute pipeline ({0})...", m_debugName);

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = cassidy::init::pipelineLayoutCreateInfo(
    numDescSetLayouts, descSetLayouts, numPushConstantRanges, pushConstantRanges);

  vkCreatePipelineLayout(rendererRef->getLogicalDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout);

  VkComputePipelineCreateInfo computePipelineInfo = cassidy::init::computePipelineCreateInfo(
    computeShaderStage, m_pipelineLayout);

  vkCreateComputePipelines(rendererRef->getLogicalDevice(), VK_NULL_HANDLE, 1, &computePipelineInfo, nullptr, &m_pipeline);
}

cassidy::PipelineBuilder::~PipelineBuilder()
{
  for (const auto& stage : m_shaderStages)
    delete[] stage.second.codeBuffer;
}

cassidy::PipelineBuilder& cassidy::PipelineBuilder::addShaderStage(VkShaderStageFlagBits stage, const std::string& filepath)
{
  const char* stageName = "";
  switch (stage)
  {
  case VK_SHADER_STAGE_VERTEX_BIT:
    stageName = "VERTEX";
    break;
  case VK_SHADER_STAGE_FRAGMENT_BIT:
    stageName = "FRAGMENT";
    break;
  case VK_SHADER_STAGE_COMPUTE_BIT:
    stageName = "COMPUTE";
    break;
  }

  // Remove existing shader stage code if one already exists:
  if (m_shaderStages.find(stage) != m_shaderStages.end())
    delete[] m_shaderStages[stage].codeBuffer;

  SpirvShaderCode shaderCode = loadSpirv(SHADER_ABS_FILEPATH + filepath);
  if (shaderCode.codeBuffer)
  {
    m_shaderStages[stage] = shaderCode;
    CS_LOG_INFO("Updated pipeline builder shader stage ({0}: {1}", stageName, filepath);
  }

  return *this;
}

cassidy::PipelineBuilder& cassidy::PipelineBuilder::addPushConstantRange(VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size)
{
  m_pushConstantRanges.emplace_back(cassidy::init::pushConstantRange(stageFlags, offset, size));
  return *this;
}

bool cassidy::PipelineBuilder::buildGraphicsPipeline(GraphicsPipeline& pipeline)
{
  bool wasBuildSuccessful = true;
  const std::string_view name = pipeline.getDebugName();

  if (m_shaderStages.find(VK_SHADER_STAGE_VERTEX_BIT) == m_shaderStages.end())
  {
    CS_LOG_ERROR("Pipeline \"{0}\" doesn't possess a vertex shader stage module!", name);
    wasBuildSuccessful = false;
  }

  if (m_shaderStages.find(VK_SHADER_STAGE_FRAGMENT_BIT) == m_shaderStages.end())
  {
    CS_LOG_ERROR("Pipeline \"{0}\" doesn't possess a fragment shader stage module!", name);
    wasBuildSuccessful = false;
  }

  if (m_currentRenderPass == VK_NULL_HANDLE)
  {
    CS_LOG_ERROR("No render pass assigned to pipeline \"{0}\" before creation!", name);
    wasBuildSuccessful = false;
  }

  if (!wasBuildSuccessful) return false;

  VkShaderModuleCreateInfo vertexModuleInfo = cassidy::init::shaderModuleCreateInfo(m_shaderStages.at(VK_SHADER_STAGE_VERTEX_BIT));
  VkShaderModuleCreateInfo fragmentModuleInfo = cassidy::init::shaderModuleCreateInfo(m_shaderStages.at(VK_SHADER_STAGE_FRAGMENT_BIT));

  VkShaderModule vertexModule;
  VkShaderModule fragmentModule;

  vkCreateShaderModule(m_rendererRef->getLogicalDevice(), &vertexModuleInfo, nullptr, &vertexModule);
  vkCreateShaderModule(m_rendererRef->getLogicalDevice(), &fragmentModuleInfo, nullptr, &fragmentModule);

  VkPipelineShaderStageCreateInfo shaderStages[] = {
    cassidy::init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexModule),
    cassidy::init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentModule),
  };

  VkViewport viewport = {
    .x = 0.0f,
    .y = 0.0f,
    .width = static_cast<float>(m_rendererRef->getSwapchain().extent.width),
    .height = static_cast<float>(m_rendererRef->getSwapchain().extent.height),
    .minDepth = 0.0f,
    .maxDepth = 1.0f,
  };

  VkRect2D scissor = {
    .offset = { 0, 0 },
    .extent = m_rendererRef->getSwapchain().extent,
  };

  VkPipelineViewportStateCreateInfo viewportInfo = cassidy::init::pipelineViewportStateCreateInfo(1, &viewport, 1, &scissor);

  VkPipelineDynamicStateCreateInfo dynamicStateInfo = cassidy::init::pipelineDynamicStateCreateinfo(
    static_cast<uint32_t>(cassidy::Renderer::DYNAMIC_STATES.size()), cassidy::Renderer::DYNAMIC_STATES.data());

  auto bindingDescription = Vertex::getBindingDesc();
  auto attributeDescriptions = Vertex::getAttributeDescs();

  m_vertexInputStateInfo = cassidy::init::pipelineVertexInputStateCreateInfo(
    1, &bindingDescription, static_cast<uint32_t>(attributeDescriptions.size()), attributeDescriptions.data());

  pipeline.buildGraphicsPipeline(
    static_cast<uint32_t>(m_descSetLayouts.size()), m_descSetLayouts.data(),
    static_cast<uint32_t>(m_pushConstantRanges.size()), m_pushConstantRanges.data(),
    2, shaderStages,
    &m_vertexInputStateInfo, &m_inputAssemblyStateInfo,
    &viewportInfo, &m_rasterisationStateInfo,
    &m_multisampleStateInfo, &m_depthStencilStateInfo,
    &m_colourBlendAttachState, &dynamicStateInfo,
    m_currentRenderPass, 0, m_rendererRef);

  vkDestroyShaderModule(m_rendererRef->getLogicalDevice(), vertexModule, nullptr);
  vkDestroyShaderModule(m_rendererRef->getLogicalDevice(), fragmentModule, nullptr);

  return true;
}

bool cassidy::PipelineBuilder::buildComputePipeline(ComputePipeline& pipeline)
{
  bool wasBuildSuccessful = true;
  const std::string_view name = pipeline.getDebugName();

  if (m_shaderStages.find(VK_SHADER_STAGE_COMPUTE_BIT) == m_shaderStages.end())
  {
    CS_LOG_ERROR("Pipeline \"{0}\" doesn't possess a compute shader stage module!", name);
    wasBuildSuccessful = false;
  }

  if (!wasBuildSuccessful) return false;

  VkShaderModuleCreateInfo computeModuleInfo = cassidy::init::shaderModuleCreateInfo(m_shaderStages.at(VK_SHADER_STAGE_COMPUTE_BIT));

  VkShaderModule computeModule;

  vkCreateShaderModule(m_rendererRef->getLogicalDevice(), &computeModuleInfo, nullptr, &computeModule);

  VkPipelineShaderStageCreateInfo stageInfo = cassidy::init::pipelineShaderStageCreateInfo(
    VK_SHADER_STAGE_COMPUTE_BIT, computeModule);

  pipeline.buildComputePipeline(
    static_cast<uint32_t>(m_descSetLayouts.size()), m_descSetLayouts.data(),
    static_cast<uint32_t>(m_pushConstantRanges.size()), m_pushConstantRanges.data(),
    &stageInfo, m_rendererRef);

  return true;
}

cassidy::PipelineBuilder& cassidy::PipelineBuilder::resetToDefaults()
{
  for (const auto& stage : m_shaderStages)
  {
    delete[] stage.second.codeBuffer;
    const VkShaderStageFlagBits stageType = stage.first;
    m_shaderStages.erase(stageType);
  }
  m_pushConstantRanges.clear();
  m_descSetLayouts.clear();
  m_currentRenderPass = VK_NULL_HANDLE;

  m_inputAssemblyStateInfo = cassidy::init::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

  m_rasterisationStateInfo = cassidy::init::pipelineRasterizationStateCreateInfo(
    VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT);

  m_multisampleStateInfo = cassidy::init::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

  m_colourBlendAttachState = cassidy::init::pipelineColorBlendAttachmentState(
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    VK_FALSE, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD);

  m_depthStencilStateInfo = cassidy::init::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE,
    VK_COMPARE_OP_LESS);

  return *this;
}

SpirvShaderCode cassidy::PipelineBuilder::loadSpirv(const std::string& filepath)
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
