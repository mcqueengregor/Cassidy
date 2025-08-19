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

    Pipeline& init(cassidy::Renderer* rendererRef);
    void release();

    // Pipeline layout dependencies binding:
    Pipeline& addPushConstantRange(VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size);
    Pipeline& addDescriptorSetLayout(VkDescriptorSetLayout setLayout);
    Pipeline& setRenderPass(VkRenderPass renderPass);
    
    void buildGraphicsPipeline(const std::string& vertexFilepath, const std::string& fragmentFilepath);

    // Getters/setters: ------------------------------------------------------------------------------------------
    VkPipeline        getGraphicsPipeline() const  { return m_graphicsPipeline; }
    VkPipelineLayout  getLayout()           const  { return m_pipelineLayout; }
    VkRenderPass      getRenderPass()       const  { return m_renderPass; }

  private:
    SpirvShaderCode loadSpirv(const std::string& filepath);

    VkPipeline m_graphicsPipeline;
    VkPipelineLayout m_pipelineLayout;

    VkRenderPass m_renderPass;

    // Pipeline layout dependencies (push constant ranges, descriptor sets):
    std::vector<VkPushConstantRange> m_pushConstantRanges;
    std::vector<VkDescriptorSetLayout> m_descSetLayouts;

    cassidy::Renderer* m_rendererRef;
  };
}