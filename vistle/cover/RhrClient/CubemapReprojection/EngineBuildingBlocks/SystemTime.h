// EngineBuildingBlocks/SystemTime.h

#ifndef _ENGINEBUILDINGBLOCKS_SYSTEMTIME_H_INCLUDED_
#define _ENGINEBUILDINGBLOCKS_SYSTEMTIME_H_INCLUDED_

#include <chrono>

namespace EngineBuildingBlocks
{
	class SystemTime
	{
	private:

		typedef std::chrono::high_resolution_clock Clock;
		typedef std::chrono::high_resolution_clock::time_point TimePoint;

		TimePoint m_FirstTime;
		TimePoint m_LastTime;
		TimePoint m_CurrentTime;

		double GetDifference(const TimePoint& current, const TimePoint& last) const;

	public:

		SystemTime();

		void Initialize();
		void Update();

		void Reset();

		// Returns elapsed time in seconds.
		double GetElapsedTime() const;

		// Returns total time in seconds.
		double GetTotalTime() const;
	};
}

#endif