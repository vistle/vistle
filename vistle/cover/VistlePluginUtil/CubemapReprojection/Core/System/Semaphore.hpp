// Core/System/Semaphore.hpp

#ifndef _CORE_SEMAPHORE_HPP_
#define _CORE_SEMAPHORE_HPP_

#include <mutex>
#include <condition_variable>
#include <cassert>

namespace Core
{
	class Semaphore
	{
		unsigned m_MaxCount;
		unsigned m_Count;

		std::mutex m_Mutex;
		std::condition_variable m_ConditionalVariable;

	public:

		Semaphore(unsigned maxCount = 1, int count = -1)
			: m_MaxCount(maxCount)
			, m_Count(count == -1 ? maxCount : static_cast<unsigned>(count))
		{
			assert(m_Count <= m_MaxCount);
		}

		inline void Increase()
		{
			std::lock_guard<std::mutex> guard(m_Mutex);
			if (m_Count < m_MaxCount)
			{
				m_Count++;
				m_ConditionalVariable.notify_one();
			}
		}

		inline void Decrease()
		{
			std::unique_lock<std::mutex> lock(m_Mutex);
			m_ConditionalVariable.wait(lock, [=] { return m_Count > 0; });
			m_Count--;
		}

		inline bool TryDecrease()
		{
			std::lock_guard<std::mutex> guard(m_Mutex);
			if (m_Count > 0)
			{
				m_Count--;
				return true;
			}
			return false;
		}

		inline unsigned GetCount()
		{
			std::lock_guard<std::mutex> guard(m_Mutex);
			return m_Count;
		}

		inline unsigned GetMaxCount()
		{
			return m_MaxCount;
		}
	};

	class SetResetSemaphore
	{
		bool m_Value;

		std::mutex m_Mutex;
		std::condition_variable m_ConditionalVariable;

	public:

		SetResetSemaphore(bool value = false)
			: m_Value(value)
		{
		}

		inline void Set()
		{
			std::lock_guard<std::mutex> guard(m_Mutex);
			m_Value = true;
			m_ConditionalVariable.notify_all();
		}

		inline void Reset()
		{
			std::lock_guard<std::mutex> guard(m_Mutex);
			m_Value = false;
		}

		inline void Wait()
		{
			std::unique_lock<std::mutex> lock(m_Mutex);
			m_ConditionalVariable.wait(lock, [=] { return m_Value; });
		}

		inline bool IsSet()
		{
			std::lock_guard<std::mutex> guard(m_Mutex);
			return m_Value;
		}
	};
}

#endif