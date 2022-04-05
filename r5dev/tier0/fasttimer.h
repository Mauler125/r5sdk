//===== Copyright (c) 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef FASTTIMER_H
#define FASTTIMER_H

#include "tier0/platform.h"
#include "tier0/cpu.h"
/******************************************************************************/ 

// -------------------------------------------------------------------------- // 
// CClockSpeedInit
// -------------------------------------------------------------------------- // 
class CClockSpeed
{
public:
	CClockSpeed(void)
	{
		const CPUInformation& pi = GetCPUInformation();
		m_nClockSpeed = pi.m_Speed;
		m_dwClockSpeed = (unsigned long)m_nClockSpeed;

		m_dClockSpeedMicrosecondsMultiplier = 1000000.0 / (double)m_nClockSpeed;
		m_dClockSpeedMillisecondsMultiplier = 1000.0 / (double)m_nClockSpeed;
		m_dClockSpeedSecondsMultiplier = 1.0f / (double)m_nClockSpeed;
	}

	uint64_t m_nClockSpeed;
	uint32_t m_dwClockSpeed;

	double m_dClockSpeedMicrosecondsMultiplier;
	double m_dClockSpeedMillisecondsMultiplier;
	double m_dClockSpeedSecondsMultiplier;
};
extern CClockSpeed* g_pClockSpeed;
/******************************************************************************/ 


// -------------------------------------------------------------------------- // 
// CCycleCount
// -------------------------------------------------------------------------- // 
class CCycleCount
{
	friend class CFastTimer;

public:
	CCycleCount(void);
	CCycleCount(uint64_t cycles);

	void            Sample(void);            // Sample the clock. This takes about 34 clocks to execute (or 26,000 calls per millisecond on a P900).
	void            Init(void);              // Set to zero.
	void            Init(float initTimeMsec);
	void            Init(double initTimeMsec) { Init((float)initTimeMsec); }
	void            Init(uint64_t cycles);
	bool            IsLessThan(CCycleCount const& other) const; // Compare two counts.

	// Convert to other time representations. These functions are slow, so it's preferable to call them during display rather than inside a timing block.
	unsigned long   GetCycles(void) const;
	uint64_t        GetLongCycles(void) const;

	unsigned long   GetMicroseconds(void) const;
	uint64_t        GetUlMicroseconds(void) const;
	double          GetMicrosecondsF(void) const;
	void            SetMicroseconds(unsigned long nMicroseconds);

	unsigned long   GetMilliseconds(void) const;
	double          GetMillisecondsF(void) const;
	double          GetSeconds(void) const;

	CCycleCount& operator+=(CCycleCount const& other);

	// dest = rSrc1 + rSrc2
	static void     Add(CCycleCount const& rSrc1, CCycleCount const& rSrc2, CCycleCount& dest);	// Add two samples together.
	// dest = rSrc1 - rSrc2
	static void     Sub(CCycleCount const& rSrc1, CCycleCount const& rSrc2, CCycleCount& dest);	// Add two samples together.
	static uint64_t GetTimestamp(void);

private:
	uint64_t        m_Int64{};
};
/******************************************************************************/ 


// -------------------------------------------------------------------------- // 
// CFastTimer
// These functions are fast to call and should be called from your sampling code.
// -------------------------------------------------------------------------- // 
class CFastTimer
{
public:
	void               Start(void);
	void               End(void);

	const CCycleCount& GetDuration(void) const;	// Get the elapsed time between Start and End calls.
	CCycleCount        GetDurationInProgress(void) const; // Call without ending. Not that cheap.

	// Return number of cycles per second on this processor.
	static inline unsigned long	GetClockSpeed(void);

private:
	CCycleCount m_Duration;
#ifdef DEBUG_FASTTIMER
	bool m_bRunning; // Are we currently running?
#endif
};
/******************************************************************************/ 


// -------------------------------------------------------------------------- // 
// CTimeScope
// This is a helper class that times whatever block of code it's in.
// -------------------------------------------------------------------------- // 
class CTimeScope
{
public:
	CTimeScope(CFastTimer* pTimer);
	~CTimeScope(void);

private:
	CFastTimer* m_pTimer;
};
/******************************************************************************/ 


// -------------------------------------------------------------------------- // 
// CTimeScope
// This is a helper class that times whatever block of code it's in and adds the total (int microseconds) to a global counter.
// -------------------------------------------------------------------------- // 
class CTimeAdder
{
public:
	CTimeAdder(CCycleCount* pTotal);
	~CTimeAdder(void);

	void    End(void);

private:
	CCycleCount* m_pTotal;
	CFastTimer m_Timer;
};
/******************************************************************************/ 


// -------------------------------------------------------------------------- // 
// CLimitTimer
// Use this to time whether a desired interval of time has passed.  It's extremely fast
// to check while running.  NOTE: CMicroSecOverage() and CMicroSecLeft() are not as fast to check.
// -------------------------------------------------------------------------- // 
class CLimitTimer
{
public:
	CLimitTimer(void) { }
	CLimitTimer(uint64_t cMicroSecDuration) { SetLimit(cMicroSecDuration); }
	void SetLimit(uint64_t m_cMicroSecDuration);
	bool BLimitReached(void) const;

	int CMicroSecOverage(void) const;
	uint64_t CMicroSecLeft(void) const;

private:
	uint64_t m_lCycleLimit{};
};
/******************************************************************************/ 


// -------------------------------------------------------------------------- // 
// CAverageCycleCounter
// -------------------------------------------------------------------------- // 
class CAverageCycleCounter
{
public:
	CAverageCycleCounter(void);

	void Init(void);
	void MarkIter(const CCycleCount& duration);

	unsigned GetIters(void) const;

	double GetAverageMilliseconds(void) const;
	double GetTotalMilliseconds(void) const;
	double GetPeakMilliseconds(void) const;

private:
	unsigned    m_nIters {};
	CCycleCount m_Total  {};
	CCycleCount m_Peak   {};
	bool        m_fReport{};
	const char* m_pszName{};
};
/******************************************************************************/ 


// -------------------------------------------------------------------------- // 
// CAverageTimeMarker
// -------------------------------------------------------------------------- // 
class CAverageTimeMarker
{
public:
	CAverageTimeMarker(CAverageCycleCounter* pCounter);
	~CAverageTimeMarker(void);

private:
	CAverageCycleCounter* m_pCounter;
	CFastTimer            m_Timer;
};
/******************************************************************************/ 


// -------------------------------------------------------------------------- // 
// CCycleCount inlines.
// -------------------------------------------------------------------------- // 
inline CCycleCount::CCycleCount(void)
{
	Init((uint64_t)0);
}

inline CCycleCount::CCycleCount(uint64_t cycles)
{
	Init(cycles);
}

inline void CCycleCount::Init(void)
{
	Init((uint64_t)0);
}

inline void CCycleCount::Init(float initTimeMsec)
{
	if (g_pClockSpeed->m_dClockSpeedMillisecondsMultiplier > 0)
		Init((uint64_t)(initTimeMsec / g_pClockSpeed->m_dClockSpeedMillisecondsMultiplier));
	else
		Init((uint64_t)0);
}

inline void CCycleCount::Init(uint64_t cycles)
{
	m_Int64 = cycles;
}

inline void CCycleCount::Sample(void)
{
	m_Int64 = Plat_Rdtsc();
}

inline CCycleCount& CCycleCount::operator+=(CCycleCount const& other)
{
	m_Int64 += other.m_Int64;
	return *this;
}

inline void CCycleCount::Add(CCycleCount const& rSrc1, CCycleCount const& rSrc2, CCycleCount& dest)
{
	dest.m_Int64 = rSrc1.m_Int64 + rSrc2.m_Int64;
}

inline void CCycleCount::Sub(CCycleCount const& rSrc1, CCycleCount const& rSrc2, CCycleCount& dest)
{
	dest.m_Int64 = rSrc1.m_Int64 - rSrc2.m_Int64;
}

inline uint64_t CCycleCount::GetTimestamp(void)
{
	CCycleCount c;
	c.Sample();
	return c.GetLongCycles();
}

inline bool CCycleCount::IsLessThan(CCycleCount const& other) const
{
	return m_Int64 < other.m_Int64;
}

inline unsigned long CCycleCount::GetCycles(void) const
{
	return (unsigned long)m_Int64;
}

inline uint64_t CCycleCount::GetLongCycles(void) const
{
	return m_Int64;
}

inline unsigned long CCycleCount::GetMicroseconds(void) const
{
	return (unsigned long)((m_Int64 * 1000000) / g_pClockSpeed->m_nClockSpeed);
}

inline uint64_t CCycleCount::GetUlMicroseconds(void) const
{
	return ((m_Int64 * 1000000) / g_pClockSpeed->m_nClockSpeed);
}

inline double CCycleCount::GetMicrosecondsF(void) const
{
	return (double)(m_Int64 * g_pClockSpeed->m_dClockSpeedMicrosecondsMultiplier);
}

inline void	CCycleCount::SetMicroseconds(unsigned long nMicroseconds)
{
	m_Int64 = ((uint64_t)nMicroseconds * g_pClockSpeed->m_nClockSpeed) / 1000000;
}

inline unsigned long CCycleCount::GetMilliseconds(void) const
{
	return (unsigned long)((m_Int64 * 1000) / g_pClockSpeed->m_nClockSpeed);
}

inline double CCycleCount::GetMillisecondsF(void) const
{
	return (double)(m_Int64 * g_pClockSpeed->m_dClockSpeedMillisecondsMultiplier);
}

inline double CCycleCount::GetSeconds(void) const
{
	return (double)(m_Int64 * g_pClockSpeed->m_dClockSpeedSecondsMultiplier);
}
// -------------------------------------------------------------------------- // 


// -------------------------------------------------------------------------- // 
// CFastTimer inlines.
// -------------------------------------------------------------------------- // 
inline void CFastTimer::Start(void)
{
	m_Duration.Sample();
#ifdef DEBUG_FASTTIMER
	m_bRunning = true;
#endif
}

inline void CFastTimer::End(void)
{
	CCycleCount cnt;
	cnt.Sample();

	m_Duration.m_Int64 = cnt.m_Int64 - m_Duration.m_Int64;

#ifdef DEBUG_FASTTIMER
	m_bRunning = false;
#endif
}

inline CCycleCount CFastTimer::GetDurationInProgress(void) const
{
	CCycleCount cnt;
	cnt.Sample();

	CCycleCount result;
	result.m_Int64 = cnt.m_Int64 - m_Duration.m_Int64;

	return result;
}

inline unsigned long CFastTimer::GetClockSpeed(void)
{
	return g_pClockSpeed->m_dwClockSpeed;
}

inline CCycleCount const& CFastTimer::GetDuration(void) const
{
#ifdef DEBUG_FASTTIMER
	assert(!m_bRunning);
#endif
	return m_Duration;
}
// -------------------------------------------------------------------------- // 


// -------------------------------------------------------------------------- // 
// CTimeScope inlines.
// -------------------------------------------------------------------------- // 
inline CTimeScope::CTimeScope(CFastTimer* pTotal)
{
	m_pTimer = pTotal;
	m_pTimer->Start();
}

inline CTimeScope::~CTimeScope(void)
{
	m_pTimer->End();
}
// -------------------------------------------------------------------------- // 


// -------------------------------------------------------------------------- // 
// CTimeAdder inlines.
// -------------------------------------------------------------------------- // 
inline CTimeAdder::CTimeAdder(CCycleCount* pTotal)
{
	m_pTotal = pTotal;
	m_Timer.Start();
}

inline CTimeAdder::~CTimeAdder(void)
{
	End();
}

inline void CTimeAdder::End(void)
{
	if (m_pTotal)
	{
		m_Timer.End();
		*m_pTotal += m_Timer.GetDuration();
		m_pTotal = 0;
	}
}
// -------------------------------------------------------------------------- // 


// -------------------------------------------------------------------------- // 
// CAverageCycleCounter inlines
// -------------------------------------------------------------------------- // 
inline CAverageCycleCounter::CAverageCycleCounter(void)
	: m_nIters(0)
{
}

inline void CAverageCycleCounter::Init(void)
{
	m_Total.Init();
	m_Peak.Init();
	m_nIters = 0;
}

inline void CAverageCycleCounter::MarkIter(const CCycleCount& duration)
{
	++m_nIters;
	m_Total += duration;
	if (m_Peak.IsLessThan(duration))
		m_Peak = duration;
}

inline unsigned CAverageCycleCounter::GetIters(void) const
{
	return m_nIters;
}

inline double CAverageCycleCounter::GetAverageMilliseconds(void) const
{
	if (m_nIters)
		return (m_Total.GetMillisecondsF() / (double)m_nIters);
	else
		return 0;
}

inline double CAverageCycleCounter::GetTotalMilliseconds(void) const
{
	return m_Total.GetMillisecondsF();
}

inline double CAverageCycleCounter::GetPeakMilliseconds(void) const
{
	return m_Peak.GetMillisecondsF();
}
// -------------------------------------------------------------------------- // 


// -------------------------------------------------------------------------- // 
// CAverageTimeMarker inlines
// -------------------------------------------------------------------------- // 
inline CAverageTimeMarker::CAverageTimeMarker(CAverageCycleCounter* pCounter)
{
	m_pCounter = pCounter;
	m_Timer.Start();
}

inline CAverageTimeMarker::~CAverageTimeMarker(void)
{
	m_Timer.End();
	m_pCounter->MarkIter(m_Timer.GetDuration());
}
// -------------------------------------------------------------------------- // 


// -------------------------------------------------------------------------- // 
// CLimitTimer inlines
// -------------------------------------------------------------------------- // 
// Purpose: Initializes the limit timer with a period of time to measure.
// Input  : cMicroSecDuration -		How long a time period to measure
//-----------------------------------------------------------------------------
inline void CLimitTimer::SetLimit(uint64_t cMicroSecDuration)
{
	uint64_t dlCycles = ((uint64_t)cMicroSecDuration * (uint64_t)g_pClockSpeed->m_dwClockSpeed) / (uint64_t)1000000L;
	CCycleCount cycleCount;
	cycleCount.Sample();
	m_lCycleLimit = cycleCount.GetLongCycles() + dlCycles;
}

//-----------------------------------------------------------------------------
// Purpose: Determines whether our specified time period has passed
// Output:	true if at least the specified time period has passed
//-----------------------------------------------------------------------------
inline bool CLimitTimer::BLimitReached(void) const
{
	CCycleCount cycleCount;
	cycleCount.Sample();
	return (cycleCount.GetLongCycles() >= m_lCycleLimit);
}

//-----------------------------------------------------------------------------
// Purpose: If we're over our specified time period, return the amount of the overage.
// Output:	# of microseconds since we reached our specified time period.
//-----------------------------------------------------------------------------
inline int CLimitTimer::CMicroSecOverage(void) const
{
	CCycleCount cycleCount;
	cycleCount.Sample();
	uint64_t lcCycles = cycleCount.GetLongCycles();

	if (lcCycles < m_lCycleLimit)
		return 0;

	return((int)((lcCycles - m_lCycleLimit) * (uint64_t)1000000L / g_pClockSpeed->m_dwClockSpeed));
}

//-----------------------------------------------------------------------------
// Purpose: If we're under our specified time period, return the amount under.
// Output:	# of microseconds until we reached our specified time period, 0 if we've passed it
//-----------------------------------------------------------------------------
inline uint64_t CLimitTimer::CMicroSecLeft(void) const
{
	CCycleCount cycleCount;
	cycleCount.Sample();
	uint64_t lcCycles = cycleCount.GetLongCycles();

	if (lcCycles >= m_lCycleLimit)
		return 0;

	return((uint64_t)((m_lCycleLimit - lcCycles) * (uint64_t)1000000L / g_pClockSpeed->m_dwClockSpeed));
}
// -------------------------------------------------------------------------- // 

// -------------------------------------------------------------------------- // 
// Simple tool to support timing a block of code, and reporting the results on
// program exit or at each iteration
//
//	Macros used because dbg.h uses this header, thus Msg() is unavailable
// -------------------------------------------------------------------------- // 

#define PROFILE_SCOPE(name)                                                          \
	class C##name##ACC : public CAverageCycleCounter                                 \
	{                                                                                \
	public:                                                                          \
		~C##name##ACC()                                                              \
		{                                                                            \
			Msg("%-48s: %6.3f avg (%8.1f total, %7.3f peak, %5d iters)\n",           \
				#name,                                                               \
				GetAverageMilliseconds(),                                            \
				GetTotalMilliseconds(),                                              \
				GetPeakMilliseconds(),                                               \
				GetIters() );                                                        \
		}                                                                            \
	};                                                                               \
	static C##name##ACC name##_ACC;                                                  \
	CAverageTimeMarker name##_ATM( &name##_ACC )

#define TIME_SCOPE(name)                                                             \
	class CTimeScopeMsg_##name                                                       \
	{                                                                                \
	public:                                                                          \
		CTimeScopeMsg_##name() { m_Timer.Start(); }                                  \
		~CTimeScopeMsg_##name()                                                      \
		{                                                                            \
			m_Timer.End();                                                           \
			Msg( #name "time: %.4fms\n", m_Timer.GetDuration().GetMillisecondsF() ); \
		}                                                                            \
	private:                                                                         \
		CFastTimer m_Timer;                                                          \
	} name##_TSM;
// -------------------------------------------------------------------------- // 

#endif // FASTTIMER_H
