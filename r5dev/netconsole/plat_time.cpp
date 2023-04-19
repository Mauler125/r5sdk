#include "core/stdafx.h"

static LARGE_INTEGER g_PerformanceFrequency;
static double g_PerformanceCounterToS;
static double g_PerformanceCounterToMS;
static double g_PerformanceCounterToUS;
static LARGE_INTEGER g_ClockStart;
static bool s_bTimeInitted = false;

// Benchmark mode uses this heavy-handed method 
static bool g_bBenchmarkMode = false;
static double g_FakeBenchmarkTime = 0;
static double g_FakeBenchmarkTimeInc = 1.0 / 66.0;

static void InitTime()
{
	if (!s_bTimeInitted)
	{
		s_bTimeInitted = true;
		QueryPerformanceFrequency(&g_PerformanceFrequency);
		g_PerformanceCounterToS = 1.0 / g_PerformanceFrequency.QuadPart;
		g_PerformanceCounterToMS = 1e3 / g_PerformanceFrequency.QuadPart;
		g_PerformanceCounterToUS = 1e6 / g_PerformanceFrequency.QuadPart;
		QueryPerformanceCounter(&g_ClockStart);
	}
}

double Plat_FloatTime()
{
	if (!s_bTimeInitted)
		InitTime();
	if (g_bBenchmarkMode)
	{
		g_FakeBenchmarkTime += g_FakeBenchmarkTimeInc;
		return g_FakeBenchmarkTime;
	}

	LARGE_INTEGER CurrentTime;

	QueryPerformanceCounter(&CurrentTime);

	double fRawSeconds = (double)(CurrentTime.QuadPart - g_ClockStart.QuadPart) * g_PerformanceCounterToS;

	return fRawSeconds;
}

uint64_t Plat_MSTime()
{
	if (!s_bTimeInitted)
		InitTime();
	if (g_bBenchmarkMode)
	{
		g_FakeBenchmarkTime += g_FakeBenchmarkTimeInc;
		return (uint64_t)(g_FakeBenchmarkTime * 1000.0);
	}

	LARGE_INTEGER CurrentTime;

	QueryPerformanceCounter(&CurrentTime);

	return (uint64_t)((CurrentTime.QuadPart - g_ClockStart.QuadPart) * g_PerformanceCounterToMS);
}

uint64 Plat_USTime()
{
	if (!s_bTimeInitted)
		InitTime();
	if (g_bBenchmarkMode)
	{
		g_FakeBenchmarkTime += g_FakeBenchmarkTimeInc;
		return (uint64)(g_FakeBenchmarkTime * 1e6);
	}

	LARGE_INTEGER CurrentTime;

	QueryPerformanceCounter(&CurrentTime);

	return (uint64)((CurrentTime.QuadPart - g_ClockStart.QuadPart) * g_PerformanceCounterToUS);
}

#ifdef NETCONSOLE
#define TIMER_FORMAT "(%.3f) "
#else
#define TIMER_FORMAT "[%.3f] "
#endif // NETCONSOLE

const char* Plat_GetProcessUpTime()
{
	static char szBuf[4096];
	sprintf_s(szBuf, sizeof(szBuf), TIMER_FORMAT, Plat_FloatTime());

	return szBuf;
}
