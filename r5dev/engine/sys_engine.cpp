#include "core/stdafx.h"
#include "sys_engine.h"

///////////////////////////////////////////////////////////////////////////////
CEngine* g_pEngine = nullptr;

//-----------------------------------------------------------------------------
// Purpose: Start initializing the engine.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CEngine::Load(bool dedicated, const char* rootDir)
{
	static int index = 1;
	return CallVFunc<bool>(index, this, dedicated, rootDir);
}

//-----------------------------------------------------------------------------
// Purpose: Start to shutdown the engine.
//-----------------------------------------------------------------------------
void CEngine::Unload(void)
{
	static int index = 2;
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
	static int index = 5;
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
	static int index = 7;
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
	static int index = 9;
	CallVFunc<void>(index, this, quitDllState);
}
