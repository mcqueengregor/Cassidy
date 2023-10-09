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
    Pipeline(cassidy::Renderer* rendererh);

    Pipeline& init(cassidy::Renderer* renderer);
    void release();

    // Pipeline layout dependencies binding:
    Pipeline& addPushConstantRange(VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size);
    Pipeline& addDescriptorSetLayout(VkDescriptorSetLayout setLayout);

    void buildGraphicsPipeline(const std::string& vertexFilepath, const std::string& fragmentFilepath);

    SpirvShaderCode loadSpirv(const std::string& filepath);

    enum class ShaderType
    {
      VERTEX    = 0,
      FRAGMENT  = 1,
      COMPUTE   = 2,
      GEOMETRY  = 3,
    };

    // Getters/setters: ------------------------------------------------------------------------------------------
    VkPipeline getGraphicsPipeline()  { return m_graphicsPipeline; }
    VkPipelineLayout getLayout()      { return m_pipelineLayout; }
    VkRenderPass getRenderPass()      { return m_renderPass; }

  private:
    void initRenderPass();

    VkPipeline m_graphicsPipeline;
    VkPipelineLayout m_pipelineLayout;

    VkRenderPass m_renderPass;

    // Pipeline layout dependencies (push constant ranges, descriptor sets):
    std::vector<VkPushConstantRange> m_pushConstantRanges;
    std::vector<VkDescriptorSetLayout> m_descSetLayouts;

    cassidy::Renderer* m_rendererRef;
  };
}