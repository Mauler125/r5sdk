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
inline int(*CModAppSystemGroup__Main)(CModAppSystemGroup* pModAppSystemGroup);
inline bool(*CModAppSystemGroup__Create)(CModAppSystemGroup* pModAppSystemGroup);
inline bool(*CSourceAppSystemGroup__PreInit)(CSourceAppSystemGroup* pModAppSystemGroup);
inline bool(*CSourceAppSystemGroup__Create)(CSourceAppSystemGroup* pModAppSystemGroup);

inline bool g_bAppSystemInit = false;

/* ==== UTILITY ========================================================================================================================================================= */
inline int(*Sys_Error_Internal)(char* fmt, va_list args);

inline bool* gfExtendedError = nullptr;

///////////////////////////////////////////////////////////////////////////////
int HSys_Error_Internal(char* fmt, va_list args);

///////////////////////////////////////////////////////////////////////////////
class VSys_Dll : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CModAppSystemGroup::Main", CModAppSystemGroup__Main);
		LogFunAdr("CModAppSystemGroup::Create", CModAppSystemGroup__Create);
		LogFunAdr("CSourceAppSystemGroup::PreInit", CSourceAppSystemGroup__PreInit);
		LogFunAdr("CSourceAppSystemGroup::Create", CSourceAppSystemGroup__Create);
		LogFunAdr("Sys_Error_Internal", Sys_Error_Internal);
		LogVarAdr("gfExtendedError", gfExtendedError);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 80 B9 ?? ?? ?? ?? ?? BB ?? ?? ?? ??").GetPtr(CModAppSystemGroup__Main);
		g_GameDll.FindPatternSIMD("48 8B C4 55 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC 60").GetPtr(CModAppSystemGroup__Create);

		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 8B F9 E8 ?? ?? ?? ?? 33 C9").GetPtr(CSourceAppSystemGroup__Create);
		g_GameDll.FindPatternSIMD("48 89 74 24 ?? 55 48 8D AC 24 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ??").GetPtr(CSourceAppSystemGroup__PreInit);

		g_GameDll.FindPatternSIMD("48 89 5C 24 08 48 89 74 24 10 57 48 81 EC 30 08 ?? ?? 48 8B DA 48 8B F9 E8 ?? ?? ?? FF 33 F6 48").GetPtr(Sys_Error_Internal);
	}
	virtual void GetVar(void) const
	{
		gfExtendedError = CMemory(v_COM_ExplainDisconnection).Offset(0x0)
			.FindPatternSelf("C6 05", CMemory::Direction::DOWN, 300).ResolveRelativeAddressSelf(0x2, 0x7).RCast<bool*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
