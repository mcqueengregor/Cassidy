#pragma once
#include <Utils/Types.h>

#include <thread>
#include <mutex>
#include <condition_variable>

#include <functional>
#include <queue>

class WorkerThread
{
  struct JobQueue 
  {
    std::queue<std::function<void()>> queue;
    std::mutex blockAddToQueueMutex;
  };

public:
	void init();
  void release();

  void pushJobLowPrio(std::function<void()> jobLambda);
  void pushJobHighPrio(std::function<void()> jobLambda);

private:
	void tryAcquireJob();

	std::thread m_thread;

	std::mutex m_emptyQueuesMutex;
	std::condition_variable m_condVar;

	JobQueue m_lowPrioJobQueue;
	JobQueue m_highPrioJobQueue;

  volatile bool m_isRunning = true;
};