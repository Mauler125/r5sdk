#include "core/stdafx.h"
#include "tier1/cvar.h"
#include "sys_engine.h"
#ifdef DEDICATED
#include "game/shared/shareddefs.h"
#endif // DEDICATED

///////////////////////////////////////////////////////////////////////////////
CEngine* g_pEngine = nullptr;

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

/*
//-----------------------------------------------------------------------------
// Purpose: Start initializing the engine.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CEngine::Load(bool dedicated, const char* rootDir)
{
	const static int index = 1;
	return CallVFunc<bool>(index, this, dedicated, rootDir);
}

//-----------------------------------------------------------------------------
// Purpose: Start to shutdown the engine.
//-----------------------------------------------------------------------------
void CEngine::Unload(void)
{
	const static int index = 2;
	CallVFunc<void>(index, this);
}

//-----------------------------------------------------------------------------
// Purpose: Set the next dll engine state.
//-----------------------------------------------------------------------------
void CEngine::SetNextState(EngineState_t iNextState)
{
	m_nNextDLLState = iNextState;
}

//-----------------------------------------------------------------------------
// Purpose: Get the dll engine state.
//-----------------------------------------------------------------------------
EngineState_t CEngine::GetState(void) const
{
	return m_nDLLState;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEngine::Frame(void)
{
	const static int index = 5;
	CallVFunc<void>(index, this);
}

//-----------------------------------------------------------------------------
// Purpose: Get engine frame time.
//-----------------------------------------------------------------------------
float CEngine::GetFrameTime(void) const
{
	return m_flFrameTime;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CEngine::GetPreviousTime(void) // I'm not sure if this is right, should double check.
{
	const static int index = 7;
	return CallVFunc<float>(index, this);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
__m128 __fastcall CEngine::GetCurTime(CEngine *thisPtr) const
{
	return _mm_cvtpd_ps(_mm_cvtepi32_pd(_mm_cvtsi64_si128(thisPtr->m_flCurrentTime)));
}

//-----------------------------------------------------------------------------
// Purpose: Set dll state.
//-----------------------------------------------------------------------------
void CEngine::SetQuitting(EngineDllQuitting_t quitDllState)
{
	const static int index = 9;
	CallVFunc<void>(index, this, quitDllState);
}
*/

void VEngine::Attach() const
{
	DetourAttach((LPVOID*)&v_CEngine_Frame, &CEngine::_Frame);
}

void VEngine::Detach() const
{
	DetourDetach((LPVOID*)&v_CEngine_Frame, &CEngine::_Frame);
}