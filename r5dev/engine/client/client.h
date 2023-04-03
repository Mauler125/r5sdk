#pragma once
#include "vpc/keyvalues.h"
#include "common/protocol.h"
#include "engine/net_chan.h"
#include "public/edict.h"

//-----------------------------------------------------------------------------
// Enumerations
//-----------------------------------------------------------------------------
enum Reputation_t
{
	REP_NONE = 0,
	REP_REMOVE_ONLY,
	REP_MARK_BAD
};

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CServer;
class CClient;

struct Spike_t
{
public:
	Spike_t() :
		m_nBits(0)
	{
		m_szDesc[0] = 0;
	}

	// !TODO: !unconfirmed!
	char	m_szDesc[64];
	int		m_nBits;
};

class CNetworkStatTrace
{
public:
	CNetworkStatTrace() :
		m_nStartBit(0), m_nCurBit(0), m_nMinWarningBytes(0)
	{
	}
	int						m_nStartBit;
	int						m_nCurBit;
	int						m_nMinWarningBytes;
	CUtlVector< Spike_t >	m_Records;
};

class CClientFrame
{
	// !TODO: !unconfirmed!
	int last_entity;
	int tick_count;
	CClientFrame* m_pNext;
};

///////////////////////////////////////////////////////////////////////////////
extern CClient* g_pClient;

class CClient : IClientMessageHandler, INetChannelHandler
{
public:
	CClient* GetClient(int nIndex) const;
	edict_t GetHandle(void) const;
	uint32_t GetUserID(void) const;
	uint64_t GetNucleusID(void) const;
	SIGNONSTATE GetSignonState(void) const;
	PERSISTENCE GetPersistenceState(void) const;
	CNetChan* GetNetChan(void) const;
	CServer* GetServer(void) const;
	int GetCommandTick(void) const;
	const char* GetServerName(void) const;
	const char* GetClientName(void) const;
	void SetHandle(edict_t nHandle);
	void SetUserID(uint32_t nUserID);
	void SetNucleusID(uint64_t nNucleusID);
	void SetSignonState(SIGNONSTATE nSignonState);
	void SetPersistenceState(PERSISTENCE nPersistenceState);
	void SetNetChan(CNetChan* pNetChan);
	bool IsConnected(void) const;
	bool IsSpawned(void) const;
	bool IsActive(void) const;
	bool IsPersistenceAvailable(void) const;
	bool IsPersistenceReady(void) const;
	bool IsFakeClient(void) const;
	bool IsHumanPlayer(void) const;

	bool SendNetMsg(CNetMessage* pMsg, char bLocal, bool bForceReliable, bool bVoice);
	bool Connect(const char* szName, void* pNetChannel, bool bFakePlayer, void* a5, char* szMessage, int nMessageSize);
	void Disconnect(const Reputation_t nRepLvl, const char* szReason, ...);
	static bool VConnect(CClient* pClient, const char* szName, void* pNetChannel, bool bFakePlayer, void* a5, char* szMessage, int nMessageSize);
	void Clear(void);

public: // Hook statics:
	static void VClear(CClient* pClient);
	static bool VProcessStringCmd(CClient* pClient, NET_StringCmd* pMsg);
	static void* VSendSnapshot(CClient* pClient, CClientFrame* pFrame, int nTick, int nTickAck);

private:
	uint32_t m_nUserID;              //0x0010
	edict_t m_nHandle;               //0x0014
	char m_szServerName[256];        //0x0160
	char m_szClientName[256];        //0x0116
	char pad_0015[258];              //0x0216
	int m_nCommandTick;              //0x0318
	char pad_031C[68];               //0x031C
	KeyValues* m_ConVars;            //0x0360
	char pad_0368[8];                //0x0368
	CServer* m_pServer;              //0x0370
	char pad_0378[40];               //0x0378
	CNetChan* m_NetChannel;          //0x03A0
	char pad_03A8[8];                //0x03A8
	SIGNONSTATE m_nSignonState;      //0x03B0
	int32_t m_nDeltaTick;            //0x03B4
	uint64_t m_nNucleusID;           //0x03B8
	int32_t m_nStringTableAckTick;   //0x03BC
	int32_t m_nSignonTick;           //0x03C0
	char pad_03C0[464];              //0x03C4
#if defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	char pad_0598[8];                //0x0598
#endif
	bool m_bFakePlayer;              //0x05A0
	bool m_bReceivedPacket;          //0x05A1
	bool m_bLowViolence;             //0x05A2
	bool m_bFullyAuthenticated;      //0x05A3
	char pad_05A4[24];               //0x05A4
	PERSISTENCE m_nPersistenceState; //0x05BC
	char pad_05C0[295312];           //0x05C0
	int m_iTracing;                  //0x48750
	CNetworkStatTrace m_Trace;       //0x48754
	char pad_4878C[7304];            //0x4878C
	int32_t m_LastMovementTick;      //0x4A414
#if defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	char pad_4A418[120];             //0x4A418
#endif
	char pad_4A440[48];              //0x4A440
};
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
static_assert(sizeof(CClient) == 0x4A440);
#else
static_assert(sizeof(CClient) == 0x4A4C0);
#endif


/* ==== CBASECLIENT ===================================================================================================================================================== */
inline CMemory p_CClient_Connect;
inline auto v_CClient_Connect = p_CClient_Connect.RCast<bool (*)(CClient* pClient, const char* szName, void* pNetChannel, bool bFakePlayer, void* a5, char* szMessage, int nMessageSize)>();

inline CMemory p_CClient_Disconnect;
inline auto v_CClient_Disconnect = p_CClient_Disconnect.RCast<bool (*)(CClient* pClient, const Reputation_t nRepLvl, const char* szReason, ...)>();

inline CMemory p_CClient_Clear;
inline auto v_CClient_Clear = p_CClient_Clear.RCast<void (*)(CClient* pClient)>();

inline CMemory p_CClient_ProcessStringCmd;
inline auto v_CClient_ProcessStringCmd = p_CClient_ProcessStringCmd.RCast<bool (*)(CClient* pClient, NET_StringCmd* pMsg)>();

inline CMemory p_CClient_SetSignonState;
inline auto v_CClient_SetSignonState = p_CClient_SetSignonState.RCast<bool (*)(CClient* pClient, SIGNONSTATE signon)>();

inline CMemory p_CClient_SendNetMsg;
inline auto v_CClient_SendNetMsg = p_CClient_SendNetMsg.RCast<bool (*)(CClient* pClient, CNetMessage* pMsg, char bLocal, bool bForceReliable, bool bVoice)>();

inline CMemory p_CClient_SendSnapshot;
inline auto v_CClient_SendSnapshot = p_CClient_SendSnapshot.RCast<void* (*)(CClient* pClient, CClientFrame* pFrame, int nTick, int nTickAck)>();

///////////////////////////////////////////////////////////////////////////////
class VClient : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CClient::Connect", p_CClient_Connect.GetPtr());
		LogFunAdr("CClient::Disconnect", p_CClient_Disconnect.GetPtr());
		LogFunAdr("CClient::Clear", p_CClient_Clear.GetPtr());
		LogFunAdr("CClient::ProcessStringCmd", p_CClient_ProcessStringCmd.GetPtr());
		LogFunAdr("CClient::SetSignonState", p_CClient_SetSignonState.GetPtr());
		LogFunAdr("CClient::SendNetMsg", p_CClient_SendNetMsg.GetPtr());
		LogFunAdr("CClient::SendSnapshot", p_CClient_SendSnapshot.GetPtr());
		LogVarAdr("g_Client[128]", reinterpret_cast<uintptr_t>(g_pClient));
	}
	virtual void GetFun(void) const
	{
		p_CClient_Connect    = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 56 57 41 56 48 83 EC 20 41 0F B6 E9");
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1) || defined (GAMEDLL_S2)
		p_CClient_Disconnect = g_GameDll.FindPatternSIMD("48 8B C4 4C 89 40 18 4C 89 48 20 53 56 57 48 81 EC ?? ?? ?? ?? 83 B9 ?? ?? ?? ?? ?? 49 8B F8 0F B6 F2");
#else // !GAMEDLL_S0 || !GAMEDLL_S1 || !GAMEDLL_S2
		p_CClient_Disconnect = g_GameDll.FindPatternSIMD("48 8B C4 4C 89 40 18 4C 89 48 20 53 56 57 48 81 EC ?? ?? ?? ?? 83 B9 ?? ?? ?? ?? ?? 49 8B F8 8B F2");
#endif
		p_CClient_Clear      = g_GameDll.FindPatternSIMD("40 53 41 56 41 57 48 83 EC 20 48 8B D9 48 89 74");
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CClient_ProcessStringCmd = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 55 48 81 EC ?? ?? ?? ?? 49 8B D8");
		p_CClient_SendNetMsg = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 41 56 41 57 48 83 EC 30 48 8B 05 ?? ?? ?? ?? 45 0F B6 F1");
		p_CClient_SendSnapshot = g_GameDll.FindPatternSIMD("44 89 44 24 ?? 48 89 4C 24 ?? 55 53 56 57 41 55");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CClient_ProcessStringCmd = g_GameDll.FindPatternSIMD("48 89 6C 24 ?? 57 48 81 EC ?? ?? ?? ?? 48 8B 7A 20");
		p_CClient_SendNetMsg = g_GameDll.FindPatternSIMD("40 53 55 56 57 41 56 48 83 EC 40 48 8B 05 ?? ?? ?? ??");
		p_CClient_SendSnapshot = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 55 56 41 55 41 56 41 57 48 8D 6C 24 ??");
#endif // !GAMEDLL_S0 || !GAMEDLL_S1
		p_CClient_SetSignonState = g_GameDll.FindPatternSIMD("48 8B C4 48 89 58 10 48 89 70 18 57 48 81 EC ?? ?? ?? ?? 0F 29 70 E8 8B F2");

		v_CClient_Connect    = p_CClient_Connect.RCast<bool (*)(CClient*, const char*, void*, bool, void*, char*, int)>(); /*48 89 5C 24 ?? 48 89 6C 24 ?? 56 57 41 56 48 83 EC 20 41 0F B6 E9*/
		v_CClient_Disconnect = p_CClient_Disconnect.RCast<bool (*)(CClient*, const Reputation_t, const char*, ...)>();     /*48 8B C4 4C 89 40 18 4C 89 48 20 53 56 57 48 81 EC ?? ?? ?? ?? 83 B9 ?? ?? ?? ?? ?? 49 8B F8 8B F2*/
		v_CClient_Clear      = p_CClient_Clear.RCast<void (*)(CClient*)>();                                                /*40 53 41 56 41 57 48 83 EC 20 48 8B D9 48 89 74*/
		v_CClient_ProcessStringCmd = p_CClient_ProcessStringCmd.RCast<bool (*)(CClient*, NET_StringCmd*)>();               /*48 89 6C 24 ?? 57 48 81 EC ?? ?? ?? ?? 48 8B 7A 20*/
		v_CClient_SetSignonState = p_CClient_SetSignonState.RCast<bool (*)(CClient*, SIGNONSTATE)>();
		v_CClient_SendNetMsg = p_CClient_SendNetMsg.RCast<bool (*)(CClient*, CNetMessage*, char, bool, bool)>();
		v_CClient_SendSnapshot = p_CClient_SendSnapshot.RCast<void* (*)(CClient*, CClientFrame*, int, int)>();
	}
	virtual void GetVar(void) const
	{
		g_pClient = g_GameDll.FindPatternSIMD("3B 15 ?? ?? ?? ?? 7D 33")
			.FindPatternSelf("48 8D 0D", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CClient*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const
	{
		DetourAttach((LPVOID*)&v_CClient_Clear, &CClient::VClear);
		DetourAttach((LPVOID*)&v_CClient_Connect, &CClient::VConnect);
		DetourAttach((LPVOID*)&v_CClient_ProcessStringCmd, &CClient::VProcessStringCmd);
		//DetourAttach((LPVOID*)&p_CClient_SendSnapshot, &CClient::VSendSnapshot);
	}
	virtual void Detach(void) const
	{
		DetourDetach((LPVOID*)&v_CClient_Clear, &CClient::VClear);
		DetourDetach((LPVOID*)&v_CClient_Connect, &CClient::VConnect);
		DetourDetach((LPVOID*)&v_CClient_ProcessStringCmd, &CClient::VProcessStringCmd);
		//DetourDetach((LPVOID*)&p_CClient_SendSnapshot, &CClient::VSendSnapshot);
	}
};
///////////////////////////////////////////////////////////////////////////////

