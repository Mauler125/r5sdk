#pragma once
#include <public/iengine.h>

class CEngine;

/* ==== CENGINE ======================================================================================================================================================= */
inline CMemory p_CEngine_Frame;
inline bool(*v_CEngine_Frame)(CEngine* thisp);

extern CEngine* g_pEngine;
extern IEngine::QuitState_t* gsm_Quitting;

class CEngine : public IEngine
{
public:
	static bool _Frame(CEngine* thisp);
	inline IEngine::QuitState_t GetQuitting() const { return *gsm_Quitting; }

private:
	EngineState_t m_nDLLState;
	EngineState_t m_nNextDLLState;
	double        m_flCurrentTime;
	double        m_flPreviousTime;
	float         m_flFrameTime;
	float         m_flPreviousFrameTime;
	float         m_flFilteredTime;
	uint8_t       gap2C[4];
	int64_t       field_30;
	char          field_38;
	char          field_39;
};

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
class VEngine : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CEngine::Frame", p_CEngine_Frame.GetPtr());
		LogVarAdr("g_Engine", reinterpret_cast<uintptr_t>(g_pEngine));
		LogVarAdr("sm_Quitting", reinterpret_cast<uintptr_t>(gsm_Quitting));
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CEngine_Frame = g_GameDll.FindPatternSIMD("40 55 53 56 48 8D AC 24 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 48 8B F1");
#elif defined (GAMEDLL_S2)
		p_CEngine_Frame = g_GameDll.FindPatternSIMD("48 8B C4 56 48 81 EC ?? ?? ?? ?? 0F 29 70 B8");
#else
		p_CEngine_Frame = g_GameDll.FindPatternSIMD("48 8B C4 55 56 48 8D A8 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 0F 29 70 B8");
#endif
		v_CEngine_Frame = p_CEngine_Frame.RCast<bool(*)(CEngine* thisp)>();
	}
	virtual void GetVar(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		g_pEngine = g_GameDll.FindPatternSIMD("48 83 EC 28 80 B9 ?? ?? ?? ?? ?? 48 8B 15 ?? ?? ?? ??").FindPatternSelf("48 8D ?? ?? ?? ?? 01", CMemory::Direction::DOWN, 300).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CEngine*>();
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		g_pEngine = g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 80 B9 ?? ?? ?? ?? ?? BB ?? ?? ?? ??").FindPatternSelf("48 8B ?? ?? ?? ?? 01", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CEngine*>();
#endif
		gsm_Quitting = g_GameDll.FindPatternSIMD("89 15 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC CC 83 C1 F4").ResolveRelativeAddressSelf(0x2, 0x6).RCast<IEngine::QuitState_t*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
