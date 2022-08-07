#include "core/stdafx.h"
#include "tier0/platform_internal.h"

double Plat_FloatTime()
{
	return v_Plat_FloatTime();
}

uint64_t Plat_MSTime()
{
	return v_Plat_MSTime();
}

const char* Plat_GetProcessUpTime()
{
	static char szBuf[4096];
	sprintf_s(szBuf, sizeof(szBuf), "[%.3f] ", Plat_FloatTime());

	return szBuf;
}