#pragma once

#include <SDL.h>

// SDL_SCANCODE_CAPSLOCK - 126, forces new CAPSLOCK keycode to DELETE + 1:
#define SDL_KEYCODE_CONVERT_CONSTANT (0x3FFF'FFBB)
#define SDL_KEYCODE_TO_BETTER_KEYCODE(x) (x - SDL_KEYCODE_CONVERT_CONSTANT)

class InputHandler
{
public:
  InputHandler(const InputHandler&) = delete; // Prevent copy instructions of this singleton.
  static InputHandler& get() 
  { 
    static InputHandler s_instance;
    return s_instance;
  }

  static inline void init() { InputHandler::get().initImpl(); }

  static inline void updateKeyStates() { InputHandler::get().updateKeyStatesImpl(); }

  static inline void setKeyDown(SDL_Keycode keyCode)  { InputHandler::get().setKeyDownImpl(keyCode); }
  static inline void setKeyUp(SDL_Keycode keyCode)    { InputHandler::get().setKeyUpImpl(keyCode); }

  /* 
    Return whether or not a key satisfies a given state.
    - "Pressed" indicates the key was pressed in this frame.
    - "Held" indicates the key was pressed in a previous frame and is still pressed.
    - "Released" indicates the key was released in this frame.
    - "Up" indicates the key was released in a previous frame and is still released.
  */
  static inline bool isKeyPressed(SDL_Keycode keyCode)   { return InputHandler::get().isKeyPressedImpl(keyCode); }
  static inline bool isKeyHeld(SDL_Keycode keyCode)      { return InputHandler::get().isKeyHeldImpl(keyCode); }
  static inline bool isKeyReleased(SDL_Keycode keyCode)  { return InputHandler::get().isKeyReleasedImpl(keyCode); }
  static inline bool isKeyUp(SDL_Keycode keyCode)        { return InputHandler::get().isKeyUpImpl(keyCode); }

private:
  InputHandler() {} // Force singleton status.
  
  void initImpl();
  void updateKeyStatesImpl();

  void setKeyDownImpl(SDL_Keycode keyCode);
  void setKeyUpImpl(SDL_Keycode keyCode);

  bool isKeyPressedImpl(SDL_Keycode keyCode);
  bool isKeyHeldImpl(SDL_Keycode keyCode);
  bool isKeyReleasedImpl(SDL_Keycode keyCode);
  bool isKeyUpImpl(SDL_Keycode keyCode);

  // Keyboard states:
  static const uint16_t KEYBOARD_SIZE = SDL_KEYCODE_TO_BETTER_KEYCODE(SDLK_SLEEP);

  bool m_keyboardStates[KEYBOARD_SIZE];
  bool m_prevKeyboardStates[KEYBOARD_SIZE];

  bool m_keyboardDowns[KEYBOARD_SIZE];
  bool m_keyboardUps[KEYBOARD_SIZE];
};
