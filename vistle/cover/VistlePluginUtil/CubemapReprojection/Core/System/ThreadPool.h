// Core/System/ThreadPool.h

#ifndef _CORE_THREADPOOL_H_INCLUDED_
#define _CORE_THREADPOOL_H_INCLUDED_

#include <Core/Constants.h>
#include <Core/System/Semaphore.hpp>
#include <Core/ObjectCache.hpp>
#include <Core/Functional.hpp>

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include <tuple>
#include <map>
#include <chrono>

namespace Core
{
	class ThreadingHelper
	{
	public:

		static unsigned GetCountRequiredThreads(unsigned countTasks,
			unsigned countMaximumThreads);

		static void GetTaskIndices(unsigned countTasks, unsigned countThreads,
			unsigned threadIndex, unsigned& startIndex, unsigned& endIndex);

		struct ThreadTasksData
		{
			std::vector<unsigned> TaskIndices;
			unsigned WorkSize;
			unsigned ThreadIndex;

			bool operator<(const ThreadTasksData& other) const;
			bool operator>(const ThreadTasksData& other) const;
		};

		struct TaskData
		{
			unsigned TaskIndex;
			unsigned TaskSize;

			bool operator<(const TaskData& other) const;
			bool operator>(const TaskData& other) const;
		};

		struct TaskIndexGettingTempType
		{
			std::vector<TaskData> TaskDatas;

			// For the vector implementation.
			std::vector<unsigned> ThreadWorkSizes;

			// For the queue implementation.
			std::priority_queue < ThreadTasksData, std::vector<ThreadTasksData>,
				std::greater < ThreadTasksData >> ThreadTaskDataQueue;

		};

		static void GetTaskIndices(const unsigned* taskSizes,
			unsigned countTasks,
			unsigned countThreads,
			std::vector<std::vector<unsigned>>& taskIndices);

		static void GetTaskIndices(const unsigned* taskSizes,
			unsigned countTasks,
			unsigned countThreads,
			std::vector<std::vector<unsigned>>& taskIndices,
			TaskIndexGettingTempType& tempData);
	};

	struct PooledThreadSharedData
	{
		SetResetSemaphore JoinAnySemaphore;
		std::atomic<unsigned> ReadyId;
		std::atomic<bool> IsJoiningAny;
	};

	class PooledThread
	{
		struct TaskBase
		{
			virtual ~TaskBase(){}
			virtual void Execute() = 0;	

			// The code seems to be the same for all implementation, but note that the template
			// function call's deduced type is different!
			virtual void Release(ObjectCache& objectCache) = 0;
		};

		template <typename FunctionType, typename... Args>
		struct FunctionTask : public TaskBase
		{
			FunctionType Function;
			std::tuple<Args...> Arguments;

			template <typename _Function, typename _Tuple>
			FunctionTask(_Function&& function, _Tuple&& arguments)
				: Function(std::forward<_Function>(function))
				, Arguments(std::forward<_Tuple>(arguments))
			{}

			inline void Execute()
			{
				Core::ApplyFunction(Function, Arguments);
			}

			inline void Release(ObjectCache& objectCache)
			{
				objectCache.Release(this);
			}
		};

		std::thread m_Thread;

		bool m_IsHandlingQueueSize;

		std::mutex m_Mutex;
		Semaphore m_WorkCount;
		Semaphore m_WorkSlotCount;

		SetResetSemaphore m_JoinSemaphore;

		ObjectCache m_ObjectCache;

		std::queue<TaskBase*> m_Tasks;

		unsigned m_Id;

		bool m_IsTerminating;

		PooledThreadSharedData* m_PSharedData;

		void ExecutionLoop();

		TaskBase* RemoveTask();

		template <typename FunctionType, typename... Args>
		void AddTask(FunctionType&& function, Args&&... args)
		{
                        using FuncDecType = typename std::decay<FunctionType>::type;
                        auto task = m_ObjectCache.Request<FunctionTask<FuncDecType, typename std::decay<Args>::type...>>(
				std::forward<FunctionType>(function),
                                std::tuple<typename std::decay<Args>::type...>(std::forward<Args>(args)...));
			m_Tasks.push(task);
		}

	public: // Only for ThreadPool.
		
		void Start(unsigned id, PooledThreadSharedData* pSharedData);
		void ExecuteEmptyTask();

	public:

		PooledThread(unsigned queueSize = Core::c_InvalidSizeU);
		~PooledThread();

		unsigned GetId() const;

		std::thread& GetThreadObject();

		void PinToLogicalCore(unsigned affinity);

		// Note that we COPY all arguments, in order to avoid taking references to
		// local variables, loop counters and so on.

		template <typename FunctionType, typename... Args>
		void Execute(FunctionType&& function, Args&&... args)
		{
			m_JoinSemaphore.Reset();
			if (m_IsHandlingQueueSize)
			{
				m_WorkSlotCount.Decrease();
			}
			m_Mutex.lock();
			AddTask(std::forward<FunctionType>(function), std::forward<Args>(args)...);
			m_Mutex.unlock();
			m_WorkCount.Increase();
		}

		void Join();
		bool TryJoin();
		void Terminate();
	};

	class ThreadPool
	{
		std::vector<PooledThread> m_Threads;
		SetResetSemaphore m_Semaphore;
		std::mutex m_Mutex;

		unsigned m_DynamicTaskCount;
		std::atomic<unsigned> m_DynamicTaskIndex;

		PooledThreadSharedData m_SharedData;

	public:

		ThreadPool(unsigned countThreads = 0);

		PooledThread& GetFreeThread();
		PooledThread& GetThread(unsigned index);
		std::vector<PooledThread>& GetThreads();

		unsigned GetCountThreads() const;

		void Join();
		void Terminate();

		void PinThreadsToDifferentLogicalCores();

		template <typename FunctionType, typename ObjectType>
		void DynamicTaskFeederFunction(unsigned threadId,
			FunctionType&& function, ObjectType* pObject, unsigned taskPackageSize)
		{
			while (true)
			{
				unsigned startIndex = m_DynamicTaskIndex.fetch_add(taskPackageSize); // Atomic increment.
				unsigned endIndex = startIndex + taskPackageSize;

				if (startIndex >= m_DynamicTaskCount)
				{
					break;
				}

				if (endIndex > m_DynamicTaskCount)
				{
					endIndex = m_DynamicTaskCount;
				}

				(pObject->*function)(threadId, startIndex, endIndex);
			}
		}

	public:

		// Executes the given function using uniform interval splitting as
		// static scheduling. Returning the number of threads that executed the tasks.
		template <typename FunctionType, typename ObjectType,
                        typename = typename std::enable_if<std::is_member_function_pointer<FunctionType>::value>::type,
			typename... Args>
			inline unsigned ExecuteWithStaticScheduling(unsigned countTasks, FunctionType&& function, ObjectType* pObject, Args&&... args)
		{
			unsigned countThreads = ThreadingHelper::GetCountRequiredThreads(
				countTasks, GetCountThreads());
			for (unsigned i = 0; i < countThreads; i++)
			{
				unsigned startIndex, endIndex;
				ThreadingHelper::GetTaskIndices(countTasks, countThreads, i, startIndex, endIndex);
				m_Threads[i].Execute(std::forward<FunctionType>(function),
					pObject, i, startIndex, endIndex, std::forward<Args>(args)...);
			}
			if (countTasks != 0)
			{
				Join();
			}
			return countThreads;
		}

		// Executes the given global function using uniform interval splitting as
		// static scheduling. Returning the number of threads that executed the tasks.
		template <typename FunctionType,
                        typename = typename std::enable_if<!std::is_member_function_pointer<FunctionType>::value>::type,
			typename... Args>
			inline unsigned ExecuteWithStaticScheduling(unsigned countTasks, FunctionType&& function, Args&&... args)
		{
			unsigned countThreads = ThreadingHelper::GetCountRequiredThreads(
				countTasks, GetCountThreads());
			for (unsigned i = 0; i < countThreads; i++)
			{
				unsigned startIndex, endIndex;
				ThreadingHelper::GetTaskIndices(countTasks, countThreads, i, startIndex, endIndex);
				m_Threads[i].Execute(std::forward<FunctionType>(function),
					i, startIndex, endIndex, std::forward<Args>(args)...);
			}
			if (countTasks != 0)
			{
				Join();
			}
			return countThreads;
		}

		template <typename FunctionType, typename ObjectType,
                        typename = typename std::enable_if<std::is_member_function_pointer<FunctionType>::value>::type>
			inline unsigned ExecuteWithDynamicScheduling(unsigned countTasks, FunctionType&& function, ObjectType* pObject, unsigned taskPackageSize = 1)
		{
			m_DynamicTaskCount = countTasks;
			m_DynamicTaskIndex = 0;

			unsigned countThreads = static_cast<unsigned>(m_Threads.size());
			for (unsigned i = 0; i < countThreads; i++)
			{
				m_Threads[i].Execute(&Core::ThreadPool::DynamicTaskFeederFunction<const FunctionType&, ObjectType>,
					this, i, std::forward<FunctionType>(function), pObject, taskPackageSize);
			}
			Join();

			return countThreads;
		}

	public: // Mainly for debugging.

		std::chrono::microseconds JoinAndMeasureDifference();
	};
}

#endif
