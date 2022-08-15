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
inline CMemory g_pEngineClientVFTable = nullptr;
inline CEngineClient* g_pEngineClient = nullptr;

///////////////////////////////////////////////////////////////////////////////
class HVEngineClient : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: IVEngineClient::CommandExecute       : {:#18x} |\n", p_CEngineClient_CommandExecute.GetPtr());
		spdlog::debug("| FUN: IVEngineClient::GetLocalPlayer       : {:#18x} |\n", p_CEngineClient_GetLocalPlayer.GetPtr());
		spdlog::debug("| CON: g_pEngineClientVFTable               : {:#18x} |\n", g_pEngineClientVFTable.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_CEngineClient_CommandExecute = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x20\x48\x8D\x0D\x27\x61\xa5\x1E\x41\x8B\xD8"), "xxxx?xxxxxxxx????xxx");
		CEngineClient_CommandExecute = p_CEngineClient_CommandExecute.RCast<void(*)(void* thisptr, const char* pCmd)>(); /*48 89 5C 24 ?? 57 48 83 EC 20 48 8D 0D ?? ?? ?? ?? 41 8B D8*/

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CEngineClient_GetLocalPlayer = g_pEngineClient_VTable.WalkVTable(35).Deref().RCast<void*(*)()>();
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CEngineClient_GetLocalPlayer = g_pEngineClientVFTable.WalkVTable(36).Deref().RCast<void*(*)()>();
#endif
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const 
	{
		g_pEngineClientVFTable = g_GameDll.GetVirtualMethodTable(".?AVCEngineClient@@");
		g_pEngineClient = g_pEngineClientVFTable.RCast<CEngineClient*>();
	}
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HVEngineClient);