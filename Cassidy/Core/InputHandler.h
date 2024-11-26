#pragma once

#include "Utils/KeyCode.h"
#include "Utils/MouseCode.h"
#include <SDL_events.h>
#include <unordered_map>

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

  static inline void updateKeyStates()    { InputHandler::get().updateKeyStatesImpl(); }
  static inline void updateMouseStates()  { InputHandler::get().updateMouseStatesImpl(); }

  static inline void flushDynamicMouseStates() { InputHandler::get().flushDynamicMouseStatesImpl(); }

  static inline void logMousePosition() { InputHandler::get().logMousePositionImpl(); }
  static inline void moveMouseToLoggedPosition() { InputHandler::get().moveMouseToLoggedPositionImpl(); }

  static inline void setKeyDown(SDL_Keycode keyCode)  { InputHandler::get().setKeyDownImpl(keyCode); }
  static inline void setKeyUp(SDL_Keycode keyCode)    { InputHandler::get().setKeyUpImpl(keyCode); }

  static inline void setMouseButtonDown(int mouseCode)        { InputHandler::get().setMouseButtonDownImpl(mouseCode); }
  static inline void setMouseButtonUp(int mouseCode)          { InputHandler::get().setMouseButtonUpImpl(mouseCode); }
  static inline void setMouseButtonDown(MouseCode mouseCode)  { InputHandler::get().setMouseButtonDownImpl(mouseCode); }
  static inline void setMouseButtonUp(MouseCode mouseCode)    { InputHandler::get().setMouseButtonUpImpl(mouseCode); }

  static inline void setCursorMovement(SDL_MouseMotionEvent event) { InputHandler::get().setCursorMovementImpl(event); }

  static inline void lockCursor()   { InputHandler::get().lockCursorImpl(); }
  static inline void unlockCursor() { InputHandler::get().unlockCursorImpl(); }

  static inline void hideCursor() { InputHandler::get().hideCursorImpl(); }
  static inline void showCursor() { InputHandler::get().showCursorImpl(); }
  static inline void centreCursor(int32_t windowWidth, int32_t windowHeight) { InputHandler::get().centreCursorImpl(windowWidth, windowHeight); }

  /*
    Input states:
    - "Pressed" indicates the key was pressed in this frame.
    - "Held" indicates the key was pressed in a previous frame and is still pressed.
    - "Released" indicates the key was released in this frame.
    - "Up" indicates the key was released in a previous frame and is still released.
  */

  static inline bool isKeyPressed(KeyCode keyCode) { return InputHandler::get().isKeyPressedImpl(keyCode); }
  static inline bool isKeyHeld(KeyCode keyCode) { return InputHandler::get().isKeyHeldImpl(keyCode); }
  static inline bool isKeyReleased(KeyCode keyCode) { return InputHandler::get().isKeyReleasedImpl(keyCode); }
  static inline bool isKeyUp(KeyCode keyCode) { return InputHandler::get().isKeyUpImpl(keyCode); }

  static inline bool isMouseButtonPressed(MouseCode mouseCode) { return InputHandler::get().isMouseButtonPressedImpl(mouseCode); }
  static inline bool isMouseButtonHeld(MouseCode mouseCode) { return InputHandler::get().isMouseButtonHeldImpl(mouseCode); }
  static inline bool isMouseButtonReleased(MouseCode mouseCode) { return InputHandler::get().isMouseButtonReleasedImpl(mouseCode); }
  static inline bool isMouseButtonUp(MouseCode mouseCode) { return InputHandler::get().isMouseButtonUpImpl(mouseCode); }

  static inline int32_t getCursorPositionX() { return InputHandler::get().getCursorPositionXImpl(); }
  static inline int32_t getCursorPositionY() { return InputHandler::get().getCursorPositionYImpl(); }

  static inline int32_t getCursorOffsetX() { return InputHandler::get().getCursorOffsetXImpl(); }
  static inline int32_t getCursorOffsetY() { return InputHandler::get().getCursorOffsetYImpl(); }

  static inline int32_t getCursorLoggedPositionX() { return InputHandler::get().getCursorLoggedPositionXImpl(); }
  static inline int32_t getCursorLoggedPositionY() { return InputHandler::get().getCursorLoggedPositionYImpl(); }

private:
  InputHandler() {}

  void initImpl();
  void updateKeyStatesImpl();
  void updateMouseStatesImpl();
  
  void flushDynamicMouseStatesImpl();

  void logMousePositionImpl();
  void moveMouseToLoggedPositionImpl();

  void setKeyDownImpl(SDL_Keycode keyCode);
  void setKeyUpImpl(SDL_Keycode keyCode);

  void setMouseButtonDownImpl(int mouseCode);
  void setMouseButtonUpImpl(int mouseCode);
  void setMouseButtonDownImpl(MouseCode mouseCode);
  void setMouseButtonUpImpl(MouseCode mouseCode);

  void setCursorMovementImpl(SDL_MouseMotionEvent event);

  void lockCursorImpl();
  void unlockCursorImpl();

  void hideCursorImpl();
  void showCursorImpl();
  void centreCursorImpl(int32_t windowWidth, int32_t windowHeight);

  bool isKeyPressedImpl(KeyCode keyCode);
  bool isKeyHeldImpl(KeyCode keyCode);
  bool isKeyReleasedImpl(KeyCode keyCode);
  bool isKeyUpImpl(KeyCode keyCode);

  bool isMouseButtonPressedImpl(MouseCode mouseCode);
  bool isMouseButtonHeldImpl(MouseCode mouseCode);
  bool isMouseButtonReleasedImpl(MouseCode mouseCode);
  bool isMouseButtonUpImpl(MouseCode mouseCode);

  int32_t getCursorPositionXImpl();
  int32_t getCursorPositionYImpl();

  int32_t getCursorOffsetXImpl();
  int32_t getCursorOffsetYImpl();

  int32_t getCursorLoggedPositionXImpl();
  int32_t getCursorLoggedPositionYImpl();

  // Keyboard states:
  static const uint16_t KEYBOARD_SIZE = SDL_TRANSFORM_KEYCODE_TO_NEW_RANGE(SDLK_SLEEP);

  // Defined as bitmasks to store all 5 button states, as well as position of cursor relative
  // to window and other attributes:
  struct MouseState
  {
    uint8_t mouseState;
    uint8_t prevMouseState;

    uint8_t mouseUp;
    uint8_t mouseDown;

    int32_t mouseRelativePositionX;
    int32_t mouseRelativePositionY;

    int32_t prevMouseRelativePositionX;
    int32_t prevMouseRelativePositionY;

    int32_t mouseOriginalRelativePositionX;
    int32_t mouseOriginalRelativePositionY;

    int32_t mouseRelativeMotionX;
    int32_t mouseRelativeMotionY;

    int32_t loggedMouseRelativePositionX;
    int32_t loggedMouseRelativePositionY;

    bool isCursorLocked;
  } m_mouseState;

  // Defined as array of struct of bools, since there are too many potential keys to store as a single mask:
  // TODO: Rework to arrays of 64-bit masks for better memory packing?
  struct KeyboardState
  {
    bool keyboardState;
    bool prevKeyboardState;

    bool keyboardDown;
    bool keyboardUp;
  } m_keyboardStates[KEYBOARD_SIZE];

  const std::unordered_map<SDL_Keycode, KeyCode> KEYCODE_CONVERSION_TABLE = {
    { SDLK_UNKNOWN, KeyCode::KEYCODE_UNKNOWN },

    { SDLK_RETURN, KeyCode::KEYCODE_RETURN },
    { SDLK_ESCAPE, KeyCode::KEYCODE_ESCAPE },
    { SDLK_BACKSPACE, KeyCode::KEYCODE_BACKSPACE },
    { SDLK_TAB, KeyCode::KEYCODE_TAB },
    { SDLK_SPACE, KeyCode::KEYCODE_SPACE },
    { SDLK_EXCLAIM, KeyCode::KEYCODE_EXCLAIM },
    { SDLK_QUOTEDBL, KeyCode::KEYCODE_QUOTEDBL },
    { SDLK_HASH, KeyCode::KEYCODE_HASH },
    { SDLK_PERCENT, KeyCode::KEYCODE_PERCENT },
    { SDLK_DOLLAR, KeyCode::KEYCODE_DOLLAR },
    { SDLK_AMPERSAND, KeyCode::KEYCODE_AMPERSAND },
    { SDLK_QUOTE, KeyCode::KEYCODE_QUOTE },
    { SDLK_LEFTPAREN, KeyCode::KEYCODE_LEFTPAREN },
    { SDLK_RIGHTPAREN, KeyCode::KEYCODE_RIGHTPAREN },
    { SDLK_ASTERISK, KeyCode::KEYCODE_ASTERISK },
    { SDLK_PLUS, KeyCode::KEYCODE_PLUS },
    { SDLK_COMMA, KeyCode::KEYCODE_COMMA },
    { SDLK_MINUS, KeyCode::KEYCODE_MINUS },
    { SDLK_PERIOD, KeyCode::KEYCODE_PERIOD },
    { SDLK_SLASH, KeyCode::KEYCODE_SLASH },
    { SDLK_0, KeyCode::KEYCODE_0 },
    { SDLK_1, KeyCode::KEYCODE_1 },
    { SDLK_2, KeyCode::KEYCODE_2 },
    { SDLK_3, KeyCode::KEYCODE_3 },
    { SDLK_4, KeyCode::KEYCODE_4 },
    { SDLK_5, KeyCode::KEYCODE_5 },
    { SDLK_6, KeyCode::KEYCODE_6 },
    { SDLK_7, KeyCode::KEYCODE_7 },
    { SDLK_8, KeyCode::KEYCODE_8 },
    { SDLK_9, KeyCode::KEYCODE_9 },
    { SDLK_COLON, KeyCode::KEYCODE_COLON },
    { SDLK_SEMICOLON, KeyCode::KEYCODE_SEMICOLON },
    { SDLK_LESS, KeyCode::KEYCODE_LESS },
    { SDLK_EQUALS, KeyCode::KEYCODE_EQUALS },
    { SDLK_GREATER, KeyCode::KEYCODE_GREATER },
    { SDLK_QUESTION, KeyCode::KEYCODE_QUESTION },
    { SDLK_AT, KeyCode::KEYCODE_AT },

    /*
       Skip uppercase letters
     */

    { SDLK_LEFTBRACKET, KeyCode::KEYCODE_LEFTBRACKET },
    { SDLK_BACKSLASH, KeyCode::KEYCODE_BACKSLASH },
    { SDLK_RIGHTBRACKET, KeyCode::KEYCODE_RIGHTBRACKET },
    { SDLK_CARET, KeyCode::KEYCODE_CARET },
    { SDLK_UNDERSCORE, KeyCode::KEYCODE_UNDERSCORE },
    { SDLK_BACKQUOTE, KeyCode::KEYCODE_BACKQUOTE },
    { SDLK_a, KeyCode::KEYCODE_a },
    { SDLK_b, KeyCode::KEYCODE_b },
    { SDLK_c, KeyCode::KEYCODE_c },
    { SDLK_d, KeyCode::KEYCODE_d },
    { SDLK_e, KeyCode::KEYCODE_e },
    { SDLK_f, KeyCode::KEYCODE_f },
    { SDLK_g, KeyCode::KEYCODE_g },
    { SDLK_h, KeyCode::KEYCODE_h },
    { SDLK_i, KeyCode::KEYCODE_i },
    { SDLK_j, KeyCode::KEYCODE_j },
    { SDLK_k, KeyCode::KEYCODE_k },
    { SDLK_l, KeyCode::KEYCODE_l },
    { SDLK_m, KeyCode::KEYCODE_m },
    { SDLK_n, KeyCode::KEYCODE_n },
    { SDLK_o, KeyCode::KEYCODE_o },
    { SDLK_p, KeyCode::KEYCODE_p },
    { SDLK_q, KeyCode::KEYCODE_q },
    { SDLK_r, KeyCode::KEYCODE_r },
    { SDLK_s, KeyCode::KEYCODE_s },
    { SDLK_t, KeyCode::KEYCODE_t },
    { SDLK_u, KeyCode::KEYCODE_u },
    { SDLK_v, KeyCode::KEYCODE_v },
    { SDLK_w, KeyCode::KEYCODE_w },
    { SDLK_x, KeyCode::KEYCODE_x },
    { SDLK_y, KeyCode::KEYCODE_y },
    { SDLK_z, KeyCode::KEYCODE_z },

    { SDLK_CAPSLOCK, KeyCode::KEYCODE_CAPSLOCK },

    { SDLK_F1, KeyCode::KEYCODE_F1 },
    { SDLK_F2, KeyCode::KEYCODE_F2 },
    { SDLK_F3, KeyCode::KEYCODE_F3 },
    { SDLK_F4, KeyCode::KEYCODE_F4 },
    { SDLK_F5, KeyCode::KEYCODE_F5 },
    { SDLK_F6, KeyCode::KEYCODE_F6 },
    { SDLK_F7, KeyCode::KEYCODE_F7 },
    { SDLK_F8, KeyCode::KEYCODE_F8 },
    { SDLK_F9, KeyCode::KEYCODE_F9 },
    { SDLK_F10, KeyCode::KEYCODE_F10 },
    { SDLK_F11, KeyCode::KEYCODE_F11 },
    { SDLK_F12, KeyCode::KEYCODE_F12 },

    { SDLK_PRINTSCREEN, KeyCode::KEYCODE_PRINTSCREEN },
    { SDLK_SCROLLLOCK, KeyCode::KEYCODE_SCROLLLOCK },
    { SDLK_PAUSE, KeyCode::KEYCODE_PAUSE },
    { SDLK_INSERT, KeyCode::KEYCODE_INSERT },
    { SDLK_HOME, KeyCode::KEYCODE_HOME },
    { SDLK_PAGEUP, KeyCode::KEYCODE_PAGEUP },
    { SDLK_DELETE, KeyCode::KEYCODE_DELETE },
    { SDLK_END, KeyCode::KEYCODE_END },
    { SDLK_PAGEDOWN, KeyCode::KEYCODE_PAGEDOWN },
    { SDLK_RIGHT, KeyCode::KEYCODE_RIGHT },
    { SDLK_LEFT, KeyCode::KEYCODE_LEFT },
    { SDLK_DOWN, KeyCode::KEYCODE_DOWN },
    { SDLK_UP, KeyCode::KEYCODE_UP },

    { SDLK_NUMLOCKCLEAR, KeyCode::KEYCODE_NUMLOCKCLEAR },
    { SDLK_KP_DIVIDE, KeyCode::KEYCODE_KP_DIVIDE },
    { SDLK_KP_MULTIPLY, KeyCode::KEYCODE_KP_MULTIPLY },
    { SDLK_KP_MINUS, KeyCode::KEYCODE_KP_MINUS },
    { SDLK_KP_PLUS, KeyCode::KEYCODE_KP_PLUS },
    { SDLK_KP_ENTER, KeyCode::KEYCODE_KP_ENTER },
    { SDLK_KP_1, KeyCode::KEYCODE_KP_1 },
    { SDLK_KP_2, KeyCode::KEYCODE_KP_2 },
    { SDLK_KP_3, KeyCode::KEYCODE_KP_3 },
    { SDLK_KP_4, KeyCode::KEYCODE_KP_4 },
    { SDLK_KP_5, KeyCode::KEYCODE_KP_5 },
    { SDLK_KP_6, KeyCode::KEYCODE_KP_6 },
    { SDLK_KP_7, KeyCode::KEYCODE_KP_7 },
    { SDLK_KP_8, KeyCode::KEYCODE_KP_8 },
    { SDLK_KP_9, KeyCode::KEYCODE_KP_9 },
    { SDLK_KP_0, KeyCode::KEYCODE_KP_0 },
    { SDLK_KP_PERIOD, KeyCode::KEYCODE_KP_PERIOD },

    { SDLK_APPLICATION, KeyCode::KEYCODE_APPLICATION },
    { SDLK_POWER, KeyCode::KEYCODE_POWER },
    { SDLK_KP_EQUALS, KeyCode::KEYCODE_KP_EQUALS },
    { SDLK_F13, KeyCode::KEYCODE_F13 },
    { SDLK_F14, KeyCode::KEYCODE_F14 },
    { SDLK_F15, KeyCode::KEYCODE_F15 },
    { SDLK_F16, KeyCode::KEYCODE_F16 },
    { SDLK_F17, KeyCode::KEYCODE_F17 },
    { SDLK_F18, KeyCode::KEYCODE_F18 },
    { SDLK_F19, KeyCode::KEYCODE_F19 },
    { SDLK_F20, KeyCode::KEYCODE_F20 },
    { SDLK_F21, KeyCode::KEYCODE_F21 },
    { SDLK_F22, KeyCode::KEYCODE_F22 },
    { SDLK_F23, KeyCode::KEYCODE_F23 },
    { SDLK_F24, KeyCode::KEYCODE_F24 },
    { SDLK_EXECUTE, KeyCode::KEYCODE_EXECUTE },
    { SDLK_HELP, KeyCode::KEYCODE_HELP },
    { SDLK_MENU, KeyCode::KEYCODE_MENU },
    { SDLK_SELECT, KeyCode::KEYCODE_SELECT },
    { SDLK_STOP, KeyCode::KEYCODE_STOP },
    { SDLK_AGAIN, KeyCode::KEYCODE_AGAIN },
    { SDLK_UNDO, KeyCode::KEYCODE_UNDO },
    { SDLK_CUT, KeyCode::KEYCODE_CUT },
    { SDLK_COPY, KeyCode::KEYCODE_COPY },
    { SDLK_PASTE, KeyCode::KEYCODE_PASTE },
    { SDLK_FIND, KeyCode::KEYCODE_FIND },
    { SDLK_MUTE, KeyCode::KEYCODE_MUTE },
    { SDLK_VOLUMEUP, KeyCode::KEYCODE_VOLUMEUP },
    { SDLK_VOLUMEDOWN, KeyCode::KEYCODE_VOLUMEDOWN },
    { SDLK_KP_COMMA, KeyCode::KEYCODE_KP_COMMA },
    { SDLK_KP_EQUALSAS400, KeyCode::KEYCODE_KP_EQUALSAS400 },

    { SDLK_ALTERASE, KeyCode::KEYCODE_ALTERASE },
    { SDLK_SYSREQ, KeyCode::KEYCODE_SYSREQ },
    { SDLK_CANCEL, KeyCode::KEYCODE_CANCEL },
    { SDLK_CLEAR, KeyCode::KEYCODE_CLEAR },
    { SDLK_PRIOR, KeyCode::KEYCODE_PRIOR },
    { SDLK_RETURN2, KeyCode::KEYCODE_RETURN2 },
    { SDLK_SEPARATOR, KeyCode::KEYCODE_SEPARATOR },
    { SDLK_OUT, KeyCode::KEYCODE_OUT },
    { SDLK_OPER, KeyCode::KEYCODE_OPER },
    { SDLK_CLEARAGAIN, KeyCode::KEYCODE_CLEARAGAIN },
    { SDLK_CRSEL, KeyCode::KEYCODE_CRSEL },
    { SDLK_EXSEL, KeyCode::KEYCODE_EXSEL },

    { SDLK_KP_00, KeyCode::KEYCODE_KP_00 },
    { SDLK_KP_000, KeyCode::KEYCODE_KP_000 },
    { SDLK_THOUSANDSSEPARATOR, KeyCode::KEYCODE_THOUSANDSSEPARATOR },
    { SDLK_DECIMALSEPARATOR, KeyCode::KEYCODE_DECIMALSEPARATOR },
    { SDLK_CURRENCYUNIT, KeyCode::KEYCODE_CURRENCYUNIT },
    { SDLK_CURRENCYSUBUNIT, KeyCode::KEYCODE_CURRENCYSUBUNIT },
    { SDLK_KP_LEFTPAREN, KeyCode::KEYCODE_KP_LEFTPAREN },
    { SDLK_KP_RIGHTPAREN, KeyCode::KEYCODE_KP_RIGHTPAREN },
    { SDLK_KP_LEFTBRACE, KeyCode::KEYCODE_KP_LEFTBRACE },
    { SDLK_KP_RIGHTBRACE, KeyCode::KEYCODE_KP_RIGHTBRACE },
    { SDLK_KP_TAB, KeyCode::KEYCODE_KP_TAB },
    { SDLK_KP_BACKSPACE, KeyCode::KEYCODE_KP_BACKSPACE },
    { SDLK_KP_A, KeyCode::KEYCODE_KP_A },
    { SDLK_KP_B, KeyCode::KEYCODE_KP_B },
    { SDLK_KP_C, KeyCode::KEYCODE_KP_C },
    { SDLK_KP_D, KeyCode::KEYCODE_KP_D },
    { SDLK_KP_E, KeyCode::KEYCODE_KP_E },
    { SDLK_KP_F, KeyCode::KEYCODE_KP_F },
    { SDLK_KP_XOR, KeyCode::KEYCODE_KP_XOR },
    { SDLK_KP_POWER, KeyCode::KEYCODE_KP_POWER },
    { SDLK_KP_PERCENT, KeyCode::KEYCODE_KP_PERCENT },
    { SDLK_KP_LESS, KeyCode::KEYCODE_KP_LESS },
    { SDLK_KP_GREATER, KeyCode::KEYCODE_KP_GREATER },
    { SDLK_KP_AMPERSAND, KeyCode::KEYCODE_KP_AMPERSAND },
    { SDLK_KP_DBLAMPERSAND, KeyCode::KEYCODE_KP_DBLAMPERSAND },
    { SDLK_KP_COLON, KeyCode::KEYCODE_KP_COLON },
    { SDLK_KP_HASH, KeyCode::KEYCODE_KP_HASH },
    { SDLK_KP_SPACE, KeyCode::KEYCODE_KP_SPACE },
    { SDLK_KP_AT, KeyCode::KEYCODE_KP_AT },
    { SDLK_KP_EXCLAM, KeyCode::KEYCODE_KP_EXCLAIM },
    { SDLK_KP_MEMSTORE, KeyCode::KEYCODE_KP_MEMSTORE },
    { SDLK_KP_MEMRECALL, KeyCode::KEYCODE_KP_MEMRECALL },
    { SDLK_KP_MEMCLEAR, KeyCode::KEYCODE_KP_MEMCLEAR },
    { SDLK_KP_MEMADD, KeyCode::KEYCODE_KP_MEMADD },
    { SDLK_KP_MEMSUBTRACT, KeyCode::KEYCODE_KP_MEMSUBTRACT },
    { SDLK_KP_MEMMULTIPLY, KeyCode::KEYCODE_KP_MEMMULTIPLY },
    { SDLK_KP_MEMDIVIDE, KeyCode::KEYCODE_KP_MEMDIVIDE },
    { SDLK_KP_PLUSMINUS, KeyCode::KEYCODE_KP_PLUSMINUS },
    { SDLK_KP_CLEAR, KeyCode::KEYCODE_KP_CLEAR },
    { SDLK_KP_CLEARENTRY, KeyCode::KEYCODE_KP_CLEARENTRY },
    { SDLK_KP_BINARY, KeyCode::KEYCODE_KP_BINARY },
    { SDLK_KP_OCTAL, KeyCode::KEYCODE_KP_OCTAL },
    { SDLK_KP_DECIMAL, KeyCode::KEYCODE_KP_DECIMAL },
    { SDLK_KP_HEXADECIMAL, KeyCode::KEYCODE_KP_HEXADECIMAL },

    { SDLK_LCTRL, KeyCode::KEYCODE_LCTRL },
    { SDLK_LSHIFT, KeyCode::KEYCODE_LSHIFT },
    { SDLK_LALT, KeyCode::KEYCODE_LALT },
    { SDLK_LGUI, KeyCode::KEYCODE_LGUI },
    { SDLK_RCTRL, KeyCode::KEYCODE_RCTRL },
    { SDLK_RSHIFT, KeyCode::KEYCODE_RSHIFT },
    { SDLK_RALT, KeyCode::KEYCODE_RALT },
    { SDLK_RGUI, KeyCode::KEYCODE_RGUI },

    { SDLK_MODE, KeyCode::KEYCODE_MODE },

    { SDLK_AUDIONEXT, KeyCode::KEYCODE_AUDIONEXT },
    { SDLK_AUDIOPREV, KeyCode::KEYCODE_AUDIOPREV },
    { SDLK_AUDIOSTOP, KeyCode::KEYCODE_AUDIOSTOP },
    { SDLK_AUDIOPLAY, KeyCode::KEYCODE_AUDIOPLAY },
    { SDLK_AUDIOMUTE, KeyCode::KEYCODE_AUDIOMUTE },
    { SDLK_MEDIASELECT, KeyCode::KEYCODE_MEDIASELECT },
    { SDLK_WWW, KeyCode::KEYCODE_WWW },
    { SDLK_MAIL, KeyCode::KEYCODE_MAIL },
    { SDLK_CALCULATOR, KeyCode::KEYCODE_CALCULATOR },
    { SDLK_COMPUTER, KeyCode::KEYCODE_COMPUTER },
    { SDLK_AC_SEARCH, KeyCode::KEYCODE_AC_SEARCH },
    { SDLK_AC_SEARCH, KeyCode::KEYCODE_AC_SEARCH },
    { SDLK_AC_HOME, KeyCode::KEYCODE_AC_HOME },
    { SDLK_AC_BACK, KeyCode::KEYCODE_AC_BACK },
    { SDLK_AC_FORWARD, KeyCode::KEYCODE_AC_FORWARD },
    { SDLK_AC_STOP, KeyCode::KEYCODE_AC_STOP },
    { SDLK_AC_REFRESH, KeyCode::KEYCODE_AC_REFRESH },
    { SDLK_AC_BOOKMARKS, KeyCode::KEYCODE_AC_BOOKMARKS },

    { SDLK_BRIGHTNESSDOWN, KeyCode::KEYCODE_BRIGHTNESSDOWN },
    { SDLK_BRIGHTNESSUP, KeyCode::KEYCODE_BRIGHTNESSUP },
    { SDLK_DISPLAYSWITCH, KeyCode::KEYCODE_DISPLAYSWITCH },
    { SDLK_KBDILLUMTOGGLE, KeyCode::KEYCODE_KBDILLUMTOGGLE },
    { SDLK_KBDILLUMDOWN, KeyCode::KEYCODE_KBDILLUMDOWN },
    { SDLK_KBDILLUMUP, KeyCode::KEYCODE_KBDILLUMUP },
    { SDLK_EJECT, KeyCode::KEYCODE_EJECT },
    { SDLK_SLEEP, KeyCode::KEYCODE_SLEEP },
  };
};