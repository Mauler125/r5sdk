#ifndef MATSYS_INTERFACE_H
#define MATSYS_INTERFACE_H

#include "public/imaterialsystem.h"

//-------------------------------------------------------------------------
// RUNTIME: GAME_CFG
//-------------------------------------------------------------------------
inline CMemory p_UpdateCurrentVideoConfig;
inline CMemory p_UpdateMaterialSystemConfig;
inline CMemory p_HandleConfigFile;
inline CMemory p_ResetPreviousGameState;
inline CMemory p_LoadPlayerConfig;

inline bool(*v_UpdateCurrentVideoConfig)(MaterialSystem_Config_t* pConfig);


///////////////////////////////////////////////////////////////////////////////
class VMatSys_Interface : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("UpdateMaterialSystemConfig", p_UpdateMaterialSystemConfig.GetPtr());
		LogFunAdr("UpdateCurrentVideoConfig", p_UpdateCurrentVideoConfig.GetPtr());
		LogFunAdr("HandleConfigFile", p_HandleConfigFile.GetPtr());
		LogFunAdr("ResetPreviousGameState", p_ResetPreviousGameState.GetPtr());
		LogFunAdr("LoadPlayerConfig", p_LoadPlayerConfig.GetPtr());
	}
	virtual void GetFun(void) const
	{
		p_UpdateMaterialSystemConfig = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 57 48 83 EC 20 80 3D ?? ?? ?? ?? ?? 0F 84 ?? ?? ?? ??");
		p_UpdateCurrentVideoConfig = g_GameDll.FindPatternSIMD("40 55 ?? 41 56 48 8D AC 24 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 4C 8B F1");
		p_HandleConfigFile = g_GameDll.FindPatternSIMD("40 56 48 81 EC ?? ?? ?? ?? 8B F1");
		p_ResetPreviousGameState = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 44 89 3D ?? ?? ?? ?? ?? 8B ?? 24 ??").ResolveRelativeAddressSelf(0x1, 0x5);
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1) || defined (GAMEDLL_S2)
		p_LoadPlayerConfig = g_GameDll.FindPatternSIMD("48 81 EC ?? ?? ?? ?? 48 83 3D ?? ?? ?? ?? ?? 75 0C");
#elif defined (GAMEDLL_S3)
		p_LoadPlayerConfig = g_GameDll.FindPatternSIMD("E9 ?? ?? ?? ?? CC CC CC CC CC CC CC CC CC CC CC 40 53 48 83 EC 30 4D 8B D1").FollowNearCallSelf();
#endif

		v_UpdateCurrentVideoConfig = p_UpdateCurrentVideoConfig.RCast<bool (*)(MaterialSystem_Config_t*)>();
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // MATSYS_INTERFACE_H
