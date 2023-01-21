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

inline auto v_UpdateCurrentVideoConfig = p_UpdateCurrentVideoConfig.RCast<bool (*)(MaterialSystem_Config_t* pConfig)>();


void MatSys_Iface_Attach();
void MatSys_Iface_Detach();
///////////////////////////////////////////////////////////////////////////////
class VMatSys_Interface : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: UpdateMaterialSystemConfig           : {:#18x} |\n", p_UpdateMaterialSystemConfig.GetPtr());
		spdlog::debug("| FUN: UpdateCurrentVideoConfig             : {:#18x} |\n", p_UpdateCurrentVideoConfig.GetPtr());
		spdlog::debug("| FUN: HandleConfigFile                     : {:#18x} |\n", p_HandleConfigFile.GetPtr());
		spdlog::debug("| FUN: ResetPreviousGameState               : {:#18x} |\n", p_ResetPreviousGameState.GetPtr());
		spdlog::debug("| FUN: LoadPlayerConfig                     : {:#18x} |\n", p_LoadPlayerConfig.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_UpdateMaterialSystemConfig = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 57 48 83 EC 20 80 3D ?? ?? ?? ?? ?? 0F 84 ?? ?? ?? ??");
		p_UpdateCurrentVideoConfig = g_GameDll.FindPatternSIMD("40 55 ?? 41 56 48 8D AC 24 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 4C 8B F1");
		p_HandleConfigFile = g_GameDll.FindPatternSIMD("40 56 48 81 EC ?? ?? ?? ?? 8B F1");
		p_ResetPreviousGameState = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 44 89 3D ?? ?? ?? ?? ?? 8B ?? 24 ??").ResolveRelativeAddressSelf(0x1, 0x5);
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1) || defined (GAMEDLL_S2)
		LoadPlayerConfig = g_GameDll.FindPatternSIMD("48 81 EC ?? ?? ?? ?? 48 83 3D ?? ?? ?? ?? ?? 75 0C");
#elif defined (GAMEDLL_S3)
		p_LoadPlayerConfig = g_GameDll.FindPatternSIMD("89 4C 24 08 48 81 EC ?? ?? ?? ?? 48 83 3D ?? ?? ?? ?? ??");
#endif

		v_UpdateCurrentVideoConfig = p_UpdateCurrentVideoConfig.RCast<bool (*)(MaterialSystem_Config_t*)>();
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VMatSys_Interface);

#endif // MATSYS_INTERFACE_H
