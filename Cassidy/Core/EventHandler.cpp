#include "EventHandler.h"
#include <SDL_events.h>

void cassidy::EventHandler::init()
{
  InputHandler::init();
}

void cassidy::EventHandler::processEvent(SDL_Event* sdlEvent)
{
  switch (sdlEvent->type)
  {
  case SDL_KEYDOWN:
    InputHandler::setKeyDown(sdlEvent->key.keysym.sym);
    break;
  case SDL_KEYUP:
    InputHandler::setKeyUp(sdlEvent->key.keysym.sym);
    break;
  case SDL_MOUSEMOTION:
    InputHandler::setCursorMovement(sdlEvent->motion);
    break;
  }
}
