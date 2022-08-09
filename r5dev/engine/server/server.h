#pragma once
#include "tier1/NetAdr2.h"
#include "networksystem/pylon.h"
#include "engine/client/client.h"
#include "engine/networkstringtable.h"
#include "public/iserver.h"
#ifndef CLIENT_DLL
#include "server/vengineserver_impl.h"
#endif // !CLIENT_DLL

enum class server_state_t
{
	ss_dead = 0,	// Dead
	ss_loading,		// Spawning
	ss_active,		// Running
	ss_paused,		// Running, but paused
};

struct user_creds_s
{
	v_netadr_t m_nAddr;
	int32_t  m_nProtocolVer;
	int32_t  m_nchallenge;
	uint8_t  gap2[8];
	uint64_t m_nNucleusID;
	uint8_t* m_nUserID;
};

class CServer : public IServer
{
public:
	int	GetTick(void) const { return m_nTickCount; }
#ifndef CLIENT_DLL // Only the connectionless packet handler is implemented on the client via the IServer base class.
	int GetNumHumanPlayers(void) const;
	int GetNumFakeClients(void) const;
	const char* GetMapName(void) const { return m_szMapname; }
	const char* GetMapGroupName(void) const { return m_szMapGroupName; }
	int GetNumClasses(void) const { return m_nServerClasses; }
	int GetClassBits(void) const { return m_nServerClassBits; }
	bool IsActive(void) const { return m_State >= server_state_t::ss_active; }
	bool IsLoading(void) const { return m_State == server_state_t::ss_loading; }
	bool IsDedicated(void) const { return g_bDedicated; }
	static CClient* Authenticate(CServer* pServer, user_creds_s* pInpacket);
#endif // !CLIENT_DLL

private:
	server_state_t                m_State;                       // some actions are only valid during load
	int                           m_Socket;                      // network socket 
	int                           m_nTickCount;                  // current server tick
	bool                          m_bResetMaxTeams;              // reset max players on the server
	char                          m_szMapname[MAX_MAP_NAME];     // map name and path without extension
	char                          m_szMapGroupName[64];          // map group name
	char                          m_szPassword[32];              // server password
	uint32_t                      m_WorldmapCRC;                 // for detecting that client has a hacked local copy of map, the client will be dropped if this occurs.
	uint32_t                      m_ClientDllCRC;                // the dll that this server is expecting clients to be using.
	CNetworkStringTableContainer* m_StringTables;                // network string table container
	CNetworkStringTable*          m_pInstanceBaselineTable;      // instancebaseline
	CNetworkStringTable*          m_pLightStyleTable;            // lightstyles
	CNetworkStringTable*          m_pUserInfoTable;              // userinfo
	CNetworkStringTable*          m_pServerQueryTable;           // server_query_inf
	bool                          m_bReplay;                     // MAYBE
	bool                          m_bUpdateFrame;                // perform snapshot update
	bool                          m_bUseReputation;              // use of player reputation on the server
	bool                          m_bSimulating;                 // are we simulating or not
	int                           m_nPad;                        // padding
	bf_write                      m_Signon;                      // signon bitbuf
	CUtlMemory<byte>              m_SignonBuffer;                // signon memory
	int                           m_nServerClasses;              // number of unique server classes
	int                           m_nServerClassBits;            // log2 of serverclasses
	char                          m_szHostInfo[128];             // see '[r5apex_ds.exe + 0x237740]' for more details. fmt: '[IPv6]:PORT:TIMEi64u'
	char                          m_nGap0[0x4A290];              // TODO: Reverse the rest in this gap.
#if defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	char                          m_nGap1[0x80];
#endif
};
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
static_assert(sizeof(CServer) == 0x4A440);
#else
static_assert(sizeof(CServer) == 0x4A4C0);
#endif

extern CServer* g_pServer;

/* ==== CSERVER ========================================================================================================================================================= */
inline CMemory p_CServer_Think;
inline auto v_CServer_Think = p_CServer_Think.RCast<void (*)(bool bCheckClockDrift, bool bIsSimulating)>();

inline CMemory p_CServer_Authenticate;
inline auto v_CServer_Authenticate = p_CServer_Authenticate.RCast<CClient* (*)(CServer* pServer, user_creds_s* pCreds)>();

inline CMemory p_CServer_RejectConnection;
inline auto v_CServer_RejectConnection = p_CServer_RejectConnection.RCast<void* (*)(CServer* pServer, int iSocket, user_creds_s* pCreds, const char* szMessage)>();

void CServer_Attach();
void CServer_Detach();

extern bool g_bCheckCompBanDB;

///////////////////////////////////////////////////////////////////////////////
class VServer : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: CServer::Think                       : {:#18x} |\n", p_CServer_Think.GetPtr());
		spdlog::debug("| FUN: CServer::Authenticate                : {:#18x} |\n", p_CServer_Authenticate.GetPtr());
		spdlog::debug("| FUN: CServer::RejectConnection            : {:#18x} |\n", p_CServer_RejectConnection.GetPtr());
		spdlog::debug("| VAR: g_pServer[128]                       : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pServer));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_CServer_Think = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x81\xEC\x00\x00\x00\x00\x80\x3D\x00\x00\x00\x00\x00"), "xxxx?xxxx?xxxx????xx?????");
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CServer_Authenticate = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x44\x89\x44\x24\x00\x55\x56\x57\x48\x8D\xAC\x24\x00\x00\x00\x00"), "xxxx?xxxxxxx????");
#elif defined (GAMEDLL_S2)
		p_CServer_Authenticate = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x44\x89\x44\x24\x00\x56\x57\x48\x81\xEC\x00\x00\x00\x00"), "xxxx?xxxxx????");
#else
		p_CServer_Authenticate = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x55\x57\x41\x55\x41\x57\x48\x8D\xAC\x24\x00\x00\x00\x00"), "xxxxxxxxxxx????");
#endif
		p_CServer_RejectConnection = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x4C\x89\x4C\x24\x00\x53\x55\x56\x57\x48\x81\xEC\x00\x00\x00\x00\x49\x8B\xD9"), "xxxx?xxxxxxx????xxx");

		v_CServer_Think = p_CServer_Think.RCast<void (*)(bool, bool)>();                                                       /*48 89 5C 24 ?? 48 89 74 24 ?? 57 48 81 EC ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ??*/
		v_CServer_Authenticate = p_CServer_Authenticate.RCast<CClient* (*)(CServer*, user_creds_s*)>();                        /*40 55 57 41 55 41 57 48 8D AC 24 ?? ?? ?? ??*/
		v_CServer_RejectConnection = p_CServer_RejectConnection.RCast<void* (*)(CServer*, int, user_creds_s*, const char*)>(); /*4C 89 4C 24 ?? 53 55 56 57 48 81 EC ?? ?? ?? ?? 49 8B D9*/
	}
	virtual void GetVar(void) const
	{
		g_pServer = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x57\x48\x83\xEC\x20\x48\x0F\xBF\xD1"), "xxxx?xxxxxxxxx")
			.FindPatternSelf("48 8D 3D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<CServer*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VServer);
