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
	*g_EbisuSDKInit     = true; // <- 1st EbisuSDK
	*g_EbisuProfileInit = true; // <- 2nd EbisuSDK
	*g_NucleusID        = true; // <- 3rd EbisuSDK
#endif // DEDICATED
}
