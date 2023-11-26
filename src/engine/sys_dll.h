#pragma once
#include "engine/common.h"
#include "public/appframework/IAppSystem.h"
#include "public/appframework/IAppSystemGroup.h"

//-------------------------------------------------------------------------
// 
//-------------------------------------------------------------------------
class CSourceAppSystemGroup : public CAppSystemGroup
{
public:
	static bool StaticPreInit(CSourceAppSystemGroup* pSourceAppSystemGroup);
	static bool StaticCreate(CSourceAppSystemGroup* pSourceAppSystemGroup);

private:
	CFileSystem_Stdio* m_pFileSystem;
};

//-------------------------------------------------------------------------
// 
//-------------------------------------------------------------------------
class CModAppSystemGroup : public CAppSystemGroup
{
public:
	static int StaticMain(CModAppSystemGroup* pModAppSystemGroup);
	static bool StaticCreate(CModAppSystemGroup* pModAppSystemGroup);
	static void InitPluginSystem(CModAppSystemGroup* pModAppSystemGroup);

	inline bool IsServerOnly(void) const
	{
		return m_bServerOnly;
	}
	inline void SetServerOnly(void)
	{
		m_bServerOnly = true;
	}

private:
	bool m_bServerOnly;
};

/* ==== CAPPSYSTEMGROUP ================================================================================================================================================= */
inline CMemory p_CModAppSystemGroup_Main;
inline int(*CModAppSystemGroup_Main)(CModAppSystemGroup* pModAppSystemGroup);

inline CMemory p_CModAppSystemGroup_Create;
inline bool(*CModAppSystemGroup_Create)(CModAppSystemGroup* pModAppSystemGroup);

inline CMemory p_CSourceAppSystemGroup__PreInit;
inline bool(*CSourceAppSystemGroup__PreInit)(CSourceAppSystemGroup* pModAppSystemGroup);

inline CMemory p_CSourceAppSystemGroup__Create;
inline bool(*CSourceAppSystemGroup__Create)(CSourceAppSystemGroup* pModAppSystemGroup);

inline bool g_bAppSystemInit = false;

/* ==== UTILITY ========================================================================================================================================================= */
inline CMemory p_Sys_Error_Internal;
inline int(*Sys_Error_Internal)(char* fmt, va_list args);

inline bool* gfExtendedError = nullptr;

///////////////////////////////////////////////////////////////////////////////
int HSys_Error_Internal(char* fmt, va_list args);

///////////////////////////////////////////////////////////////////////////////
class VSys_Dll : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CModAppSystemGroup::Main", p_CModAppSystemGroup_Main.GetPtr());
		LogFunAdr("CModAppSystemGroup::Create", p_CModAppSystemGroup_Create.GetPtr());
		LogFunAdr("CSourceAppSystemGroup::PreInit", p_CSourceAppSystemGroup__PreInit.GetPtr());
		LogFunAdr("CSourceAppSystemGroup::Create", p_CSourceAppSystemGroup__Create.GetPtr());
		LogFunAdr("Sys_Error_Internal", p_Sys_Error_Internal.GetPtr());
		LogVarAdr("gfExtendedError", reinterpret_cast<uintptr_t>(gfExtendedError));
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CModAppSystemGroup_Main = g_GameDll.FindPatternSIMD("48 83 EC 28 80 B9 ?? ?? ?? ?? ?? 48 8B 15 ?? ?? ?? ??");
		p_CModAppSystemGroup_Create = g_GameDll.FindPatternSIMD("48 8B C4 57 41 54 41 55 41 56 41 57 48 83 EC 60 48 C7 40 ?? ?? ?? ?? ?? 48 89 58 08");

		p_CSourceAppSystemGroup__Create = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 8B F9 E8 ?? ?? ?? ?? 33 C9");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CModAppSystemGroup_Main = g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 80 B9 ?? ?? ?? ?? ?? BB ?? ?? ?? ??");
		p_CModAppSystemGroup_Create = g_GameDll.FindPatternSIMD("48 8B C4 55 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC 60");

		p_CSourceAppSystemGroup__Create = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 8B F9 E8 ?? ?? ?? ?? 33 C9");
#endif
		p_CSourceAppSystemGroup__PreInit = g_GameDll.FindPatternSIMD("48 89 74 24 ?? 55 48 8D AC 24 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ??");

		CModAppSystemGroup_Main = p_CModAppSystemGroup_Main.RCast<int(*)(CModAppSystemGroup*)>();
		CModAppSystemGroup_Create = p_CModAppSystemGroup_Create.RCast<bool(*)(CModAppSystemGroup*)>();
		CSourceAppSystemGroup__PreInit = p_CSourceAppSystemGroup__PreInit.RCast<bool(*)(CSourceAppSystemGroup*)>();
		CSourceAppSystemGroup__Create = p_CSourceAppSystemGroup__Create.RCast<bool(*)(CSourceAppSystemGroup*)>();

		p_Sys_Error_Internal = g_GameDll.FindPatternSIMD("48 89 5C 24 08 48 89 74 24 10 57 48 81 EC 30 08 ?? ?? 48 8B DA 48 8B F9 E8 ?? ?? ?? FF 33 F6 48");
		Sys_Error_Internal = p_Sys_Error_Internal.RCast<int (*)(char*, va_list)>();
	}
	virtual void GetVar(void) const
	{
		gfExtendedError = p_COM_ExplainDisconnection.Offset(0x0).FindPatternSelf("C6 05", CMemory::Direction::DOWN, 300).ResolveRelativeAddressSelf(0x2, 0x7).RCast<bool*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
