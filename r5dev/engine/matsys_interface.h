#ifndef MATSYS_INTERFACE_H
#define MATSYS_INTERFACE_H

#include "public/imaterialsystem.h"
#include "public/inputsystem/ButtonCode.h"

//-------------------------------------------------------------------------
// RUNTIME: GAME_CFG
//-------------------------------------------------------------------------
inline void(*v_UpdateMaterialSystemConfig)(void);
inline bool(*v_UpdateCurrentVideoConfig)(MaterialSystem_Config_t* const pConfig);
inline bool(*v_HandleConfigFile)(const int configType); //(saved games cfg) 0 = local, 1 = profile.
inline void(*v_ResetPreviousGameState)(void);
inline void(*v_LoadPlayerConfig)(ButtonCode_t buttonCode, void* unused);


///////////////////////////////////////////////////////////////////////////////
class VMatSys_Interface : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("UpdateMaterialSystemConfig", v_UpdateMaterialSystemConfig);
		LogFunAdr("UpdateCurrentVideoConfig", v_UpdateCurrentVideoConfig);
		LogFunAdr("HandleConfigFile", v_HandleConfigFile);
		LogFunAdr("ResetPreviousGameState", v_ResetPreviousGameState);
		LogFunAdr("LoadPlayerConfig", v_LoadPlayerConfig);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 57 48 83 EC 20 80 3D ?? ?? ?? ?? ?? 0F 84 ?? ?? ?? ??").GetPtr(v_UpdateMaterialSystemConfig);
		g_GameDll.FindPatternSIMD("40 55 ?? 41 56 48 8D AC 24 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 4C 8B F1").GetPtr(v_UpdateCurrentVideoConfig);
		g_GameDll.FindPatternSIMD("40 56 48 81 EC ?? ?? ?? ?? 8B F1").GetPtr(v_HandleConfigFile);
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 44 89 3D ?? ?? ?? ?? ?? 8B ?? 24 ??").ResolveRelativeAddressSelf(0x1, 0x5).GetPtr(v_ResetPreviousGameState);
		g_GameDll.FindPatternSIMD("E9 ?? ?? ?? ?? CC CC CC CC CC CC CC CC CC CC CC 40 53 48 83 EC 30 4D 8B D1").FollowNearCallSelf().GetPtr(v_LoadPlayerConfig);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // MATSYS_INTERFACE_H
