#pragma once
#include <launcher/IApplication.h>
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
	bool Load(bool dedicated, const char* rootDir);
	void Unload();
	void SetNextState(EngineState_t iNextState);
	EngineState_t GetState();
	void Frame();
	float GetFrameTime();
	float GetPreviousTime();
	void SetQuitting(EngineDllQuitting_t quitDllState);
	// __m128 __fastcall GetCurTime()

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
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	static ADDRESS g_pEngineBuffer = p_IAppSystem_Main.Offset(0x0).FindPatternSelf("48 8D ?? ?? ?? ?? 01", ADDRESS::Direction::DOWN, 300).ResolveRelativeAddressSelf(0x3, 0x7);
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	static ADDRESS g_pEngineBuffer = p_IAppSystem_Main.Offset(0x0).FindPatternSelf("48 8B ?? ?? ?? ?? 01", ADDRESS::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7);
#endif
}

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
class HEngine : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| VAR: g_pEngine                            : 0x" << std::hex << std::uppercase << g_pEngineBuffer.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HEngine);