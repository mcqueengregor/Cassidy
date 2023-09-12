#include "EventHandler.h"
#include <SDL.h>

void EventHandler::init()
{
  m_inputHandler.init();
}

void EventHandler::processEvent(SDL_Event* event)
{
  switch (event->type)
  {

  }
}
