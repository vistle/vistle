// EngineBuildingBlocks/SystemTime.cpp

#include <EngineBuildingBlocks/SystemTime.h>

using namespace EngineBuildingBlocks;

SystemTime::SystemTime()
{
}

void SystemTime::Initialize()
{
	m_FirstTime = m_LastTime = m_CurrentTime = Clock::now();
}

void SystemTime::Update()
{
	m_LastTime = m_CurrentTime;
	m_CurrentTime = Clock::now();
}

void SystemTime::Reset()
{
	Initialize();
}

double SystemTime::GetDifference(const TimePoint& current, const TimePoint& last) const
{
	return static_cast<double>(
		std::chrono::duration_cast<std::chrono::nanoseconds>(current - last).count()) / 1e9;
}

double SystemTime::GetElapsedTime() const
{
	return GetDifference(m_CurrentTime, m_LastTime);
}

double SystemTime::GetTotalTime() const
{
	return GetDifference(m_CurrentTime, m_FirstTime);
}