// Core/System/ThreadPool.cpp

#include <Core/System/ThreadPool.h>

#include <Core/Windows.h>
#include <Core/DataStructures/SimpleTypeVector.hpp>

#include <queue>
#include <algorithm>
#include <stdexcept>
#include <cstdio>

using namespace Core;

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

unsigned ThreadingHelper::GetCountRequiredThreads(unsigned countTasks, unsigned countMaximumThreads)
{
	return (countTasks < countMaximumThreads ? countTasks : countMaximumThreads);
}

void ThreadingHelper::GetTaskIndices(unsigned countTasks, unsigned countThreads, unsigned threadIndex,
	unsigned& startIndex, unsigned& endIndex)
{
	if (countThreads == 0)
	{
		startIndex = endIndex = 0;
		return;
	}

	unsigned countThreadTasks = countTasks / countThreads;
	unsigned tasksLeft = countTasks % countThreads;
	startIndex = countThreadTasks * threadIndex;
	startIndex += std::min(threadIndex, tasksLeft);
	if (threadIndex < tasksLeft)
	{
		countThreadTasks++;
	}
	endIndex = startIndex + countThreadTasks;
}

bool ThreadingHelper::ThreadTasksData::operator<(const ThreadTasksData& other) const
{
	if (WorkSize != other.WorkSize)
	{
		return (WorkSize < other.WorkSize);
	}
	return (ThreadIndex < other.ThreadIndex);
}

bool ThreadingHelper::ThreadTasksData::operator>(const ThreadTasksData& other) const
{
	if (WorkSize != other.WorkSize)
	{
		return (WorkSize > other.WorkSize);
	}
	return (ThreadIndex > other.ThreadIndex);
}

bool ThreadingHelper::TaskData::operator<(const TaskData& other) const
{
	if (TaskSize != other.TaskSize)
	{
		return (TaskSize < other.TaskSize);
	}
	return (TaskIndex < other.TaskIndex);
}

bool ThreadingHelper::TaskData::operator>(const TaskData& other) const
{
	if (TaskSize != other.TaskSize)
	{
		return (TaskSize > other.TaskSize);
	}
	return (TaskIndex > other.TaskIndex);
}

void ThreadingHelper::GetTaskIndices(const unsigned* taskSizes,
	unsigned countTasks,
	unsigned countThreads,
	std::vector<std::vector<unsigned>>& taskIndices)
{
	TaskIndexGettingTempType tempData;
	GetTaskIndices(taskSizes, countTasks, countThreads, taskIndices, tempData);
}

void ThreadingHelper::GetTaskIndices(const unsigned* taskSizes, unsigned countTasks, unsigned countThreads,
	std::vector<std::vector<unsigned>>& taskIndices, TaskIndexGettingTempType& tempData)
{
	// Creating and sorting task data.
	auto& taskDatas = tempData.TaskDatas;
	{
		taskDatas.clear();
		for (unsigned i = 0; i < countTasks; i++)
		{
			TaskData taskData;
			taskData.TaskIndex = i;
			taskData.TaskSize = taskSizes[i];
			taskDatas.push_back(taskData);
		}
		std::sort(taskDatas.begin(), taskDatas.end(), std::greater<TaskData>());
	}

	if (countThreads <= 32) // We use linear search and avoid every memory allocation.
	{
		auto& workSizes = tempData.ThreadWorkSizes;
		workSizes.clear();
		workSizes.resize(countThreads, 0);

		taskIndices.resize(countThreads);
		for (unsigned i = 0; i < countThreads; i++)
		{
			taskIndices[i].clear();
		}

		// Assigning tasks to threads in descending order of size.
		for (unsigned i = 0; i < countTasks; i++)
		{
			// Getting task data.
			unsigned taskIndex = taskDatas[i].TaskIndex;
			unsigned taskSize = taskDatas[i].TaskSize;

			// Finding the thread with the lowest work size.
			unsigned targetThreadIndex = 0;
			unsigned targetThreadWorkSize = workSizes[0];
			for (unsigned j = 1; j < countThreads; j++)
			{
				unsigned currentThreadWorkSize = workSizes[j];
				if (currentThreadWorkSize < targetThreadWorkSize)
				{
					targetThreadIndex = j;
					targetThreadWorkSize = currentThreadWorkSize;
				}
			}

			// Assigning task to the thread.
			taskIndices[targetThreadIndex].push_back(taskIndex);
			workSizes[targetThreadIndex] += taskSize;
		}
	}
	else // We use logarithmic search but also memory will be allocated.
	{
		// Creating thread task data.
		auto& threadTaskDatas = tempData.ThreadTaskDataQueue;
		{
			while (!threadTaskDatas.empty())
			{
				threadTaskDatas.pop();
			}
			for (unsigned i = 0; i < countThreads; i++)
			{
				ThreadTasksData threadTaskData;
				threadTaskData.ThreadIndex = i;
				threadTaskData.WorkSize = 0;
				threadTaskDatas.push(std::move(threadTaskData));
			}
		}

		// Assigning tasks to threads in descending order of size.
		for (unsigned i = 0; i < countTasks; i++)
		{
			// Getting task data.
			unsigned taskIndex = taskDatas[i].TaskIndex;
			unsigned taskSize = taskDatas[i].TaskSize;

			// Getting and removing thread data with the lowest work size.
			auto threadTaskData = std::move(threadTaskDatas.top());
			threadTaskDatas.pop();

			// Assigning task to the thread.
			threadTaskData.TaskIndices.push_back(taskIndex);
			threadTaskData.WorkSize += taskSize;

			// Putting back thread data.
			threadTaskDatas.push(std::move(threadTaskData));
		}

		// Copying task indices.
		taskIndices.resize(countThreads);
		for (; !threadTaskDatas.empty(); threadTaskDatas.pop())
		{
			auto& data = threadTaskDatas.top();
			taskIndices[data.ThreadIndex] = std::move(data.TaskIndices);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

PooledThread::PooledThread(unsigned queueSize)
	: m_IsHandlingQueueSize(queueSize != Core::c_InvalidSizeU)
	, m_WorkCount(queueSize, 0)
	, m_WorkSlotCount(queueSize, queueSize)
	, m_JoinSemaphore(true)
	, m_Id(0)
	, m_IsTerminating(false)
	, m_PSharedData(nullptr)
{
}

void PooledThread::Start(unsigned id, PooledThreadSharedData* pSharedData)
{
	this->m_Id = id;
	this->m_PSharedData = pSharedData;

	std::thread newThread(&PooledThread::ExecutionLoop, this);
	m_Thread.swap(newThread);
}

PooledThread::~PooledThread()
{
	Terminate();
}

unsigned PooledThread::GetId() const
{
	return m_Id;
}

std::thread& PooledThread::GetThreadObject()
{
	return this->m_Thread;
}

void PooledThread::PinToLogicalCore(unsigned coreId)
{
#ifdef IS_WINDOWS

	auto handle = m_Thread.native_handle();

	_GROUP_AFFINITY previousGroupAffinity;
	auto res = GetThreadGroupAffinity(handle, &previousGroupAffinity);

	if (res == 0)
	{
		throw std::runtime_error("Failed to get thread group affinity.");
	}

	_GROUP_AFFINITY group;
	ZeroMemory(&group, sizeof(_GROUP_AFFINITY));
	group.Group = static_cast<WORD>(coreId / 64);
	group.Mask = unsigned long long(1) << (coreId % 64);
	res = SetThreadGroupAffinity(handle, &group, &previousGroupAffinity);

	if (res == 0)
	{
		throw std::runtime_error("Failed to set thread group affinity.");
	}

#else
	throw std::runtime_error("Affinity setting is not implemented.");
#endif
}

void PooledThread::ExecutionLoop()
{
	try
	{
		auto& sharedData = *m_PSharedData;
		bool isTerminating = false;
		while (!isTerminating)
		{
			auto task = RemoveTask();
			task->Execute();
			
			m_Mutex.lock();
			{
				task->Release(m_ObjectCache);

				isTerminating = m_IsTerminating;
				if (m_Tasks.size() == 0)
				{
					m_JoinSemaphore.Set();
					if (sharedData.IsJoiningAny)
					{
						sharedData.ReadyId = m_Id;
						sharedData.JoinAnySemaphore.Set();
					}
				}
			}
			m_Mutex.unlock();
		}
	}
	catch (const std::exception& ex)
	{
		printf("An error has been occured during the task execution: %s", ex.what());
	}
	catch (...)
	{
		printf("An unknown error has been occured during the task execution.");
	}
}

inline void PooledThread_EmptyFunction() {}

void PooledThread::ExecuteEmptyTask()
{
	Execute(&PooledThread_EmptyFunction);
}

void PooledThread::Join()
{
	m_JoinSemaphore.Wait();
}

bool PooledThread::TryJoin()
{
	return m_JoinSemaphore.IsSet();
}

void PooledThread::Terminate()
{
	m_Mutex.lock();
	m_IsTerminating = true;
	AddTask(&PooledThread_EmptyFunction);
	m_Mutex.unlock();
	m_WorkCount.Increase();
	if (m_Thread.joinable())
	{
		m_Thread.join();
	}
}

PooledThread::TaskBase* PooledThread::RemoveTask()
{
	m_WorkCount.Decrease();
	m_Mutex.lock();
	auto task = m_Tasks.front();
	m_Tasks.pop();
	m_Mutex.unlock();
	if (m_IsHandlingQueueSize)
	{
		m_WorkSlotCount.Increase();
	}
	return task;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

ThreadPool::ThreadPool(unsigned countThreads)
	: m_Threads(countThreads == 0 ? std::thread::hardware_concurrency() : countThreads)
{
	if (countThreads == 0) countThreads = std::thread::hardware_concurrency();

	m_SharedData.ReadyId = Core::c_InvalidIndexU;	
	m_SharedData.IsJoiningAny = false;

	for (unsigned i = 0; i < countThreads; i++)
	{
		m_Threads[i].Start(i, &m_SharedData);
	}
}

PooledThread& ThreadPool::GetFreeThread()
{
	m_SharedData.JoinAnySemaphore.Reset();
	m_SharedData.ReadyId = Core::c_InvalidIndexU;
	m_SharedData.IsJoiningAny = true;
	for (size_t i = 0; i < m_Threads.size(); i++)
	{
		m_Threads[i].ExecuteEmptyTask(); // Waking the executor thread if it's waiting on a new task.
	}
	m_SharedData.JoinAnySemaphore.Wait();
	m_SharedData.IsJoiningAny = false;
	return m_Threads[m_SharedData.ReadyId];
}

PooledThread& ThreadPool::GetThread(unsigned index)
{
	return m_Threads[index];
}

std::vector<PooledThread>& ThreadPool::GetThreads()
{
	return m_Threads;
}

unsigned ThreadPool::GetCountThreads() const
{
	return static_cast<unsigned>(m_Threads.size());
}

void ThreadPool::Join()
{
	for (size_t i = 0; i < m_Threads.size(); i++)
	{
		m_Threads[i].Join();
	}
}

void ThreadPool::Terminate()
{
	for (size_t i = 0; i < m_Threads.size(); i++)
	{
		m_Threads[i].Terminate();
	}
}

void ThreadPool::PinThreadsToDifferentLogicalCores()
{
	for (size_t i = 0; i < m_Threads.size(); i++)
	{
		m_Threads[i].PinToLogicalCore(static_cast<unsigned>(i));
	}
}

std::chrono::microseconds ThreadPool::JoinAndMeasureDifference()
{
	std::chrono::high_resolution_clock::time_point startTime;

	for (size_t i = 0; i < m_Threads.size(); i++)
	{
		m_Threads[i].ExecuteEmptyTask(); // Waking the executor thread if it's waiting on a new task.
	}

	bool isFirst = true;
	Core::ByteVectorU isRunning(static_cast<unsigned>(m_Threads.size()), Core::c_True);
	unsigned countThreads = static_cast<unsigned>(m_Threads.size());
	unsigned countRunningThreads = countThreads;
	while (countRunningThreads > 0)
	{
		for (unsigned i = 0; i < countThreads; i++)
		{
			if (isRunning[i] && m_Threads[i].TryJoin())
			{
				isRunning[i] = Core::c_False;
				countRunningThreads--;

				if (isFirst)
				{
					startTime = std::chrono::high_resolution_clock::now();
					isFirst = false;
				}
			}
		}

		std::this_thread::sleep_for(std::chrono::microseconds(100));
	}

	auto endTime = std::chrono::high_resolution_clock::now();
	return std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
}