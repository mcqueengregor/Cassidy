#include "InputHandler.h"

#include <imgui/imgui_impl_sdl2.h>

void InputHandler::initImpl()
{
  m_mouseState = { 0x0000, 0x0000, 0x0000, 0x0000, 0, 0, 0, 0, 0, 0, false };  // Yuck?

  for (uint16_t i = 0; i < KEYBOARD_SIZE; ++i)
  {
    m_keyboardStates[i].keyboardState = false;
    m_keyboardStates[i].prevKeyboardState = false;

    m_keyboardStates[i].keyboardDown = false;
    m_keyboardStates[i].keyboardUp = false;
  }
}

void InputHandler::updateKeyStatesImpl()
{
  for (uint16_t i = 0; i < KEYBOARD_SIZE; ++i)
  {
    // Detect change between current frame and previous frame's keyboard inputs:
    bool keyChange = m_keyboardStates[i].keyboardState ^ m_keyboardStates[i].prevKeyboardState;

    m_keyboardStates[i].prevKeyboardState = m_keyboardStates[i].keyboardState;

    // Log new keyboard states for this frame:
    m_keyboardStates[i].keyboardDown = keyChange && m_keyboardStates[i].keyboardState;
    m_keyboardStates[i].keyboardUp = keyChange && (!m_keyboardStates[i].keyboardState);
  }
}

void InputHandler::updateMouseStatesImpl()
{
  m_mouseState.mouseState = SDL_GetMouseState(&m_mouseState.mouseRelativePositionX, &m_mouseState.mouseRelativePositionY);
  
  // Detect change between current frame and previous frame's mouse inputs:
  uint8_t buttonChange = m_mouseState.mouseState ^ m_mouseState.prevMouseState;

  m_mouseState.prevMouseState = m_mouseState.mouseState;

  // Log new mouse states for this frame:
  m_mouseState.mouseDown = buttonChange & m_mouseState.mouseState;
  m_mouseState.mouseUp = buttonChange & (~m_mouseState.mouseState);
}

void InputHandler::flushDynamicMouseStatesImpl()
{
  m_mouseState.mouseRelativeMotionX = 0;
  m_mouseState.mouseRelativeMotionY = 0;
}

void InputHandler::logMousePositionImpl()
{
  m_mouseState.loggedMouseRelativePositionX = m_mouseState.mouseRelativePositionX;
  m_mouseState.loggedMouseRelativePositionY = m_mouseState.mouseRelativePositionY;
}

void InputHandler::moveMouseToLoggedPositionImpl()
{
  SDL_WarpMouseInWindow(NULL, m_mouseState.loggedMouseRelativePositionX, m_mouseState.loggedMouseRelativePositionY);
}

void InputHandler::setKeyDownImpl(SDL_Keycode keyCode)
{
  KeyCode convertedCode = KEYCODE_CONVERSION_TABLE.at(keyCode);

  m_keyboardStates[static_cast<uint16_t>(convertedCode)].keyboardState = true;
}

void InputHandler::setKeyUpImpl(SDL_Keycode keyCode)
{
  KeyCode convertedCode = KEYCODE_CONVERSION_TABLE.at(keyCode);

  m_keyboardStates[static_cast<uint16_t>(convertedCode)].keyboardState = false;
}

void InputHandler::setMouseButtonDownImpl(int mouseCode)
{
  m_mouseState.mouseState |= SDL_BUTTON(mouseCode);
}

void InputHandler::setMouseButtonUpImpl(int mouseCode)
{
  m_mouseState.mouseState &= (~SDL_BUTTON(mouseCode));
}

void InputHandler::setMouseButtonDownImpl(MouseCode mouseCode)
{
  const uint8_t codeMask = static_cast<uint8_t>(mouseCode);

  m_mouseState.mouseState |= codeMask;
}

void InputHandler::setMouseButtonUpImpl(MouseCode mouseCode)
{
  const uint8_t codeMask = static_cast<uint8_t>(mouseCode);

  m_mouseState.mouseState &= (~codeMask);
}

void InputHandler::setCursorMovementImpl(SDL_MouseMotionEvent event)
{
  m_mouseState.mouseRelativeMotionX = event.xrel;
  m_mouseState.mouseRelativeMotionY = event.yrel;
}

void InputHandler::lockCursorImpl()
{
  m_mouseState.isCursorLocked = true;
  m_mouseState.mouseOriginalRelativePositionX = m_mouseState.mouseRelativePositionX;
  m_mouseState.mouseOriginalRelativePositionY = m_mouseState.mouseRelativePositionY;
}

void InputHandler::unlockCursorImpl()
{
  m_mouseState.isCursorLocked = false;

  SDL_WarpMouseInWindow(NULL, m_mouseState.mouseOriginalRelativePositionX, m_mouseState.mouseOriginalRelativePositionY);
}

void InputHandler::hideCursorImpl()
{
  SDL_ShowCursor(SDL_DISABLE);
}

void InputHandler::showCursorImpl()
{
  SDL_ShowCursor(SDL_ENABLE);
}

void InputHandler::centreCursorImpl(int32_t windowWidth, int32_t windowHeight)
{
  if (m_mouseState.mouseRelativePositionX != windowWidth / 2.0f
    && m_mouseState.mouseRelativePositionY != windowHeight / 2.0f)
  {
    SDL_WarpMouseInWindow(NULL,
      static_cast<int>(windowWidth / 2.0f), static_cast<int>(windowHeight / 2.0f));
  }
}

bool InputHandler::isKeyPressedImpl(KeyCode keyCode)
{
  return m_keyboardStates[static_cast<uint16_t>(keyCode)].keyboardDown;
}

bool InputHandler::isKeyHeldImpl(KeyCode keyCode)
{
  return m_keyboardStates[static_cast<uint16_t>(keyCode)].keyboardState;
}

bool InputHandler::isKeyReleasedImpl(KeyCode keyCode)
{
  return m_keyboardStates[static_cast<uint16_t>(keyCode)].keyboardUp;
}

bool InputHandler::isKeyUpImpl(KeyCode keyCode)
{
  return !m_keyboardStates[static_cast<uint16_t>(keyCode)].keyboardState;
}

bool InputHandler::isMouseButtonPressedImpl(MouseCode mouseCode)
{
  return (m_mouseState.mouseDown & static_cast<uint8_t>(mouseCode)) != 0;
}

bool InputHandler::isMouseButtonHeldImpl(MouseCode mouseCode)
{
  return (m_mouseState.mouseState & static_cast<uint8_t>(mouseCode)) != 0;
}

bool InputHandler::isMouseButtonReleasedImpl(MouseCode mouseCode)
{
  return (m_mouseState.mouseUp & static_cast<uint8_t>(mouseCode)) != 0;
}

bool InputHandler::isMouseButtonUpImpl(MouseCode mouseCode)
{
  return (m_mouseState.mouseState & static_cast<uint8_t>(mouseCode)) == 0;
}

int32_t InputHandler::getCursorPositionXImpl()
{
  return m_mouseState.mouseRelativePositionX;
}

int32_t InputHandler::getCursorPositionYImpl()
{
  return m_mouseState.mouseRelativePositionY;
}

int32_t InputHandler::getCursorOffsetXImpl()
{
  return m_mouseState.mouseRelativeMotionX;
}

int32_t InputHandler::getCursorOffsetYImpl()
{
  // Inverted since y-coords of cursor relative to window range from top to bottom:
  return -m_mouseState.mouseRelativeMotionY;
}

int32_t InputHandler::getCursorLoggedPositionXImpl()
{
    return m_mouseState.loggedMouseRelativePositionX;
}

int32_t InputHandler::getCursorLoggedPositionYImpl()
{
  return m_mouseState.loggedMouseRelativePositionY;
}
