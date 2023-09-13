#pragma once
#include "InputHandler.h"

// Forward declarations:
union SDL_Event;

namespace cassidy
{
  class EventHandler
  {
  public:
    void init();

    void processEvent(SDL_Event* event);

  private:

  };
}
