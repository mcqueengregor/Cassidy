#pragma once

#include "Utils/Keycode.h"
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
  static inline bool isKeyPressed(Keycode keyCode)   { return InputHandler::get().isKeyPressedImpl(keyCode); }
  static inline bool isKeyHeld(Keycode keyCode)      { return InputHandler::get().isKeyHeldImpl(keyCode); }
  static inline bool isKeyReleased(Keycode keyCode)  { return InputHandler::get().isKeyReleasedImpl(keyCode); }
  static inline bool isKeyUp(Keycode keyCode)        { return InputHandler::get().isKeyUpImpl(keyCode); }

private:
  InputHandler() {}
  
  void initImpl();
  void updateKeyStatesImpl();

  void setKeyDownImpl(SDL_Keycode keyCode);
  void setKeyUpImpl(SDL_Keycode keyCode);

  bool isKeyPressedImpl(Keycode keyCode);
  bool isKeyHeldImpl(Keycode keyCode);
  bool isKeyReleasedImpl(Keycode keyCode);
  bool isKeyUpImpl(Keycode keyCode);

  // Keyboard states:
  static const uint16_t KEYBOARD_SIZE = SDL_TRANSFORM_KEYCODE_TO_NEW_RANGE(SDLK_SLEEP);

  bool m_keyboardStates[KEYBOARD_SIZE];
  bool m_prevKeyboardStates[KEYBOARD_SIZE];

  bool m_keyboardDowns[KEYBOARD_SIZE];
  bool m_keyboardUps[KEYBOARD_SIZE];

  const std::unordered_map<SDL_Keycode, Keycode> KEYCODE_CONVERSION_TABLE = {
    { SDLK_UNKNOWN, Keycode::KEYCODE_UNKNOWN },

    { SDLK_RETURN, Keycode::KEYCODE_RETURN },
    { SDLK_ESCAPE, Keycode::KEYCODE_ESCAPE },
    { SDLK_BACKSPACE, Keycode::KEYCODE_BACKSPACE },
    { SDLK_TAB, Keycode::KEYCODE_TAB },
    { SDLK_SPACE, Keycode::KEYCODE_SPACE },
    { SDLK_EXCLAIM, Keycode::KEYCODE_EXCLAIM },
    { SDLK_QUOTEDBL, Keycode::KEYCODE_QUOTEDBL },
    { SDLK_HASH, Keycode::KEYCODE_HASH },
    { SDLK_PERCENT, Keycode::KEYCODE_PERCENT },
    { SDLK_DOLLAR, Keycode::KEYCODE_DOLLAR },
    { SDLK_AMPERSAND, Keycode::KEYCODE_AMPERSAND },
    { SDLK_QUOTE, Keycode::KEYCODE_QUOTE },
    { SDLK_LEFTPAREN, Keycode::KEYCODE_LEFTPAREN },
    { SDLK_RIGHTPAREN, Keycode::KEYCODE_RIGHTPAREN },
    { SDLK_ASTERISK, Keycode::KEYCODE_ASTERISK },
    { SDLK_PLUS, Keycode::KEYCODE_PLUS },
    { SDLK_COMMA, Keycode::KEYCODE_COMMA },
    { SDLK_MINUS, Keycode::KEYCODE_MINUS },
    { SDLK_PERIOD, Keycode::KEYCODE_PERIOD },
    { SDLK_SLASH, Keycode::KEYCODE_SLASH },
    { SDLK_0, Keycode::KEYCODE_0 },
    { SDLK_1, Keycode::KEYCODE_1 },
    { SDLK_2, Keycode::KEYCODE_2 },
    { SDLK_3, Keycode::KEYCODE_3 },
    { SDLK_4, Keycode::KEYCODE_4 },
    { SDLK_5, Keycode::KEYCODE_5 },
    { SDLK_6, Keycode::KEYCODE_6 },
    { SDLK_7, Keycode::KEYCODE_7 },
    { SDLK_8, Keycode::KEYCODE_8 },
    { SDLK_9, Keycode::KEYCODE_9 },
    { SDLK_COLON, Keycode::KEYCODE_COLON },
    { SDLK_SEMICOLON, Keycode::KEYCODE_SEMICOLON },
    { SDLK_LESS, Keycode::KEYCODE_LESS },
    { SDLK_EQUALS, Keycode::KEYCODE_EQUALS },
    { SDLK_GREATER, Keycode::KEYCODE_GREATER },
    { SDLK_QUESTION, Keycode::KEYCODE_QUESTION },
    { SDLK_AT, Keycode::KEYCODE_AT },

    /*
       Skip uppercase letters
     */

    { SDLK_LEFTBRACKET, Keycode::KEYCODE_LEFTBRACKET },
    { SDLK_BACKSLASH, Keycode::KEYCODE_BACKSLASH },
    { SDLK_RIGHTBRACKET, Keycode::KEYCODE_RIGHTBRACKET },
    { SDLK_CARET, Keycode::KEYCODE_CARET },
    { SDLK_UNDERSCORE, Keycode::KEYCODE_UNDERSCORE },
    { SDLK_BACKQUOTE, Keycode::KEYCODE_BACKQUOTE },
    { SDLK_a, Keycode::KEYCODE_a },
    { SDLK_b, Keycode::KEYCODE_b },
    { SDLK_c, Keycode::KEYCODE_c },
    { SDLK_d, Keycode::KEYCODE_d },
    { SDLK_e, Keycode::KEYCODE_e },
    { SDLK_f, Keycode::KEYCODE_f },
    { SDLK_g, Keycode::KEYCODE_g },
    { SDLK_h, Keycode::KEYCODE_h },
    { SDLK_i, Keycode::KEYCODE_i },
    { SDLK_j, Keycode::KEYCODE_j },
    { SDLK_k, Keycode::KEYCODE_k },
    { SDLK_l, Keycode::KEYCODE_l },
    { SDLK_m, Keycode::KEYCODE_m },
    { SDLK_n, Keycode::KEYCODE_n },
    { SDLK_o, Keycode::KEYCODE_o },
    { SDLK_p, Keycode::KEYCODE_p },
    { SDLK_q, Keycode::KEYCODE_q },
    { SDLK_r, Keycode::KEYCODE_r },
    { SDLK_s, Keycode::KEYCODE_s },
    { SDLK_t, Keycode::KEYCODE_t },
    { SDLK_u, Keycode::KEYCODE_u },
    { SDLK_v, Keycode::KEYCODE_v },
    { SDLK_w, Keycode::KEYCODE_w },
    { SDLK_x, Keycode::KEYCODE_x },
    { SDLK_y, Keycode::KEYCODE_y },
    { SDLK_z, Keycode::KEYCODE_z },

    { SDLK_CAPSLOCK, Keycode::KEYCODE_CAPSLOCK },

    { SDLK_F1, Keycode::KEYCODE_F1 },
    { SDLK_F2, Keycode::KEYCODE_F2 },
    { SDLK_F3, Keycode::KEYCODE_F3 },
    { SDLK_F4, Keycode::KEYCODE_F4 },
    { SDLK_F5, Keycode::KEYCODE_F5 },
    { SDLK_F6, Keycode::KEYCODE_F6 },
    { SDLK_F7, Keycode::KEYCODE_F7 },
    { SDLK_F8, Keycode::KEYCODE_F8 },
    { SDLK_F9, Keycode::KEYCODE_F9 },
    { SDLK_F10, Keycode::KEYCODE_F10 },
    { SDLK_F11, Keycode::KEYCODE_F11 },
    { SDLK_F12, Keycode::KEYCODE_F12 },

    { SDLK_PRINTSCREEN, Keycode::KEYCODE_PRINTSCREEN },
    { SDLK_SCROLLLOCK, Keycode::KEYCODE_SCROLLLOCK },
    { SDLK_PAUSE, Keycode::KEYCODE_PAUSE },
    { SDLK_INSERT, Keycode::KEYCODE_INSERT },
    { SDLK_HOME, Keycode::KEYCODE_HOME },
    { SDLK_PAGEUP, Keycode::KEYCODE_PAGEUP },
    { SDLK_DELETE, Keycode::KEYCODE_DELETE },
    { SDLK_END, Keycode::KEYCODE_END },
    { SDLK_PAGEDOWN, Keycode::KEYCODE_PAGEDOWN },
    { SDLK_RIGHT, Keycode::KEYCODE_RIGHT },
    { SDLK_LEFT, Keycode::KEYCODE_LEFT },
    { SDLK_DOWN, Keycode::KEYCODE_DOWN },
    { SDLK_UP, Keycode::KEYCODE_UP },

    { SDLK_NUMLOCKCLEAR, Keycode::KEYCODE_NUMLOCKCLEAR },
    { SDLK_KP_DIVIDE, Keycode::KEYCODE_KP_DIVIDE },
    { SDLK_KP_MULTIPLY, Keycode::KEYCODE_KP_MULTIPLY },
    { SDLK_KP_MINUS, Keycode::KEYCODE_KP_MINUS },
    { SDLK_KP_PLUS, Keycode::KEYCODE_KP_PLUS },
    { SDLK_KP_ENTER, Keycode::KEYCODE_KP_ENTER },
    { SDLK_KP_1, Keycode::KEYCODE_KP_1 },
    { SDLK_KP_2, Keycode::KEYCODE_KP_2 },
    { SDLK_KP_3, Keycode::KEYCODE_KP_3 },
    { SDLK_KP_4, Keycode::KEYCODE_KP_4 },
    { SDLK_KP_5, Keycode::KEYCODE_KP_5 },
    { SDLK_KP_6, Keycode::KEYCODE_KP_6 },
    { SDLK_KP_7, Keycode::KEYCODE_KP_7 },
    { SDLK_KP_8, Keycode::KEYCODE_KP_8 },
    { SDLK_KP_9, Keycode::KEYCODE_KP_9 },
    { SDLK_KP_0, Keycode::KEYCODE_KP_0 },
    { SDLK_KP_PERIOD, Keycode::KEYCODE_KP_PERIOD },

    { SDLK_APPLICATION, Keycode::KEYCODE_APPLICATION },
    { SDLK_POWER, Keycode::KEYCODE_POWER },
    { SDLK_KP_EQUALS, Keycode::KEYCODE_KP_EQUALS },
    { SDLK_F13, Keycode::KEYCODE_F13 },
    { SDLK_F14, Keycode::KEYCODE_F14 },
    { SDLK_F15, Keycode::KEYCODE_F15 },
    { SDLK_F16, Keycode::KEYCODE_F16 },
    { SDLK_F17, Keycode::KEYCODE_F17 },
    { SDLK_F18, Keycode::KEYCODE_F18 },
    { SDLK_F19, Keycode::KEYCODE_F19 },
    { SDLK_F20, Keycode::KEYCODE_F20 },
    { SDLK_F21, Keycode::KEYCODE_F21 },
    { SDLK_F22, Keycode::KEYCODE_F22 },
    { SDLK_F23, Keycode::KEYCODE_F23 },
    { SDLK_F24, Keycode::KEYCODE_F24 },
    { SDLK_EXECUTE, Keycode::KEYCODE_EXECUTE },
    { SDLK_HELP, Keycode::KEYCODE_HELP },
    { SDLK_MENU, Keycode::KEYCODE_MENU },
    { SDLK_SELECT, Keycode::KEYCODE_SELECT },
    { SDLK_STOP, Keycode::KEYCODE_STOP },
    { SDLK_AGAIN, Keycode::KEYCODE_AGAIN },
    { SDLK_UNDO, Keycode::KEYCODE_UNDO },
    { SDLK_CUT, Keycode::KEYCODE_CUT },
    { SDLK_COPY, Keycode::KEYCODE_COPY },
    { SDLK_PASTE, Keycode::KEYCODE_PASTE },
    { SDLK_FIND, Keycode::KEYCODE_FIND },
    { SDLK_MUTE, Keycode::KEYCODE_MUTE },
    { SDLK_VOLUMEUP, Keycode::KEYCODE_VOLUMEUP },
    { SDLK_VOLUMEDOWN, Keycode::KEYCODE_VOLUMEDOWN },
    { SDLK_KP_COMMA, Keycode::KEYCODE_KP_COMMA },
    { SDLK_KP_EQUALSAS400, Keycode::KEYCODE_KP_EQUALSAS400 },

    { SDLK_ALTERASE, Keycode::KEYCODE_ALTERASE },
    { SDLK_SYSREQ, Keycode::KEYCODE_SYSREQ },
    { SDLK_CANCEL, Keycode::KEYCODE_CANCEL },
    { SDLK_CLEAR, Keycode::KEYCODE_CLEAR },
    { SDLK_PRIOR, Keycode::KEYCODE_PRIOR },
    { SDLK_RETURN2, Keycode::KEYCODE_RETURN2 },
    { SDLK_SEPARATOR, Keycode::KEYCODE_SEPARATOR },
    { SDLK_OUT, Keycode::KEYCODE_OUT },
    { SDLK_OPER, Keycode::KEYCODE_OPER },
    { SDLK_CLEARAGAIN, Keycode::KEYCODE_CLEARAGAIN },
    { SDLK_CRSEL, Keycode::KEYCODE_CRSEL },
    { SDLK_EXSEL, Keycode::KEYCODE_EXSEL },

    { SDLK_KP_00, Keycode::KEYCODE_KP_00 },
    { SDLK_KP_000, Keycode::KEYCODE_KP_000 },
    { SDLK_THOUSANDSSEPARATOR, Keycode::KEYCODE_THOUSANDSSEPARATOR },
    { SDLK_DECIMALSEPARATOR, Keycode::KEYCODE_DECIMALSEPARATOR },
    { SDLK_CURRENCYUNIT, Keycode::KEYCODE_CURRENCYUNIT },
    { SDLK_CURRENCYSUBUNIT, Keycode::KEYCODE_CURRENCYSUBUNIT },
    { SDLK_KP_LEFTPAREN, Keycode::KEYCODE_KP_LEFTPAREN },
    { SDLK_KP_RIGHTPAREN, Keycode::KEYCODE_KP_RIGHTPAREN },
    { SDLK_KP_LEFTBRACE, Keycode::KEYCODE_KP_LEFTBRACE },
    { SDLK_KP_RIGHTBRACE, Keycode::KEYCODE_KP_RIGHTBRACE },
    { SDLK_KP_TAB, Keycode::KEYCODE_KP_TAB },
    { SDLK_KP_BACKSPACE, Keycode::KEYCODE_KP_BACKSPACE },
    { SDLK_KP_A, Keycode::KEYCODE_KP_A },
    { SDLK_KP_B, Keycode::KEYCODE_KP_B },
    { SDLK_KP_C, Keycode::KEYCODE_KP_C },
    { SDLK_KP_D, Keycode::KEYCODE_KP_D },
    { SDLK_KP_E, Keycode::KEYCODE_KP_E },
    { SDLK_KP_F, Keycode::KEYCODE_KP_F },
    { SDLK_KP_XOR, Keycode::KEYCODE_KP_XOR },
    { SDLK_KP_POWER, Keycode::KEYCODE_KP_POWER },
    { SDLK_KP_PERCENT, Keycode::KEYCODE_KP_PERCENT },
    { SDLK_KP_LESS, Keycode::KEYCODE_KP_LESS },
    { SDLK_KP_GREATER, Keycode::KEYCODE_KP_GREATER },
    { SDLK_KP_AMPERSAND, Keycode::KEYCODE_KP_AMPERSAND },
    { SDLK_KP_DBLAMPERSAND, Keycode::KEYCODE_KP_DBLAMPERSAND },
    { SDLK_KP_COLON, Keycode::KEYCODE_KP_COLON },
    { SDLK_KP_HASH, Keycode::KEYCODE_KP_HASH },
    { SDLK_KP_SPACE, Keycode::KEYCODE_KP_SPACE },
    { SDLK_KP_AT, Keycode::KEYCODE_KP_AT },
    { SDLK_KP_EXCLAM, Keycode::KEYCODE_KP_EXCLAIM },
    { SDLK_KP_MEMSTORE, Keycode::KEYCODE_KP_MEMSTORE },
    { SDLK_KP_MEMRECALL, Keycode::KEYCODE_KP_MEMRECALL },
    { SDLK_KP_MEMCLEAR, Keycode::KEYCODE_KP_MEMCLEAR },
    { SDLK_KP_MEMADD, Keycode::KEYCODE_KP_MEMADD },
    { SDLK_KP_MEMSUBTRACT, Keycode::KEYCODE_KP_MEMSUBTRACT },
    { SDLK_KP_MEMMULTIPLY, Keycode::KEYCODE_KP_MEMMULTIPLY },
    { SDLK_KP_MEMDIVIDE, Keycode::KEYCODE_KP_MEMDIVIDE },
    { SDLK_KP_PLUSMINUS, Keycode::KEYCODE_KP_PLUSMINUS },
    { SDLK_KP_CLEAR, Keycode::KEYCODE_KP_CLEAR },
    { SDLK_KP_CLEARENTRY, Keycode::KEYCODE_KP_CLEARENTRY },
    { SDLK_KP_BINARY, Keycode::KEYCODE_KP_BINARY },
    { SDLK_KP_OCTAL, Keycode::KEYCODE_KP_OCTAL },
    { SDLK_KP_DECIMAL, Keycode::KEYCODE_KP_DECIMAL },
    { SDLK_KP_HEXADECIMAL, Keycode::KEYCODE_KP_HEXADECIMAL },

    { SDLK_LCTRL, Keycode::KEYCODE_LCTRL },
    { SDLK_LSHIFT, Keycode::KEYCODE_LSHIFT },
    { SDLK_LALT, Keycode::KEYCODE_LALT },
    { SDLK_LGUI, Keycode::KEYCODE_LGUI },
    { SDLK_RCTRL, Keycode::KEYCODE_RCTRL },
    { SDLK_RSHIFT, Keycode::KEYCODE_RSHIFT },
    { SDLK_RALT, Keycode::KEYCODE_RALT },
    { SDLK_RGUI, Keycode::KEYCODE_RGUI },

    { SDLK_MODE, Keycode::KEYCODE_MODE },

    { SDLK_AUDIONEXT, Keycode::KEYCODE_AUDIONEXT },
    { SDLK_AUDIOPREV, Keycode::KEYCODE_AUDIOPREV },
    { SDLK_AUDIOSTOP, Keycode::KEYCODE_AUDIOSTOP },
    { SDLK_AUDIOPLAY, Keycode::KEYCODE_AUDIOPLAY },
    { SDLK_AUDIOMUTE, Keycode::KEYCODE_AUDIOMUTE },
    { SDLK_MEDIASELECT, Keycode::KEYCODE_MEDIASELECT },
    { SDLK_WWW, Keycode::KEYCODE_WWW },
    { SDLK_MAIL, Keycode::KEYCODE_MAIL },
    { SDLK_CALCULATOR, Keycode::KEYCODE_CALCULATOR },
    { SDLK_COMPUTER, Keycode::KEYCODE_COMPUTER },
    { SDLK_AC_SEARCH, Keycode::KEYCODE_AC_SEARCH },
    { SDLK_AC_SEARCH, Keycode::KEYCODE_AC_SEARCH },
    { SDLK_AC_HOME, Keycode::KEYCODE_AC_HOME },
    { SDLK_AC_BACK, Keycode::KEYCODE_AC_BACK },
    { SDLK_AC_FORWARD, Keycode::KEYCODE_AC_FORWARD },
    { SDLK_AC_STOP, Keycode::KEYCODE_AC_STOP },
    { SDLK_AC_REFRESH, Keycode::KEYCODE_AC_REFRESH },
    { SDLK_AC_BOOKMARKS, Keycode::KEYCODE_AC_BOOKMARKS },

    { SDLK_BRIGHTNESSDOWN, Keycode::KEYCODE_BRIGHTNESSDOWN },
    { SDLK_BRIGHTNESSUP, Keycode::KEYCODE_BRIGHTNESSUP },
    { SDLK_DISPLAYSWITCH, Keycode::KEYCODE_DISPLAYSWITCH },
    { SDLK_KBDILLUMTOGGLE, Keycode::KEYCODE_KBDILLUMTOGGLE },
    { SDLK_KBDILLUMDOWN, Keycode::KEYCODE_KBDILLUMDOWN },
    { SDLK_KBDILLUMUP, Keycode::KEYCODE_KBDILLUMUP },
    { SDLK_EJECT, Keycode::KEYCODE_EJECT },
    { SDLK_SLEEP, Keycode::KEYCODE_SLEEP },
  };
};