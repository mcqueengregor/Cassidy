#include "Core/Engine.h"
#include <iostream>

int main(int argc, char* argv[])
{
  cassidy::Engine engine(glm::uvec2(1280, 720));

  engine.init();

  engine.run();

  engine.release();

  return 0;
}