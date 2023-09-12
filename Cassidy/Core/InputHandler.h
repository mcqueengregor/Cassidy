#pragma once

// Forward declarations:
enum SDL_KeyCode;

class InputHandler
{
public:
  void init();

  InputHandler(const InputHandler&) = delete; // Prevent copy instructions of this singleton.
  static InputHandler& get() 
  { 
    static InputHandler s_instance;
    return s_instance;
  }

  static bool isKeyDown(SDL_KeyCode keyCode) { return InputHandler::get().isKeyDownImpl(keyCode); }

private:
  InputHandler() {} // Force singleton status.
  bool isKeyDownImpl(SDL_KeyCode keyCode);

  // Keyboard state bitmasks:
  uint32_t m_keyboardStates     = 0x0000;
  uint32_t m_prevKeyboardStates = 0x0000;

  uint32_t m_keyboardDowns      = 0x0000;
  uint32_t m_keyboardUps        = 0x0000;
};

