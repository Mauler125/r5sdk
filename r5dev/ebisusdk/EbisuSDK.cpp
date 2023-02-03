#include "core/stdafx.h"
#include "ebisusdk/EbisuSDK.h"

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool IsOriginInitialized()
{
#ifndef DEDICATED
	if ((!(*g_OriginErrorLevel)
		&& (*g_EbisuSDKInit)
		&& (*g_NucleusID)
		&& (*g_EbisuProfileInit)))
	//	&& (*g_OriginAuthCode)
	//		&& (g_NucleusToken[0])))
#endif // DEDICATED
	{
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: sets the EbisuSDK globals for dedicated to satisfy command callbacks
//-----------------------------------------------------------------------------
void HEbisuSDK_Init()
{
#ifdef DEDICATED
	*(char*)g_bEbisuSDKInitialized     = (char)0x1; // <- 1st EbisuSDK
	*(char*)g_bEbisuSDKCvarInitialized = (char)0x1; // <- 2nd EbisuSDK
	*(char*)g_NucleusID                = (char)0x1; // <- 3rd EbisuSDK
#endif // DEDICATED
}

void EbisuSDK_Attach()
{
	//
}

void EbisuSDK_Detach()
{
	//
}
