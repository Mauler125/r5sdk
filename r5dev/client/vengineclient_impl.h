#pragma once

class CEngineClient
{
public:
	void SetRestrictServerCommands(bool bRestrict);
	bool GetRestrictServerCommands() const;
	void SetRestrictClientCommands(bool bRestrict);
	bool GetRestrictClientCommands() const;
	int GetLocalPlayer(); // Local player index.
};

/* ==== CVENGINECLIENT ================================================================================================================================================== */
inline CMemory p_CEngineClient_CommandExecute;
inline auto CEngineClient_CommandExecute = p_CEngineClient_CommandExecute.RCast<void(*)(void* thisptr, const char* pCmd)>();

inline CMemory p_CEngineClient_GetLocalPlayer;
inline auto CEngineClient_GetLocalPlayer = p_CEngineClient_GetLocalPlayer.RCast<void*(*)()>();

///////////////////////////////////////////////////////////////////////////////
inline CEngineClient** g_ppEngineClient = nullptr;
inline CMemory g_pEngineClient_VTable = nullptr;

extern bool* m_bRestrictServerCommands;
extern bool* m_bRestrictClientCommands;

///////////////////////////////////////////////////////////////////////////////
class HVEngineClient : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: IVEngineClient::CommandExecute       : {:#18x} |\n", p_CEngineClient_CommandExecute.GetPtr());
		spdlog::debug("| FUN: IVEngineClient::GetLocalPlayer       : {:#18x} |\n", p_CEngineClient_GetLocalPlayer.GetPtr());
		spdlog::debug("| VAR: m_bRestrictServerCommands            : {:#18x} |\n", reinterpret_cast<uintptr_t>(m_bRestrictServerCommands));
		spdlog::debug("| VAR: m_bRestrictClientCommands            : {:#18x} |\n", reinterpret_cast<uintptr_t>(m_bRestrictClientCommands));
		spdlog::debug("| CON: g_ppEngineClient                     : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_ppEngineClient));
		spdlog::debug("| CON: g_pEngineClient_VTable               : {:#18x} |\n", g_pEngineClient_VTable.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_CEngineClient_CommandExecute = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x20\x48\x8D\x0D\x27\x61\xa5\x1E\x41\x8B\xD8"), "xxxx?xxxxxxxx????xxx");
		CEngineClient_CommandExecute = p_CEngineClient_CommandExecute.RCast<void(*)(void* thisptr, const char* pCmd)>(); /*48 89 5C 24 ?? 57 48 83 EC 20 48 8D 0D ?? ?? ?? ?? 41 8B D8*/

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CEngineClient_GetLocalPlayer = g_pEngineClient_VTable.WalkVTable(35).Deref().RCast<void*(*)()>();
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CEngineClient_GetLocalPlayer = g_pEngineClient_VTable.WalkVTable(36).Deref().RCast<void*(*)()>();
#endif
	}
	virtual void GetVar(void) const
	{
		CMemory clRestrict = g_mGameDll.FindString("DevShotGenerator_Init()").FindPatternSelf("88 05", CMemory::Direction::UP).ResolveRelativeAddressSelf(0x2).OffsetSelf(0x2);
		m_bRestrictServerCommands = clRestrict.RCast<bool*>();
		m_bRestrictClientCommands = clRestrict.Offset(0x1).RCast<bool*>();
	}
	virtual void GetCon(void) const 
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		g_pEngineClient_VTable = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8D\x05\x00\x00\x00\x00\x48\x8B\xD9\x48\x89\x01\xF6\xC2\x01\x74\x0A\xBA\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8B\xC3\x48\x83\xC4\x20\x5B\xC3\xCC\xCC\xCC\xCC\xCC\x48\x85\xC9\x48\x8D\x41\xF8"),
			"xxx????xxxxxxxxxxxx????x????xxxxxxxxxxxxxxxxxxxxx").ResolveRelativeAddressSelf(0x3, 0x7); /*48 8D 05 ? ? ? ? 48 8B D9 48 89 01 F6 C2 01 74 0A BA ? ? ? ? E8 ? ? ? ? 48 8B C3 48 83 C4 20 5B C3 CC CC CC CC CC 48 85 C9 48 8D 41 F8*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		g_pEngineClient_VTable = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\x05\x00\x00\x00\x00\xFF\x90\x00\x00\x00\x00\x4C\x8D\x05\x00\x00\x00\x00"), "xxx????xx????xxx????").ResolveRelativeAddressSelf(0x3, 0x7).Deref(); /*48 8B 05 ? ? ? ? FF 90 ? ? ? ? 4C 8D 05 ? ? ? ? */
#endif
		g_ppEngineClient = g_mGameDll.FindString("reload_script_callbacks_server").FindPatternSelf("48 8B", CMemory::Direction::UP).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CEngineClient**>();
	}
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HVEngineClient);