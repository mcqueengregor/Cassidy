#pragma once
#include "Utils/Types.h"

#include <string>

namespace cassidy
{
  struct Swapchain;
  class Renderer;

  class Pipeline
  {
  public:
    Pipeline() {};
    Pipeline(cassidy::Renderer* renderer, const std::string& vertexFilepath, const std::string& fragmentFilepath);

    ~Pipeline();

    void init(cassidy::Renderer* renderer, const std::string& vertexFilepath, const std::string& fragmentFilepath);

    SpirvShaderCode loadSpirv(const std::string& filepath);

    enum class ShaderType
    {
      VERTEX    = 0,
      FRAGMENT  = 1,
      COMPUTE   = 2,
      GEOMETRY  = 3,
    };

  private:
    void initRenderPass();
    void initGraphicsPipeline(const std::string& vertexFilepath, const std::string& fragmentFilepath);

    VkPipeline m_graphicsPipeline;
    VkPipelineLayout m_pipelineLayout;

    VkRenderPass m_renderPass;

    cassidy::Renderer* m_rendererRef;
  };
}
