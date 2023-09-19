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

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEngineSDK::FixedFrame()
{
	for (;;)
	{
#ifndef DEDICATED
		g_pBrowser->Think();
		g_pConsole->Think();
#endif // !DEDICATED
		std::this_thread::sleep_for(IntervalToDuration(sdk_fixedframe_tickinterval->GetFloat()));
	}
}

CEngineSDK* g_EngineSDK = new CEngineSDK();
