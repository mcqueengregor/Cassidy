#include "Core/Engine.h"
#include <spdlog/spdlog.h>
#include <typeindex>
#include <typeinfo>

int main(int argc, char* argv[])
{
  spdlog::info("Hello, world!");
  spdlog::error("This is an error.");
  spdlog::critical("This is a critical message.");
  spdlog::warn("This is a warning message!");

  const std::type_info& lightInfo = typeid(LightBufferData);
  const std::type_index lightIndex(lightInfo);
  spdlog::warn("LightBufferData::info.name: {0}", lightInfo.name());
  spdlog::warn("LightBufferData::index.hash_code: {0}", lightIndex.hash_code());

  const std::type_info& swapchainInfo = typeid(cassidy::Swapchain);
  const std::type_index swapchainIndex(swapchainInfo);
  spdlog::warn("Swapchain::info.name: {0}", swapchainInfo.name());
  spdlog::warn("Swapchain::index.hash_code: {0}", swapchainIndex.hash_code());

  cassidy::Engine engine(glm::uvec2(1280, 720));

  engine.init();

  engine.run();

  engine.release();

  return 0;
}