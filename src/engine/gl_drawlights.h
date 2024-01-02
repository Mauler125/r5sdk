#pragma once

inline void(*v_DrawLightSprites)(void*);

///////////////////////////////////////////////////////////////////////////////
class VGL_DrawLights : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("DrawLightSprites", v_DrawLightSprites);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 8B C4 55 57 48 8D 68 A1 48 81 EC ?? ?? ?? ?? 48 8B 15 ?? ?? ?? ??").GetPtr(v_DrawLightSprites);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
