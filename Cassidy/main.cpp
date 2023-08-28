#include "Core/Engine.h"
#include <iostream>

int main(int argc, char* argv[])
{
  cassidy::Engine engine;

  engine.init();

  engine.run();

  engine.release();

  return 0;
}