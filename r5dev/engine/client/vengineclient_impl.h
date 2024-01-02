#pragma once

class CEngineClient
{
public:
	void SetRestrictServerCommands(bool bRestrict);
	bool GetRestrictServerCommands() const;
	void SetRestrictClientCommands(bool bRestrict);
	bool GetRestrictClientCommands() const;
	int GetLocalPlayer(); // Local player index.

	// Hook statics:
	static void _ClientCmd(CEngineClient* thisptr, const char* const szCmdString);
};

/* ==== CVENGINECLIENT ================================================================================================================================================== */
///////////////////////////////////////////////////////////////////////////////
inline void(*CEngineClient__ClientCmd)(CEngineClient* thisptr, const char* const szCmdString);

inline CMemory g_pEngineClientVFTable = nullptr;
inline CEngineClient* g_pEngineClient = nullptr;

///////////////////////////////////////////////////////////////////////////////
class HVEngineClient : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogConAdr("CEngineClient::`vftable'", (void*)g_pEngineClientVFTable.GetPtr());
		LogFunAdr("CEngineClient::ClientCmd", CEngineClient__ClientCmd);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 80 3D ?? ?? ?? ?? ?? 48 8B DA 74 0C").GetPtr(CEngineClient__ClientCmd);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const 
	{
		g_pEngineClientVFTable = g_GameDll.GetVirtualMethodTable(".?AVCEngineClient@@");
		g_pEngineClient = g_pEngineClientVFTable.RCast<CEngineClient*>();
	}
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
