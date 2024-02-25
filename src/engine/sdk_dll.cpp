//=============================================================================//
//
// Purpose: 
//
//=============================================================================//

#include "core/stdafx.h"
#include "tier1/cvar.h"
#include "engine/sdk_dll.h"
#ifndef DEDICATED
#include "gameui/IBrowser.h"
#include "gameui/IConsole.h"
#endif // !DEDICATED

static ConVar sdk_fixedframe_tickinterval("sdk_fixedframe_tickinterval", "0.01", FCVAR_RELEASE | FCVAR_ACCESSIBLE_FROM_THREADS, "The tick interval used by the SDK fixed frame.");

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEngineSDK::FixedFrame()
{
	for (;;)
	{
#ifndef DEDICATED
		g_Browser.Think();
		g_Console.Think();
#endif // !DEDICATED
		std::this_thread::sleep_for(IntervalToDuration(sdk_fixedframe_tickinterval.GetFloat()));
	}
}

CEngineSDK g_EngineSDK;
