#include "core/stdafx.h"
#include "sys_engine.h"

///////////////////////////////////////////////////////////////////////////////
CEngine* g_pEngine = reinterpret_cast<CEngine*>(g_pEngineBuffer.GetPtr());

//-----------------------------------------------------------------------------
// Purpose: Start initializing the engine.
//-----------------------------------------------------------------------------
bool CEngine::Load(bool dedicated, const char* rootDir)
{
	static int index = 1;
	return CallVFunc<bool>(index, this, dedicated, rootDir);
}

//-----------------------------------------------------------------------------
// Purpose: Start to shutdown the engine.
//-----------------------------------------------------------------------------
void CEngine::Unload()
{
	static int index = 2;
	CallVFunc<void>(index, this);
}

//-----------------------------------------------------------------------------
// Purpose: Set the next dll engine state.
//-----------------------------------------------------------------------------
void CEngine::SetNextState(EngineState_t iNextState)
{
	// Rebuild function, vfunc index is 3 in season 3.
	m_nNextDLLState() = iNextState;
}

//-----------------------------------------------------------------------------
// Purpose: Get the dll engine state.
//-----------------------------------------------------------------------------
EngineState_t CEngine::GetState()
{
	return m_nDLLState(); // Rebuild function, vfunc index is 4 in season 3.
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEngine::Frame()
{
	static int index = 5;
	CallVFunc<void>(index, this);
}

//-----------------------------------------------------------------------------
// Purpose: Get engine frame time.
//-----------------------------------------------------------------------------
float CEngine::GetFrameTime()
{
	return m_flFrameTime(); // Rebuild function, vfunc index is 6 in season 3.
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CEngine::GetPreviousTime() // I'm not sure if this is right, should double check.
{
	static int index = 7;
	return CallVFunc<float>(index, this);
}

// Yes that is the function, I have no clue how to implement it at this moment so its gonna reside here for now. It's vfunc index 8.
//__m128 __fastcall GetCurTime(CEngine *thisPtr)
//{
//	return _mm_cvtpd_ps((__m128d)(unsigned __int64)thisPtr->m_flCurrentTime);
//}

//-----------------------------------------------------------------------------
// Purpose: Set dll state.
//-----------------------------------------------------------------------------
void CEngine::SetQuitting(EngineDllQuitting_t quitDllState)
{
	static int index = 9;
	CallVFunc<void>(index, this, quitDllState);
}