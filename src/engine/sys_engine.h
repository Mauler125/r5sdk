#pragma once
#include <public/iengine.h>

class CEngine;

/* ==== CENGINE ======================================================================================================================================================= */
inline bool(*CEngine__Frame)(CEngine* thisp);

extern CEngine* g_pEngine;
extern IEngine::QuitState_t* gsm_Quitting;

class CEngine : public IEngine
{
public:
	static bool _Frame(CEngine* const thisp);
	inline IEngine::QuitState_t GetQuitting() const { return *gsm_Quitting; }

private:
	EngineState_t m_nDLLState;
	EngineState_t m_nNextDLLState;
	double        m_flCurrentTime;
	double        m_flPreviousTime;
	float         m_flFrameTime;
	float         m_flPreviousFrameTime;
	float         m_flFilteredTime;
	char          padding[4]; // <- free data
	double        m_flBenchmarkTime;
	bool          m_bShouldPause;
	bool          m_bPaused;
};
static_assert(sizeof(CEngine) == 0x40);

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
class VEngine : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CEngine::Frame", CEngine__Frame);
		LogVarAdr("g_Engine", g_pEngine);
		LogVarAdr("sm_Quitting", gsm_Quitting);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 8B C4 55 56 48 8D A8 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 0F 29 70 B8").GetPtr(CEngine__Frame);
	}
	virtual void GetVar(void) const
	{
		g_pEngine = g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 80 B9 ?? ?? ?? ?? ?? BB ?? ?? ?? ??").FindPatternSelf("48 8B ?? ?? ?? ?? 01", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CEngine*>();
		gsm_Quitting = g_GameDll.FindPatternSIMD("89 15 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC CC 83 C1 F4").ResolveRelativeAddressSelf(0x2, 0x6).RCast<IEngine::QuitState_t*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
