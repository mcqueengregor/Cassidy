#pragma once

class GlobalTimer
{
public:
  GlobalTimer(const GlobalTimer&) = delete; // Prevent copy instructions of this singleton.
  static GlobalTimer& get()
  {
    static GlobalTimer s_instance;
    return s_instance;
  }

  static inline void updateGlobalTimer() { GlobalTimer::get().updateGlobalTimerImpl(); }

  static inline float deltaTime()   { return GlobalTimer::get().m_deltaTimeSecs; }
  static inline double engineTime() { return GlobalTimer::get().m_engineTimeSecs; }

private:
  GlobalTimer() : 
    m_engineTimeSecs(0.0),
    m_deltaTimeSecs(0.0f)
  {}

  void updateGlobalTimerImpl();

  double m_engineTimeSecs;
  float m_deltaTimeSecs;
};

