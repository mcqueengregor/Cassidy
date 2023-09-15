#include "GlobalTimer.h"

#include <SDL.h>

void GlobalTimer::updateGlobalTimerImpl()
{
  uint64_t currentTimeMS = SDL_GetTicks64();
  const double currentTimeSecs = currentTimeMS * 0.001;

  m_deltaTimeSecs = currentTimeSecs - m_engineTimeSecs;
  m_engineTimeSecs = currentTimeSecs;
}
