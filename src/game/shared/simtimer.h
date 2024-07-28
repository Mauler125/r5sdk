//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include <game/server/gameinterface.h>
#ifndef SIMTIMER_H
#define SIMTIMER_H

#if defined( _WIN32 )
#pragma once
#endif

#define ST_EPS 0.001

#define DEFINE_SIMTIMER( type, name ) 	DEFINE_EMBEDDED( type, name )

//-----------------------------------------------------------------------------

class CSimpleSimTimer
{
public:
	CSimpleSimTimer()
	 : m_next( -1 )
	{ 
	}

	void Force()
	{
		m_next = -1;
	}

	bool Expired() const
	{
		return ( (*g_pGlobals)->m_flCurTime - m_next > -ST_EPS );
	}

	float Delay( float delayTime )
	{
		return (m_next += delayTime);
	}
	
	float GetNext() const
	{
		return m_next;
	}

	void Set( float interval )
	{
		m_next = (*g_pGlobals)->m_flCurTime + interval;
	}

	// TODO: get CUniformRandomStream global, see 141809528 for global!!!
	//void Set( float minInterval, float maxInterval )
	//{ 
	//	if ( maxInterval > 0.0 )
	//		m_next = (*g_pGlobals)->m_flCurTime + random->RandomFloat( minInterval, maxInterval );
	//	else
	//		m_next = (*g_pGlobals)->m_flCurTime + minInterval;
	//}

	float GetRemaining() const
	{
		float result = m_next - (*g_pGlobals)->m_flCurTime;
		if (result < 0 )
			return 0;
		return result;
	}

	//DECLARE_SIMPLE_DATADESC();
	
protected:
	float m_next;
};

//-----------------------------------------------------------------------------

class CSimTimer : public CSimpleSimTimer
{
public:
	CSimTimer( float interval = 0.0f, bool startExpired = true )	
	{ 
		Set( interval, startExpired );
	}
	
	void Set( float interval, bool startExpired = true )
	{ 
		m_interval = interval;
		m_next = (startExpired) ? -1.0f : (*g_pGlobals)->m_flCurTime + m_interval;
	}

	void Reset( float interval = -1.0f )
	{
		if ( interval == -1.0f )
		{
			m_next = (*g_pGlobals)->m_flCurTime + m_interval;
		}
		else
		{
			m_next = (*g_pGlobals)->m_flCurTime + interval;
		}
	}

	float GetInterval() const
	{
		return m_interval;
	}

	//DECLARE_SIMPLE_DATADESC();
	
private:
	float m_interval;
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

class CRandSimTimer : public CSimpleSimTimer
{
public:
	// TODO: get CUniformRandomStream global, see 141809528 for global!!!
	//CRandSimTimer( float minInterval = 0.0, float maxInterval = 0.0, bool startExpired = true )	
	//{ 
	//	Set( minInterval, maxInterval, startExpired );
	//}
	
	//void Set( float minInterval, float maxInterval = 0.0, bool startExpired = true )
	//{ 
	//	m_minInterval = minInterval;
	//	m_maxInterval = maxInterval;
	//	
	//	if (startExpired)
	//	{
	//		m_next = -1;
	//	}
	//	else
	//	{
	//		if ( m_maxInterval == 0 )
	//			m_next = (*g_pGlobals)->m_flCurTime + m_minInterval;
	//		else
	//			m_next = (*g_pGlobals)->m_flCurTime + random->RandomFloat( m_minInterval, m_maxInterval );
	//	}
	//}

	//void Reset()
	//{
	//	if ( m_maxInterval == 0 )
	//		m_next = (*g_pGlobals)->m_flCurTime + m_minInterval;
	//	else
	//		m_next = (*g_pGlobals)->m_flCurTime + random->RandomFloat( m_minInterval, m_maxInterval );
	//}

	float GetMinInterval() const
	{
		return m_minInterval;
	}

	float GetMaxInterval() const
	{
		return m_maxInterval;
	}

	//DECLARE_SIMPLE_DATADESC();
	
private:
	float m_minInterval;
	float m_maxInterval;
};

//-----------------------------------------------------------------------------

class CStopwatchBase  : public CSimpleSimTimer
{
public:
	CStopwatchBase()	
	{ 
		m_fIsRunning = false;
	}

	bool IsRunning() const
	{
		return m_fIsRunning;
	}
	
	void Stop()
	{
		m_fIsRunning = false;
	}

	bool Expired() const
	{
		return ( m_fIsRunning && CSimpleSimTimer::Expired() );
	}
	
	//DECLARE_SIMPLE_DATADESC();
	
protected:
	bool m_fIsRunning;
	
};

//-------------------------------------
class CSimpleStopwatch  : public CStopwatchBase
{
public:
	// TODO: get CUniformRandomStream global, see 141809528 for global!!!
	//void Start( float minCountdown, float maxCountdown = 0.0 )
	//{ 
	//	m_fIsRunning = true;
	//	CSimpleSimTimer::Set( minCountdown, maxCountdown );
	//}

	void Stop()
	{
		m_fIsRunning = false;
	}

	bool Expired() const
	{
		return ( m_fIsRunning && CSimpleSimTimer::Expired() );
	}
};
//-------------------------------------

class CStopwatch : public CStopwatchBase
{
public:
	CStopwatch ( float interval = 0.0 )
	{ 
		Set( interval );
	}
	
	void Set( float interval )
	{ 
		m_interval = interval;
	}

	void Start( float intervalOverride )
	{ 
		m_fIsRunning = true;
		m_next = (*g_pGlobals)->m_flCurTime + intervalOverride;
	}

	void Start()
	{
		Start( m_interval );
	}
	
	float GetInterval() const
	{
		return m_interval;
	}

	//DECLARE_SIMPLE_DATADESC();
	
private:
	float m_interval;
};

//-------------------------------------

class CRandStopwatch : public CStopwatchBase
{
public:
	CRandStopwatch( float minInterval = 0.0, float maxInterval = 0.0 )	
	{ 
		Set( minInterval, maxInterval );
	}
	
	void Set( float minInterval, float maxInterval = 0.0 )
	{ 
		m_minInterval = minInterval;
		m_maxInterval = maxInterval;
	}

	// TODO: get CUniformRandomStream global, see 141809528 for global!!!
	//void Start( float minOverride, float maxOverride = 0.0 )
	//{ 
	//	m_fIsRunning = true;
	//	if ( maxOverride == 0 )
	//		m_next = (*g_pGlobals)->m_flCurTime + minOverride;
	//	else
	//		m_next = (*g_pGlobals)->m_flCurTime + random->RandomFloat( minOverride, maxOverride );
	//}

	//void Start()
	//{
	//	Start( m_minInterval, m_maxInterval );
	//}
	
	float GetInterval() const
	{
		return m_minInterval;
	}

	float GetMinInterval() const
	{
		return m_minInterval;
	}

	float GetMaxInterval() const
	{
		return m_maxInterval;
	}

	//DECLARE_SIMPLE_DATADESC();
	
private:
	float m_minInterval;
	float m_maxInterval;
};

//-----------------------------------------------------------------------------

class CThinkOnceSemaphore
{
public:
	CThinkOnceSemaphore()
	 :	m_lastTime( -1 )
	{
	}

	bool EnterThink()
	{
		if ( m_lastTime == (*g_pGlobals)->m_flCurTime)
			return false;
		m_lastTime = (*g_pGlobals)->m_flCurTime;
		return true;
	}

	bool DidThink() const
	{
		return ( (*g_pGlobals)->m_flCurTime == m_lastTime );

	}

	void SetDidThink()
	{
		m_lastTime = (*g_pGlobals)->m_flCurTime;
	}

private:
	float m_lastTime;
};

//-----------------------------------------------------------------------------

#endif // SIMTIMER_H
