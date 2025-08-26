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
    Pipeline() : m_pipeline(VK_NULL_HANDLE), m_pipelineLayout(VK_NULL_HANDLE), m_debugName("") {}
    void release(VkDevice device);

    // Getters/setters: ------------------------------------------------------------------------------------------
    inline VkPipeline         getPipeline()     const { return m_pipeline; }
    inline VkPipelineLayout   getLayout()               const { return m_pipelineLayout; }
    inline std::string_view   getDebugName()            const { return m_debugName; }

    void setDebugName(const std::string& name) { m_debugName = name; }

  protected:
    VkPipeline m_pipeline;
    VkPipelineLayout m_pipelineLayout;
    std::string m_debugName;
  };

  class GraphicsPipeline : public Pipeline
  {
  public:
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
      VkRenderPass renderPass, uint32_t subpass,
      cassidy::Renderer* rendererRef);

  private:

  };

  class ComputePipeline : public Pipeline
  {
  public:
    void buildComputePipeline(
      uint32_t numDescSetLayouts, VkDescriptorSetLayout* descSetLayouts,
      uint32_t numPushConstantRanges, VkPushConstantRange* pushConstantRanges,
      VkPipelineShaderStageCreateInfo* computeShaderStage,
      cassidy::Renderer* rendererRef);

  private:

  };

  class PipelineBuilder
  {
  public:
    PipelineBuilder(cassidy::Renderer* rendererRef) :
      m_rendererRef(rendererRef), m_currentRenderPass(VK_NULL_HANDLE) { resetToDefaults(); }
    ~PipelineBuilder();

    PipelineBuilder& setVertexInputStateInfo(VkPipelineVertexInputStateCreateInfo vertexInputStateInfo)         { m_vertexInputStateInfo = vertexInputStateInfo; return *this; }
    PipelineBuilder& setInputAssemblyStateInfo(VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo)   { m_inputAssemblyStateInfo = inputAssemblyStateInfo; return *this; }
    PipelineBuilder& setRasterisationStateInfo(VkPipelineRasterizationStateCreateInfo rasterisationStateInfo)   { m_rasterisationStateInfo = rasterisationStateInfo; return *this; }
    PipelineBuilder& setColourBlendAttachmentState(VkPipelineColorBlendAttachmentState colourBlendAttachState)  { m_colourBlendAttachState = colourBlendAttachState;  return *this; }
    PipelineBuilder& setMultisampleState(VkPipelineMultisampleStateCreateInfo multisampleStateInfo)             { m_multisampleStateInfo = multisampleStateInfo; return *this; }
    PipelineBuilder& setDepthStencilState(VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo)          { m_depthStencilStateInfo = depthStencilStateInfo; return *this; }

    PipelineBuilder& addShaderStage(VkShaderStageFlagBits stage, const std::string& filepath);
    PipelineBuilder& addDescriptorSetLayout(VkDescriptorSetLayout descSetLayout) { m_descSetLayouts.push_back(descSetLayout); return *this; }
    PipelineBuilder& addPushConstantRange(VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size);
    PipelineBuilder& setRenderPass(VkRenderPass renderPass) { m_currentRenderPass = renderPass; return *this; }

    bool buildGraphicsPipeline(GraphicsPipeline& pipeline);
    bool buildComputePipeline(ComputePipeline& pipeline);

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