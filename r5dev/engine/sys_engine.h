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

class CEngine
{
public:
	bool Load(bool dedicated, const char* rootDir);
	void Unload(void);
	void SetNextState(EngineState_t iNextState);
	EngineState_t GetState(void) const;
	void Frame(void);
	float GetFrameTime(void) const;
	float GetPreviousTime(void);
	__m128 GetCurTime(CEngine* thisPtr) const;
	void SetQuitting(EngineDllQuitting_t quitDllState);

private:
	void*         vtable;
	EngineState_t m_nDLLState;
	EngineState_t m_nNextDLLState;
	int64_t       m_flCurrentTime;
	int64_t       m_flPreviousTime;
	int           m_flFrameTime;
	int           field_24;
	int           m_flFilteredTime;
	uint8_t       gap2C[4];
	int64_t       field_30;
	char          field_38;
	char          field_39;
};

namespace
{
	/* ==== CENGINE ======================================================================================================================================================= */
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	static ADDRESS g_pEngineBuffer = p_CModAppSystemGroup_Main.Offset(0x0).FindPatternSelf("48 8D ?? ?? ?? ?? 01", ADDRESS::Direction::DOWN, 300).ResolveRelativeAddressSelf(0x3, 0x7);
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	static ADDRESS g_pEngineBuffer = p_CModAppSystemGroup_Main.Offset(0x0).FindPatternSelf("48 8B ?? ?? ?? ?? 01", ADDRESS::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7);
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