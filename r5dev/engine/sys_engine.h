#pragma once
#include <launcher/IApplication.h>

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

/* ==== CENGINE ======================================================================================================================================================= */
extern CEngine* g_pEngine;

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
class VEngine : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| VAR: g_pEngine                            : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pEngine));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		g_pEngine = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x28\x80\xB9\x00\x00\x00\x00\x00\x48\x8B\x15\x00\x00\x00\x00"), "xxxxxx?????xxx????").FindPatternSelf("48 8D ?? ?? ?? ?? 01", CMemory::Direction::DOWN, 300).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CEngine*>();
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		g_pEngine = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\x80\xB9\x00\x00\x00\x00\x00\xBB\x00\x00\x00\x00"), "xxxxxxxx?????x????").FindPatternSelf("48 8B ?? ?? ?? ?? 01", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CEngine*>();
#endif
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VEngine);