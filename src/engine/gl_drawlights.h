#pragma once

inline CMemory p_DrawLightSprites;
inline void(*v_DrawLightSprites)(void*);


///////////////////////////////////////////////////////////////////////////////
class VGL_DrawLights : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("DrawLightSprites", p_DrawLightSprites.GetPtr());
	}
	virtual void GetFun(void) const
	{
		p_DrawLightSprites = g_GameDll.FindPatternSIMD("48 8B C4 55 57 48 8D 68 A1 48 81 EC ?? ?? ?? ?? 48 8B 15 ?? ?? ?? ??");
		v_DrawLightSprites = p_DrawLightSprites.RCast<void(*)(void*)>();
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
