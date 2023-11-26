#include "core/stdafx.h"
#include "tier1/cvar.h"
#include "sys_engine.h"
#ifdef DEDICATED
#include "game/shared/shareddefs.h"
#endif // DEDICATED

///////////////////////////////////////////////////////////////////////////////
CEngine* g_pEngine = nullptr;
IEngine::QuitState_t* gsm_Quitting = nullptr;

bool CEngine::_Frame(CEngine* thisp)
{
#ifdef DEDICATED

	// The first engine frame is ran before the global variables are initialized.
	// By default, the tick interval is set to '0.0f'; we can't divide by zero.
	if (TICK_INTERVAL > 0.0f)
	{
		int nTickRate = TIME_TO_TICKS(1.0f);
		if (fps_max->GetInt() != nTickRate)
		{
			// Clamp the framerate of the server to its simulation tick rate.
			// This saves a significant amount of CPU time in CEngine::Frame,
			// as the engine uses this to decided when to run a new frame.
			fps_max->SetValue(nTickRate);
		}
	}

#endif // DEDICATED
	return v_CEngine_Frame(thisp);
}

void VEngine::Detour(const bool bAttach) const
{
	DetourSetup(&v_CEngine_Frame, &CEngine::_Frame, bAttach);
}
