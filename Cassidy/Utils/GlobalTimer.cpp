#include "GlobalTimer.h"

#include <SDL.h>

void GlobalTimer::updateGlobalTimerImpl()
{
  uint64_t currentTimeMS = SDL_GetTicks64();
  const double currentTimeSecs = static_cast<double>(currentTimeMS) * 0.001;

  m_deltaTimeSecs = static_cast<float>(currentTimeSecs - m_engineTimeSecs);
  m_engineTimeSecs = currentTimeSecs;
}
