#pragma once

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CClient;

/* ==== CVENGINESERVER ================================================================================================================================================== */
inline CMemory p_IVEngineServer__PersistenceAvailable;
inline auto IVEngineServer__PersistenceAvailable = p_IVEngineServer__PersistenceAvailable.RCast<bool (*)(void* entidx, int clientidx)>();

inline CMemory p_IVEngineServer__IsDedicatedServer;
inline auto IVEngineServer__IsDedicatedServer = p_IVEngineServer__IsDedicatedServer.RCast<bool (*)(void)>();

inline CMemory p_IVEngineServer__GetNumHumanPlayers;
inline auto IVEngineServer__GetNumHumanPlayers = p_IVEngineServer__GetNumHumanPlayers.RCast<int64_t(*)(void)>();

inline CMemory p_IVEngineServer__GetNumFakeClients;
inline auto IVEngineServer__GetNumFakeClients = p_IVEngineServer__GetNumFakeClients.RCast<int64_t(*)(void)>();

inline CMemory p_IVEngineServer__CreateFakeClient;
inline auto IVEngineServer__CreateFakeClient = p_IVEngineServer__CreateFakeClient.RCast<CClient*(*)(void* es, const char* name, int team)>();

//inline CMemory p_RunFrameServer;
//inline auto v_RunFrameServer = p_RunFrameServer.RCast<void(*)(double flFrameTime, bool bRunOverlays, bool bUniformUpdate)>();

inline bool* g_bDedicated = nullptr;

///////////////////////////////////////////////////////////////////////////////
bool HIVEngineServer__PersistenceAvailable(void* entidx, int clientidx);

void IVEngineServer_Attach();
void IVEngineServer_Detach();

///////////////////////////////////////////////////////////////////////////////

struct ServerPlayer_t
{
	ServerPlayer_t(void)
		: m_flCurrentNetProcessTime(0.0)
		, m_flLastNetProcessTime(0.0)
		, m_flStringCommandQuotaTimeStart(0.0)
		, m_nStringCommandQuotaCount(0)
		, m_bPersistenceEnabled(false)
	{}
	inline void Reset(void)
	{
		m_flCurrentNetProcessTime = 0.0;
		m_flLastNetProcessTime = 0.0;
		m_flStringCommandQuotaTimeStart = 0.0;
		m_nStringCommandQuotaCount = 0;
		m_bPersistenceEnabled = false;
	}

	double m_flCurrentNetProcessTime;
	double m_flLastNetProcessTime;
	double m_flStringCommandQuotaTimeStart;
	int m_nStringCommandQuotaCount;
	bool m_bPersistenceEnabled;
};

extern ServerPlayer_t g_ServerPlayer[MAX_PLAYERS];

///////////////////////////////////////////////////////////////////////////////
class HVEngineServer : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: IVEngineServer::PersistenceAvailable : {:#18x} |\n", p_IVEngineServer__PersistenceAvailable.GetPtr());
		spdlog::debug("| FUN: IVEngineServer::IsDedicatedServer    : {:#18x} |\n", p_IVEngineServer__IsDedicatedServer.GetPtr());
		spdlog::debug("| FUN: IVEngineServer::GetNumHumanPlayers   : {:#18x} |\n", p_IVEngineServer__GetNumHumanPlayers.GetPtr());
		spdlog::debug("| FUN: IVEngineServer::GetNumFakeClients    : {:#18x} |\n", p_IVEngineServer__GetNumFakeClients.GetPtr());
		spdlog::debug("| FUN: IVEngineServer::CreateFakeClient     : {:#18x} |\n", p_IVEngineServer__CreateFakeClient.GetPtr());
		//spdlog::debug("| FUN: RunFrameServer                       : {:#18x} |\n", p_RunFrameServer.GetPtr());
		spdlog::debug("| VAR: g_bDedicated                         : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_bDedicated));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_IVEngineServer__PersistenceAvailable = g_GameDll.FindPatternSIMD("3B 15 ?? ?? ?? ?? 7D 33");
		p_IVEngineServer__IsDedicatedServer    = g_GameDll.FindPatternSIMD("0F B6 05 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC 48 8B 05 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC 40 53");
		p_IVEngineServer__GetNumHumanPlayers   = g_GameDll.FindPatternSIMD("8B 15 ?? ?? ?? ?? 33 C0 85 D2 7E 24");
		p_IVEngineServer__GetNumFakeClients    = g_GameDll.FindPatternSIMD("8B 05 ?? ?? ?? ?? 33 C9 85 C0 7E 2D");
		p_IVEngineServer__CreateFakeClient     = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 8B F2 41 8B F8");
//		p_RunFrameServer                       = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 57 48 83 EC 30 0F 29 74 24 ?? 48 8D 0D ?? ?? ?? ??");

		IVEngineServer__PersistenceAvailable = p_IVEngineServer__PersistenceAvailable.RCast<bool (*)(void*, int)>();       /*3B 15 ?? ?? ?? ?? 7D 33*/
		IVEngineServer__IsDedicatedServer    = p_IVEngineServer__IsDedicatedServer.RCast<bool (*)(void)>();                /*0F B6 05 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC 48 8B 05 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC 40 53*/
		IVEngineServer__GetNumHumanPlayers   = p_IVEngineServer__GetNumHumanPlayers.RCast<int64_t(*)(void)>();             /*8B 15 ?? ?? ?? ?? 33 C0 85 D2 7E 24*/
		IVEngineServer__GetNumFakeClients    = p_IVEngineServer__GetNumFakeClients.RCast<int64_t(*)(void)>();              /*8B 05 ?? ?? ?? ?? 33 C9 85 C0 7E 2D*/
		IVEngineServer__CreateFakeClient     = p_IVEngineServer__CreateFakeClient.RCast<CClient*(*)(void*, const char*, int)>();/*48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 8B F2 41 8B F8*/
//		v_RunFrameServer                     = p_RunFrameServer.RCast<void(*)(double, bool, bool)>();                        /*48 89 5C 24 ?? 57 48 83 EC 30 0F 29 74 24 ?? 48 8D 0D ?? ?? ?? ??*/
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
