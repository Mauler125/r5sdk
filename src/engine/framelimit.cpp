//===========================================================================//
//
// Purpose: High-precision frame rate limiter
//
//===========================================================================//
#include <dwmapi.h>
#include "tier0/platform_internal.h"
#include "windows/id3dx.h"
#include "sys_mainwind.h"
#include "framelimit.h"

//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
CFrameLimit::CFrameLimit(void)
{
	m_MilliSeconds = 0.0;
	m_FramesPerSecond = 0.0;

	m_Start.QuadPart = 0;
	m_Next.QuadPart = 0;
	m_Time.QuadPart = 0;

	m_Frames = 0;
	m_bRestart = false;
}

//-----------------------------------------------------------------------------
// Purpose: initializer
// Input  : targetFps - 
//-----------------------------------------------------------------------------
void CFrameLimit::Reset(double targetFps)
{
	m_MilliSeconds = 1000.0 / targetFps;
	m_FramesPerSecond = targetFps;

	QueryPerformanceCounter(&m_Start);
	m_Next.QuadPart = 0ULL;
	m_Time.QuadPart = 0ULL;

	//m_Last.QuadPart = m_Start.QuadPart - (LONGLONG)((m_MilliSeconds / 1000.0) * g_pPerformanceFrequency->QuadPart);
	m_Next.QuadPart = m_Start.QuadPart + (LONGLONG)((m_MilliSeconds / 1000.0) * g_pPerformanceFrequency->QuadPart);

	m_Frames = 0;
}

//-----------------------------------------------------------------------------
// Purpose: runs the frame limiter logic
//-----------------------------------------------------------------------------
void CFrameLimit::Run(const double targetFps, const double sleepThreshold, const double maxTolerance)
{
	if (m_FramesPerSecond != targetFps)
		Reset(targetFps);

	m_Frames++;
	QueryPerformanceCounter(&m_Time);

	// Actual frametime before we forced a delay
	//m_EffectiveMilliSeconds = 1000.0 * ((double)(m_Time.QuadPart - m_Last.QuadPart) / (double)g_pPerformanceFrequency->QuadPart);

	if ((double)(m_Time.QuadPart - m_Next.QuadPart) / (double)g_pPerformanceFrequency->QuadPart / (m_MilliSeconds / 1000.0) > (maxTolerance * m_FramesPerSecond))
	{
		DevMsg(eDLL_T::ENGINE, "%s: Frame time too long (expected: %3.01fx); restarting...\n",
			__FUNCTION__, (double)(m_Time.QuadPart - m_Next.QuadPart) / (double)g_pPerformanceFrequency->QuadPart / (m_MilliSeconds / 1000.0) / m_FramesPerSecond );
		m_bRestart = true;
	}

	if (m_bRestart)
	{
		m_Frames = 0;
		m_Start.QuadPart = m_Time.QuadPart + (LONGLONG)((m_MilliSeconds / 1000.0) * (double)g_pPerformanceFrequency->QuadPart);
		m_bRestart = false;
		//Reset (targetFps);
		//return;
	}

	m_Next.QuadPart = (LONGLONG)((m_Start.QuadPart + (double)m_Frames * (m_MilliSeconds / 1000.0) * (double)g_pPerformanceFrequency->QuadPart));

	if (m_Next.QuadPart > 0ULL)
	{
		while (m_Time.QuadPart < m_Next.QuadPart)
		{
			if ((double)(m_Next.QuadPart - m_Time.QuadPart) > (sleepThreshold * (double)g_pPerformanceFrequency->QuadPart))
			{
				Sleep(10);
			}

			QueryPerformanceCounter(&m_Time);
		}
	}

	//m_Last.QuadPart = m_Time.QuadPart;
}
