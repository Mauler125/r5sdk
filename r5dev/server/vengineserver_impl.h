#pragma once

/* ==== CVENGINESERVER ================================================================================================================================================== */
inline CMemory p_IVEngineServer__PersistenceAvailable;
inline auto IVEngineServer__PersistenceAvailable = p_IVEngineServer__PersistenceAvailable.RCast<bool (*)(void* entidx, int clientidx)>();

inline CMemory p_IVEngineServer__IsDedicatedServer;
inline auto IVEngineServer__IsDedicatedServer = p_IVEngineServer__IsDedicatedServer.RCast<bool (*)(void)>();

inline CMemory p_IVEngineServer__GetNumHumanPlayers;
inline auto IVEngineServer__GetNumHumanPlayers = p_IVEngineServer__GetNumHumanPlayers.RCast<int64_t(*)(void)>();

inline CMemory p_IVEngineServer__GetNumFakeClients;
inline auto IVEngineServer__GetNumFakeClients = p_IVEngineServer__GetNumFakeClients.RCast<int64_t(*)(void)>();

inline bool* g_bDedicated = nullptr;

///////////////////////////////////////////////////////////////////////////////
bool HIVEngineServer__PersistenceAvailable(void* entidx, int clientidx);

void IVEngineServer_Attach();
void IVEngineServer_Detach();

///////////////////////////////////////////////////////////////////////////////
extern bool g_bIsPersistenceVarSet[MAX_PLAYERS];

///////////////////////////////////////////////////////////////////////////////
class HVEngineServer : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: IVEngineServer::PersistenceAvailable : {:#18x} |\n", p_IVEngineServer__PersistenceAvailable.GetPtr());
		spdlog::debug("| FUN: IVEngineServer::IsDedicatedServer    : {:#18x} |\n", p_IVEngineServer__IsDedicatedServer.GetPtr());
		spdlog::debug("| FUN: IVEngineServer::GetNumHumanPlayers   : {:#18x} |\n", p_IVEngineServer__GetNumHumanPlayers.GetPtr());
		spdlog::debug("| FUN: IVEngineServer::GetNumFakeClients    : {:#18x} |\n", p_IVEngineServer__GetNumFakeClients.GetPtr());
		spdlog::debug("| VAR: g_bDedicated                         : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_bDedicated));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_IVEngineServer__PersistenceAvailable = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x3B\x15\x00\x00\x00\x00\x7D\x33"), "xx????xx");
		p_IVEngineServer__IsDedicatedServer    = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x0F\xB6\x05\x00\x00\x00\x00\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48\x8B\x05\x00\x00\x00\x00\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x40\x53"), "xxx????xxxxxxxxxxxx????xxxxxxxxxxx");
		p_IVEngineServer__GetNumHumanPlayers   = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x8B\x15\x00\x00\x00\x00\x33\xC0\x85\xD2\x7E\x24"), "xx????xxxxxx");
		p_IVEngineServer__GetNumFakeClients    = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x8B\x05\x00\x00\x00\x00\x33\xC9\x85\xC0\x7E\x2D"), "xx????xxxxxx");

		IVEngineServer__PersistenceAvailable = p_IVEngineServer__PersistenceAvailable.RCast<bool (*)(void* entidx, int clientidx)>(); /*3B 15 ?? ?? ?? ?? 7D 33*/
		IVEngineServer__IsDedicatedServer    = p_IVEngineServer__IsDedicatedServer.RCast<bool (*)(void)>();                           /*0F B6 05 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC 48 8B 05 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC 40 53*/
		IVEngineServer__GetNumHumanPlayers   = p_IVEngineServer__GetNumHumanPlayers.RCast<int64_t(*)(void)>();                        /*8B 15 ?? ?? ?? ?? 33 C0 85 D2 7E 24*/
		IVEngineServer__GetNumFakeClients    = p_IVEngineServer__GetNumFakeClients.RCast<int64_t(*)(void)>();                         /*8B 05 ?? ?? ?? ?? 33 C9 85 C0 7E 2D*/
	}
	virtual void GetVar(void) const
	{
		g_bDedicated = p_IVEngineServer__IsDedicatedServer.ResolveRelativeAddress(0x3, 0x7).RCast<bool*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HVEngineServer);
