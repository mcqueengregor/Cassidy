#pragma once
#include <vector>

// Forward declarations:
class SDL_Window;

namespace cassidy
{
  class Renderer
  {
  public:
    void init();

    void draw();

    void release();

  private:
    const uint8_t FRAMES_IN_FLIGHT = 2;
  };
}
