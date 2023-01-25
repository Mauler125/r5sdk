#pragma once
#include <launcher/IApplication.h>
#include <public/iengine.h>

class CEngine : public IEngine
{
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

/* ==== CENGINE ======================================================================================================================================================= */
extern CEngine* g_pEngine;

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
class VEngine : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogVarAdr("g_pEngine", reinterpret_cast<uintptr_t>(g_pEngine));
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		g_pEngine = g_GameDll.FindPatternSIMD("48 83 EC 28 80 B9 ?? ?? ?? ?? ?? 48 8B 15 ?? ?? ?? ??").FindPatternSelf("48 8D ?? ?? ?? ?? 01", CMemory::Direction::DOWN, 300).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CEngine*>();
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		g_pEngine = g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 80 B9 ?? ?? ?? ?? ?? BB ?? ?? ?? ??").FindPatternSelf("48 8B ?? ?? ?? ?? 01", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CEngine*>();
#endif
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////
