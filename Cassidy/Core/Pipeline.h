#pragma once
#include "Utils/Types.h"

#include <string>

namespace cassidy
{
  // Forward declarations:
  class Renderer;

  class Pipeline
  {
  public:
    Pipeline() {};

    Pipeline& init(cassidy::Renderer* rendererRef, const std::string& debugName = std::string(""));
    void release();

    // Pipeline layout dependencies binding:
    Pipeline& addPushConstantRange(VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size);
    Pipeline& addDescriptorSetLayout(VkDescriptorSetLayout setLayout);
    Pipeline& setRenderPass(VkRenderPass renderPass);

    void buildGraphicsPipeline(const std::string& vertexFilepath, const std::string& fragmentFilepath);
    void buildGraphicsPipeline(
      uint32_t numDescSetLayouts, VkDescriptorSetLayout* descSetLayouts,
      uint32_t numPushConstantRanges, VkPushConstantRange* pushConstantRanges,
      uint32_t numShaderStages, VkPipelineShaderStageCreateInfo* shaderStages,
      VkPipelineVertexInputStateCreateInfo* vertexInputStateInfo,
      VkPipelineInputAssemblyStateCreateInfo* inputAssemblyStateInfo,
      VkPipelineViewportStateCreateInfo* viewportStateInfo,
      VkPipelineRasterizationStateCreateInfo* rasterisationStateInfo,
      VkPipelineMultisampleStateCreateInfo* multisampleStateInfo,
      VkPipelineDepthStencilStateCreateInfo* depthStencilStateInfo,
      VkPipelineColorBlendAttachmentState* colourBlendAttachState,
      VkPipelineDynamicStateCreateInfo* dynamicStateInfo,
      VkRenderPass* renderPass, uint32_t subpass,
      cassidy::Renderer* rendererRef);

    // Getters/setters: ------------------------------------------------------------------------------------------
    inline VkPipeline         getGraphicsPipeline()     const { return m_graphicsPipeline; }
    inline VkPipelineLayout   getLayout()               const { return m_pipelineLayout; }
    inline VkRenderPass       getRenderPass()           const { return m_renderPass; }
    inline std::string_view   getDebugName()            const { return m_debugName; }

  private:
    SpirvShaderCode loadSpirv(const std::string& filepath);

    VkPipeline m_graphicsPipeline;
    VkPipelineLayout m_pipelineLayout;

    VkRenderPass m_renderPass;

    // Pipeline layout dependencies (push constant ranges, descriptor sets):
    std::vector<VkPushConstantRange> m_pushConstantRanges;
    std::vector<VkDescriptorSetLayout> m_descSetLayouts;

    cassidy::Renderer* m_rendererRef;
    std::string m_debugName;
  };

  class PipelineBuilder
  {
  public:
    PipelineBuilder(cassidy::Renderer* rendererRef) : m_rendererRef(rendererRef), m_currentRenderPass(VK_NULL_HANDLE) {}

    PipelineBuilder& setVertexInputStateInfo(VkPipelineVertexInputStateCreateInfo vertexInputStateInfo)         { m_vertexInputStateInfo = vertexInputStateInfo; return *this; }
    PipelineBuilder& setInputAssemblyStateInfo(VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo)   { m_inputAssemblyStateInfo = inputAssemblyStateInfo; return *this; }
    PipelineBuilder& setRasterisationStateInfo(VkPipelineRasterizationStateCreateInfo rasterisationStateInfo)   { m_rasterisationStateInfo = rasterisationStateInfo; return *this; }
    PipelineBuilder& setColourBlendAttachmentState(VkPipelineColorBlendAttachmentState colourBlendAttachState)  { m_colourBlendAttachState = colourBlendAttachState;  return *this; }
    PipelineBuilder& setMultisampleState(VkPipelineMultisampleStateCreateInfo multisampleStateInfo)             { m_multisampleStateInfo = multisampleStateInfo; return *this; }
    PipelineBuilder& setDepthStencilState(VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo)          { m_depthStencilStateInfo = depthStencilStateInfo; return *this; }

    PipelineBuilder& addShaderStage(VkShaderStageFlagBits stage, const std::string& filepath);
    PipelineBuilder& addDescSetLayout(VkDescriptorSetLayout descSetLayout) { m_descSetLayouts.push_back(descSetLayout); return *this; }
    PipelineBuilder& addPushConstantRange(VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size);
    PipelineBuilder& setRenderPass(VkRenderPass renderPass) { m_currentRenderPass = renderPass; return *this; }

    bool buildGraphicsPipeline(Pipeline& pipeline);

    PipelineBuilder& resetToDefaults();

  private:
    PipelineBuilder() = delete; // Require reference to renderer in object creation
    
    SpirvShaderCode loadSpirv(const std::string& filepath);

    typedef std::unordered_map<VkShaderStageFlagBits, SpirvShaderCode> ShaderStageMap;
    ShaderStageMap m_shaderStages;

    VkPipelineVertexInputStateCreateInfo			m_vertexInputStateInfo;
    VkPipelineInputAssemblyStateCreateInfo		m_inputAssemblyStateInfo;
    VkPipelineRasterizationStateCreateInfo		m_rasterisationStateInfo;
    VkPipelineColorBlendAttachmentState				m_colourBlendAttachState;
    VkPipelineMultisampleStateCreateInfo			m_multisampleStateInfo;
    VkPipelineDepthStencilStateCreateInfo			m_depthStencilStateInfo;

    std::vector<VkPushConstantRange> m_pushConstantRanges;
    std::vector<VkDescriptorSetLayout> m_descSetLayouts;
    VkRenderPass m_currentRenderPass;

    cassidy::Renderer* m_rendererRef;
  };
}