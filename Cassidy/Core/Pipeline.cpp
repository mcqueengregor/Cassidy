#include "Pipeline.h"
#include <Core/Renderer.h>
#include <Core/Logger.h>
#include <Utils/Initialisers.h>
#include <Utils/Helpers.h>
#include <Utils/Types.h>

#include <fstream>

cassidy::Pipeline& cassidy::Pipeline::init(cassidy::Renderer* renderer, const std::string& debugName)
{
  m_rendererRef = renderer;
  m_debugName = debugName;

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

void cassidy::Pipeline::buildGraphicsPipeline(
  uint32_t numDescSetLayouts, VkDescriptorSetLayout* descSetLayouts, 
  uint32_t numPushConstantRanges, VkPushConstantRange* pushConstantRanges, 
  uint32_t numShaderStages, VkPipelineShaderStageCreateInfo* shaderStages, 
  VkPipelineVertexInputStateCreateInfo* vertexInputStateInfo, VkPipelineInputAssemblyStateCreateInfo* inputAssemblyStateInfo, 
  VkPipelineViewportStateCreateInfo* viewportStateInfo, VkPipelineRasterizationStateCreateInfo* rasterisationStateInfo, 
  VkPipelineMultisampleStateCreateInfo* multisampleStateInfo, VkPipelineDepthStencilStateCreateInfo* depthStencilStateInfo, 
  VkPipelineColorBlendAttachmentState* colourBlendAttachState, VkPipelineDynamicStateCreateInfo* dynamicStateInfo, 
  VkRenderPass* renderPass, uint32_t subpass,
  cassidy::Renderer* rendererRef)
{
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
    m_pipelineLayout, m_renderPass, 0);

  vkCreateGraphicsPipelines(rendererRef->getLogicalDevice(), VK_NULL_HANDLE, 1, &graphicsPipelineInfo, nullptr, &m_graphicsPipeline);
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

cassidy::PipelineBuilder& cassidy::PipelineBuilder::addShaderStage(VkShaderStageFlagBits stage, const std::string& filepath)
{
  // Remove existing shader stage code if 
  if (m_shaderStages.find(stage) != m_shaderStages.end())
    m_shaderStages.erase(stage);

  m_shaderStages[stage] = loadSpirv(filepath);
  return *this;
}

cassidy::PipelineBuilder& cassidy::PipelineBuilder::addPushConstantRange(VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size)
{
  m_pushConstantRanges.emplace_back(cassidy::init::pushConstantRange(stageFlags, offset, size));
  return *this;
}

bool cassidy::PipelineBuilder::buildGraphicsPipeline(Pipeline& pipeline)
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

  if (wasBuildSuccessful)
  {
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

    pipeline.buildGraphicsPipeline(
      static_cast<uint32_t>(m_descSetLayouts.size()), m_descSetLayouts.data(),
      static_cast<uint32_t>(m_pushConstantRanges.size()), m_pushConstantRanges.data(),
      2, shaderStages,
      &m_vertexInputStateInfo, &m_inputAssemblyStateInfo,
      &viewportInfo, &m_rasterisationStateInfo,
      &m_multisampleStateInfo, &m_depthStencilStateInfo,
      &m_colourBlendAttachState, &dynamicStateInfo,
      &m_currentRenderPass, 0, m_rendererRef);

    vkDestroyShaderModule(m_rendererRef->getLogicalDevice(), vertexModule, nullptr);
    vkDestroyShaderModule(m_rendererRef->getLogicalDevice(), fragmentModule, nullptr);
  }

  return wasBuildSuccessful;
}

cassidy::PipelineBuilder& cassidy::PipelineBuilder::resetToDefaults()
{
  for (const auto& stage : m_shaderStages)
  {
    const VkShaderStageFlagBits stageType = stage.first;
    m_shaderStages.erase(stageType);
  }
  m_pushConstantRanges.clear();
  m_descSetLayouts.clear();
  m_currentRenderPass = VK_NULL_HANDLE;
  
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
