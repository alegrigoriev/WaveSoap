#pragma once

namespace ElapsedTime
{

template<typename TimeTraits>
class CElapsedTimeT : TimeTraits
{
public:

	CElapsedTimeT()
		: m_StartTime(TimeTraits::GetTime())
	{
	}

	~CElapsedTimeT()
	{
	}
	typename TimeTraits::TimeDifferenceType ElapsedTime()
	{
		return TimeTraits::GetTime() - m_StartTime;
	}

	DWORD ElapsedTimeMs()
	{
		return TimeTraits::ToMs(TimeTraits::GetTime() - m_StartTime);
	}
	DWORD ElapsedTimeTenthMs()
	{
		return TimeTraits::ToTenthMs(TimeTraits::GetTime() - m_StartTime);
	}
private:
	typename TimeTraits::TimeType m_StartTime;
};

struct CElapsedTimeDummy
{
	DWORD ElapsedTime()
	{
		return 0;
	}

	DWORD ElapsedTimeMs()
	{
		return 0;
	}
	DWORD ElapsedTimeTenthMs()
	{
		return 0;
	}
};

class MmTimeTraits
{
public:
	typedef DWORD TimeType;
	typedef DWORD TimeDifferenceType;
	static DWORD ToMs(TimeDifferenceType diff)
	{
		return diff;
	}
	static DWORD ToTenthMs(TimeDifferenceType diff)
	{
		return diff * 10;
	}
	static TimeType GetTime()
	{
		return timeGetTime();
	}

};

class PerfCounterTimeTraits
{
public:
	typedef ULONGLONG TimeType;
	typedef LONGLONG TimeDifferenceType;
	static DWORD ToMs(TimeDifferenceType diff)
	{
		LARGE_INTEGER freq;
		if (QueryPerformanceFrequency( & freq))
		{
			ASSERT(freq >= 1000U);
			return diff / ULONG(freq / 1000U);
		}
		return 0;
	}
	static DWORD ToTenthMs(TimeDifferenceType diff)
	{
		LARGE_INTEGER freq;
		if (QueryPerformanceFrequency( & freq))
		{
			ASSERT(freq >= 10000U);
			return diff / ULONG(freq / 10000U);
		}
		return 0;
	}
	static TimeType GetTime()
	{
		LARGE_INTEGER counter;
		if (QueryPerformanceCounter( & counter))
		{
			return counter.QuadPart;
		}
		return 0;
	}

};
}

typedef ElapsedTime::CElapsedTimeT<ElapsedTime::MmTimeTraits> ElapsedTimeStampMm;
typedef ElapsedTime::CElapsedTimeT<ElapsedTime::PerfCounterTimeTraits> ElapsedTimeStampPerf;

#ifdef _DEBUG
typedef ElapsedTimeStampPerf DebugTimeStamp;
#else
typedef ElapsedTime::CElapsedTimeDummy DebugTimeStamp;
#endif
