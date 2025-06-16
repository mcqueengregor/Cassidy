#include "Core/Engine.h"
#include <spdlog/spdlog.h>

int main(int argc, char* argv[])
{
  spdlog::info("Hello, world!");
  spdlog::error("This is an error.");
  spdlog::critical("This is a critical message.");
  spdlog::warn("This is a warning message!");

  cassidy::Engine engine(glm::uvec2(1280, 720));

  engine.init();

  engine.run();

  engine.release();

  return 0;
}