#include "core/stdafx.h"
#include "ebisusdk/EbisuSDK.h"

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
