#pragma once
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

  bool m_isRunning = true;
};

/*
  MULTITHREADING BRAINDUMP:
  - Create some kind of async commands system for worker thread:
    - Decide which tasks should be done asynchronously
      - Asset loading (models, textures)
      - Upload command recording and queue submission
      - Thread-safe channel for sending commands to worker thread
  - Set up build options for single-threaded vs. multithreaded:
    - If multithreaded then create separate logger object that worker thread
      can "grab"? Or all threads including main thread have to share one logger
      which is protected with mutex?
    - Would need to create different preprocessor versions of worker thread
      command recording (e.g. calling a function like "loadModelAsync" should
      automatically run as if single-threaded if multithreading is disabled in
      build options)
  - Create thread pool system so that program can easy scale to as many threads
    as there are cores on CPU:
    - Would need to set up condition variables and other shit to put threads to
      sleep and reactivate
*/