#include "InputHandler.h"

#include <iostream>

void InputHandler::initImpl()
{
  for (uint16_t i = 0; i < KEYBOARD_SIZE; ++i)
  {
    m_keyboardStates[i] = false;
    m_prevKeyboardStates[i] = false;

    m_keyboardDowns[i] = false;
    m_keyboardUps[i] = false;
  }
}

void InputHandler::updateKeyStatesImpl()
{
  for (uint16_t i = 0; i < KEYBOARD_SIZE; ++i)
  {
    // Detect change between current frame and previous frame's inputs:
    bool keyChange = m_keyboardStates[i] ^ m_prevKeyboardStates[i];

    m_prevKeyboardStates[i] = m_keyboardStates[i];

    // Log new keyboard states for this frame:
    m_keyboardDowns[i] = keyChange && m_keyboardStates[i];
    m_keyboardUps[i] = keyChange && (!m_keyboardStates[i]);
  }
}

void InputHandler::setKeyDownImpl(SDL_Keycode keyCode)
{
  std::cout << "Pressed " << SDL_GetKeyName(keyCode) << " down!" << std::endl;
  Keycode convertedCode = KEYCODE_CONVERSION_TABLE.at(keyCode);

  m_keyboardStates[static_cast<uint16_t>(convertedCode)] = true;
}

void InputHandler::setKeyUpImpl(SDL_Keycode keyCode)
{
  std::cout << "Released " << SDL_GetKeyName(keyCode) << "!" << std::endl;
  Keycode convertedCode = KEYCODE_CONVERSION_TABLE.at(keyCode);

  m_keyboardStates[static_cast<uint16_t>(convertedCode)] = false;
}

bool InputHandler::isKeyPressedImpl(Keycode keyCode)
{
  return m_keyboardDowns[static_cast<uint16_t>(keyCode)];
}

bool InputHandler::isKeyHeldImpl(Keycode keyCode)
{
  return m_keyboardStates[static_cast<uint16_t>(keyCode)];
}

bool InputHandler::isKeyReleasedImpl(Keycode keyCode)
{
  return m_keyboardUps[static_cast<uint16_t>(keyCode)];
}

bool InputHandler::isKeyUpImpl(Keycode keyCode)
{
  return !m_keyboardStates[static_cast<uint16_t>(keyCode)];
}
