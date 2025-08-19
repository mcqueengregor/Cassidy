#include "WorkerThread.h"
#include <Core/Logger.h>

void WorkerThread::init()
{
	m_thread = std::thread(&WorkerThread::tryAcquireJob, this);
}

void WorkerThread::release()
{
	m_isRunning = false;
	m_condVar.notify_all();
	m_thread.join();
	CS_LOG_INFO("Worker thread joined!");
}

void WorkerThread::pushJobLowPrio(std::function<void()> jobLambda)
{
	std::lock_guard<std::mutex> lowPrioLockGuard(m_emptyQueuesMutex);
	m_lowPrioJobQueue.queue.push(jobLambda);
	m_condVar.notify_one();
}

void WorkerThread::pushJobHighPrio(std::function<void()> jobLambda)
{
	std::lock_guard<std::mutex> highPrioLockGuard(m_emptyQueuesMutex);
	m_highPrioJobQueue.queue.push(jobLambda);
	m_condVar.notify_one();
}

void WorkerThread::tryAcquireJob()
{
	while (m_isRunning)
	{
		bool areBothQueuesEmpty = true;

		if (!m_highPrioJobQueue.queue.empty())
		{
			areBothQueuesEmpty = false;

			std::function<void()> highPrioFunc;
			{
				// Block new jobs from being pushed until acquired job is popped:
				std::unique_lock highPrioLock(m_lowPrioJobQueue.blockAddToQueueMutex);
				highPrioFunc = m_highPrioJobQueue.queue.front();
				m_highPrioJobQueue.queue.pop();
			}
			highPrioFunc();
		}
		else if (!m_lowPrioJobQueue.queue.empty())
		{
			areBothQueuesEmpty = false;

			std::function<void()> lowPrioFunc;
			{
				std::unique_lock lowPrioLock(m_highPrioJobQueue.blockAddToQueueMutex);
				lowPrioFunc = m_lowPrioJobQueue.queue.front();
				m_lowPrioJobQueue.queue.pop();
			}
			lowPrioFunc();
		}

		if (areBothQueuesEmpty)
		{
			CS_LOG_WARN("Worker thread going to sleep!");
			std::unique_lock emptyQueuesLock(m_emptyQueuesMutex);
			m_condVar.wait(emptyQueuesLock);
		}
	}
	CS_LOG_INFO("Worker thread exiting aquireJob function!");
}
