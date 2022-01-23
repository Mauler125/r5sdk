#pragma once

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CEngine;

///////////////////////////////////////////////////////////////////////////////
extern CEngine* g_pEngine;

enum class EngineState_t : int
{
	DLL_INACTIVE = 0x0,
	DLL_ACTIVE   = 0x1,
	DLL_CLOSE    = 0x2,
	DLL_RESTART  = 0x3,
	DLL_PAUSED   = 0x4,
};

enum class EngineDllQuitting_t : int
{
	QUIT_NOTQUITTING = 0x0,
	QUIT_TODESKTOP   = 0x1,
	QUIT_RESTART     = 0x2,
};


// TODO: Check if all indexes match up between seasons. If not patternscan them.
class CEngine
{
public:
	//-----------------------------------------------------------------------------
	// Purpose: Start initializing the engine.
	//-----------------------------------------------------------------------------
	bool Load(bool dedicated, const char* rootDir)
	{
		static int index = 1;
		return CallVFunc<bool>(index, this, dedicated, rootDir);
	}
	
	//-----------------------------------------------------------------------------
	// Purpose: Start to shutdown the engine.
	//-----------------------------------------------------------------------------
	void Unload()
	{
		static int index = 2;
		CallVFunc<void>(index, this);
	}
	//-----------------------------------------------------------------------------
	// Purpose: Set the next dll engine state.
    //-----------------------------------------------------------------------------
	void SetNextState(EngineState_t iNextState)
	{
		// Rebuild function, vfunc index is 3 in season 3.
		m_nNextDLLState() = iNextState;
	}

	//-----------------------------------------------------------------------------
	// Purpose: Get the dll engine state.
	//-----------------------------------------------------------------------------
	EngineState_t GetState()
	{
		// Rebuild function, vfunc index is 4 in season 3.
		return m_nDLLState();
	}

	//-----------------------------------------------------------------------------
	// Purpose:
	//-----------------------------------------------------------------------------
	void Frame()
	{
		static int index = 5;
		CallVFunc<void>(index, this);
	}

	//-----------------------------------------------------------------------------
	// Purpose: Get engine frame time.
	//-----------------------------------------------------------------------------
	float GetFrameTime()
	{
		// Rebuild function, vfunc index is 6 in season 3.
		return m_flFrameTime();
	}

	//-----------------------------------------------------------------------------
	// Purpose:
	//-----------------------------------------------------------------------------
	float GetPreviousTime() // I'm not sure if this is right, should double check.
	{
		static int index = 7;
		return CallVFunc<float>(index, this);
	}

	// Yes that is the function, I have no clue how to implement it at this moment so its gonna reside here for now. It's vfunc index 8.
	 
	//__m128 __fastcall CEngine::GetCurTime(CEngine *thisPtr)
	//{
	//	return _mm_cvtpd_ps((__m128d)(unsigned __int64)thisPtr->m_flCurrentTime);
	//}

	//-----------------------------------------------------------------------------
	// Purpose: Set dll state.
	//-----------------------------------------------------------------------------
	void SetQuitting(EngineDllQuitting_t quitDllState)
	{
		static int index = 9;
		CallVFunc<void>(index, this, quitDllState);
	}

	// Last functions in class table.
	// sub_1401FE2A0
	// sub_1401FE2B0
	// sub_1401FE3B0

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	MEMBER_AT_OFFSET(EngineState_t, m_nDLLState, 0x8);
	MEMBER_AT_OFFSET(EngineState_t, m_nNextDLLState, 0xC);
	MEMBER_AT_OFFSET(std::int64_t, m_flCurrentTime, 0x10);
	MEMBER_AT_OFFSET(std::int64_t, m_flPreviousTime, 0x18);
	MEMBER_AT_OFFSET(int, m_flFrameTime, 0x20);
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3) // TODO: Verify offsets for other seasons. Should probably be the same as Season 2.
	MEMBER_AT_OFFSET(EngineState_t, m_nDLLState, 0x8);
	MEMBER_AT_OFFSET(EngineState_t, m_nNextDLLState, 0xC);
	MEMBER_AT_OFFSET(std::int64_t, m_flCurrentTime, 0x10); // They are 8 bytes for some reason but floats? Kinda confusing.
	MEMBER_AT_OFFSET(std::int64_t, m_flPreviousTime, 0x18);
	MEMBER_AT_OFFSET(float, m_flFrameTime, 0x20);
#endif
};

namespace
{
	/* ==== CENGINE ======================================================================================================================================================= */
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1) // Verify for s0 and s1
	static ADDRESS g_pEngineBuffer = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x8D\x0D\x00\x00\x00\x00\x74\x2A\x4C\x8B\x05\x00\x00\x00\x00", "xxx????xxxxx????").ResolveRelativeAddressSelf(0x3, 0x7);
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3) // Verify for s2
	static ADDRESS g_pEngineBuffer = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x8D\x0D\x00\x00\x00\x00\x74\x2A\x4C\x8B\x05\x00\x00\x00\x00", "xxx????xxxxx????").ResolveRelativeAddressSelf(0x3, 0x7);
#endif
}

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
class HEngine : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| VAR: g_pEngine                  : 0x" << std::hex << std::uppercase << g_pEngineBuffer.GetPtr() << std::setw(npad) << " |" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HEngine);