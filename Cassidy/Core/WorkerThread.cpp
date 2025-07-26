#include "WorkerThread.h"

void WorkerThread::init()
{
	m_thread = std::thread(&WorkerThread::tryAcquireJob);
}

void WorkerThread::pushJob(std::function<void> job, ...)
{
}

void WorkerThread::tryAcquireJob()
{
	m_queueMutex.lock();

	if (!m_highPrioJobQueue.empty())
	{
		m_highPrioJobQueue.front();
	}
}
