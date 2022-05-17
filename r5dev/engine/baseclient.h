#pragma once
#include "vpc/keyvalues.h"
#include "common/protocol.h"
#include "engine/net_chan.h"
#include "server/vengineserver_impl.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CBaseServer;
class CBaseClient;

///////////////////////////////////////////////////////////////////////////////
extern CBaseClient* g_pClient;

char pad_0014[84];
int64_t m_nReputation;
char pad_0060[752];

class CBaseClient : INetChannelHandler, IClientMessageHandler
{
public:
	CBaseClient* GetClient(int nIndex) const;
	int32_t GetUserID(void) const;
	int64_t GetOriginID(void) const;
	SIGNONSTATE GetSignonState(void) const;
	PERSISTENCE GetPersistenceState(void) const;
	CNetChan* GetNetChan(void) const;
	void SetUserID(int32_t nUserID);
	void SetOriginID(int64_t nOriginID);
	void SetSignonState(SIGNONSTATE nSignonState);
	void SetPersistenceState(PERSISTENCE nPersistenceState);
	void SetNetChan(CNetChan* pNetChan); // !TODO: HACK!
	bool IsConnected(void) const;
	bool IsSpawned(void) const;
	bool IsActive(void) const;
	bool IsPersistenceAvailable(void) const;
	bool IsPersistenceReady(void) const;
	bool IsFakeClient(void) const;
	bool IsHumanPlayer(void) const;
	static bool Connect(CBaseClient* pClient, const char* szName, void* pNetChannel, bool bFakePlayer, void* a5, char* szMessage, int nMessageSize);
	static void Clear(CBaseClient* pBaseClient);

private:
	// [ PIXIE ]: AMOS PLEASE VERIFY STRUCT INTEGRITY FOR EARLIER SEASONS. THERE WAS A PADDING AFTER ORIGINID BEFORE.
	int32_t m_nUserID;               //0x0010
	char pad_0014[68];               //0x0014
	int64_t m_nReputation;           //0x0058
	char pad_0015[768];              //0x0060
	KeyValues* m_ConVars;            //0x0360
	char pad_0368[8];                //0x0368
	CBaseServer* m_Server;           //0x0370
	char pad_0378[40];               //0x0378
	CNetChan* m_NetChannel;          //0x03A0
	char pad_03A8[8];                //0x03A8
	SIGNONSTATE m_nSignonState;      //0x03B0
	int32_t m_nDeltaTick;            //0x03B4
	int64_t m_nOriginID;             //0x03B8
	int32_t m_nStringTableAckTick;   //0x03BC
	int32_t m_nSignonTick;           //0x03C0
	char pad_03C0[472];              //0x03C4
	bool m_bFakePlayer;              //0x05A0
	bool m_bReceivedPacket;          //0x05A1
	bool m_bLowViolence;             //0x05A2
	bool m_bFullyAuthenticated;      //0x05A3
	char pad_05A4[24];               //0x05A4
	PERSISTENCE m_nPersistenceState; //0x05BC
	char pad_05C0[302676];           //0x05C0
	int32_t m_LastMovementTick;      //0x4A414
	char pad_4A418[168];             //0x4A418
};
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
static_assert(sizeof(CBaseClient) == 0x4A440);
#else
static_assert(sizeof(CBaseClient) == 0x4A4C0);
#endif


/* ==== CBASECLIENT ===================================================================================================================================================== */
inline CMemory p_CBaseClient_Connect;
inline auto CBaseClient_Connect = p_CBaseClient_Connect.RCast<bool (*)(CBaseClient* thisptr, const char* szName, void* pNetChannel, bool bFakePlayer, void* a5, char* szMessage, int nMessageSize)>();

inline CMemory p_CBaseClient_Clear;
inline auto CBaseClient_Clear = p_CBaseClient_Clear.RCast<void (*)(CBaseClient* thisptr)>();

inline CMemory g_pClientBuffer;
extern CBaseClient* g_pClient;

// Notes for earlier seasons.
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
inline const std::uintptr_t g_dwCClientSize = 0x4A440;
inline const std::uintptr_t g_dwPersistenceVar = 0x5B4;
inline const std::uintptr_t g_dwCClientPadding = 0x49E88;
#endif


///////////////////////////////////////////////////////////////////////////////
void CBaseClient_Attach();
void CBaseClient_Detach();

///////////////////////////////////////////////////////////////////////////////
class VBaseClient : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: CBaseClient::Connect                 : {:#18x} |\n", p_CBaseClient_Connect.GetPtr());
		spdlog::debug("| FUN: CBaseClient::Clear                   : {:#18x} |\n", p_CBaseClient_Clear.GetPtr());
		spdlog::debug("| VAR: g_pClient                            : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pClient));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_CBaseClient_Connect = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x41\x56\x41\x57\x48\x83\xEC\x20\x48\x8B\xD9\x48\x89\x74"), "xxxxxxxxxxxxxxxx");
		p_CBaseClient_Clear   = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x41\x56\x41\x57\x48\x83\xEC\x20\x48\x8B\xD9\x48\x89\x74"), "xxxxxxxxxxxxxxxx");

		CBaseClient_Connect = p_CBaseClient_Connect.RCast<bool (*)(CBaseClient*, const char*, void*, bool, void*, char*, int)>(); /*40 53 41 56 41 57 48 83 EC 20 48 8B D9 48 89 74*/
		CBaseClient_Clear   = p_CBaseClient_Clear.RCast<void (*)(CBaseClient*)>();                                                /*40 53 41 56 41 57 48 83 EC 20 48 8B D9 48 89 74*/
	}
	virtual void GetVar(void) const
	{
		g_pClientBuffer = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x3B\x15\x00\x00\x00\x00\x7D\x33"), "xx????xx")
			.FindPatternSelf("48 8D 0D", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7);
		g_pClient = g_pClientBuffer.RCast<CBaseClient*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VBaseClient);
