#pragma once

#include <SDL_mouse.h>

enum class MouseCode : uint8_t
{
  MOUSECODE_LEFT = SDL_BUTTON_LMASK,
  MOUSECODE_MIDDLE = SDL_BUTTON_MMASK,
  MOUSECODE_RIGHT = SDL_BUTTON_RMASK,
  MOUSECODE_X1 = SDL_BUTTON_X1MASK,
  MOUSECODE_X2 = SDL_BUTTON_X2MASK,
};