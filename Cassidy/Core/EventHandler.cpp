#include "EventHandler.h"
#include <SDL_events.h>

void cassidy::EventHandler::init()
{
  InputHandler::init();
}

void cassidy::EventHandler::processEvent(SDL_Event* event)
{
  switch (event->type)
  {
  case SDL_KEYDOWN:
    InputHandler::setKeyDown(event->key.keysym.sym);
    break;
  case SDL_KEYUP:
    InputHandler::setKeyUp(event->key.keysym.sym);
    break;
  case SDL_MOUSEMOTION:
    InputHandler::setCursorMovement(event->motion);
    break;
  }
}
