#pragma once
#include "InputHandler.h"

// Forward declarations:
union SDL_Event;

class EventHandler
{
public:
  void init();

  enum class KeyboardState
  {
    KEY_PRESSED   = 0,  // Key was pressed this frame.
    KEY_HELD      = 1,  // Key was pressed in a previous frame and is still pressed.
    KEY_RELEASED  = 2,  // Key was released this frame.
    KEY_UP        = 3,  // Key was released in a previous frame and is still released.
  };

  void processEvent(SDL_Event* event);

  KeyboardState getKeyboardButtonState(SDL_KeyCode key);
private:

};

