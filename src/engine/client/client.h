#pragma once
#include "vpc/keyvalues.h"
#include "common/protocol.h"
#include "engine/net.h"
#include "engine/net_chan.h"
#include "public/edict.h"
#include "engine/server/datablock_sender.h"

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

class CClient : IClientMessageHandler, INetChannelHandler
{
	friend class ServerDataBlockSender;
public:
	inline int64_t GetTeamNum() const { return m_iTeamNum; }
	inline edict_t GetHandle(void) const { return m_nHandle; }
	inline int GetUserID(void) const { return m_nUserID; }
	inline uint64_t GetNucleusID(void) const { return m_nNucleusID; }

	inline SIGNONSTATE GetSignonState(void) const { return m_nSignonState; }
	inline PERSISTENCE GetPersistenceState(void) const { return m_nPersistenceState; }
	inline CNetChan* GetNetChan(void) const { return m_NetChannel; }
	inline CServer* GetServer(void) const { return m_pServer; }

	inline int GetCommandTick(void) const { return m_nCommandTick; }
	inline const char* GetServerName(void) const { return m_szServerName; }
	inline const char* GetClientName(void) const { return m_szClientName; }

	inline void SetHandle(edict_t nHandle) { m_nHandle = nHandle; }
	inline void SetUserID(uint32_t nUserID) { m_nUserID = nUserID; }
	inline void SetNucleusID(uint64_t nNucleusID) { m_nNucleusID = nNucleusID; }

	inline void SetSignonState(SIGNONSTATE nSignonState) { m_nSignonState = nSignonState; }
	inline void SetPersistenceState(PERSISTENCE nPersistenceState) { m_nPersistenceState = nPersistenceState; }
	inline void SetNetChan(CNetChan* pNetChan) { m_NetChannel = pNetChan; }

	inline bool IsConnected(void) const { return m_nSignonState >= SIGNONSTATE::SIGNONSTATE_CONNECTED; }
	inline bool IsSpawned(void) const { return m_nSignonState >= SIGNONSTATE::SIGNONSTATE_NEW; }
	inline bool IsActive(void) const { return m_nSignonState == SIGNONSTATE::SIGNONSTATE_FULL; }
	inline bool IsPersistenceAvailable(void) const { return m_nPersistenceState >= PERSISTENCE::PERSISTENCE_AVAILABLE; }
	inline bool IsPersistenceReady(void) const { return m_nPersistenceState == PERSISTENCE::PERSISTENCE_READY; }
	inline bool IsFakeClient(void) const { return m_bFakePlayer; }
	inline bool IsHumanPlayer(void) const { if (!IsConnected() || IsFakeClient()) { return false; } return true; }

	bool SendNetMsgEx(CNetMessage* pMsg, bool bLocal, bool bForceReliable, bool bVoice);

	bool Authenticate(const char* const playerName, char* const reasonBuf, const size_t reasonBufLen);
	bool Connect(const char* szName, CNetChan* pNetChan, bool bFakePlayer,
		CUtlVector<NET_SetConVar::cvar_t>* conVars, char* szMessage, int nMessageSize);
	void Disconnect(const Reputation_t nRepLvl, const char* szReason, ...);
	void Clear(void);

public: // Hook statics:
	static void VClear(CClient* pClient);
	static bool VConnect(CClient* pClient, const char* szName, CNetChan* pNetChan, bool bFakePlayer,
		CUtlVector<NET_SetConVar::cvar_t>* conVars, char* szMessage, int nMessageSize);

	static void VActivatePlayer(CClient* pClient);
	static void* VSendSnapshot(CClient* pClient, CClientFrame* pFrame, int nTick, int nTickAck);
	static bool VSendNetMsgEx(CClient* pClient, CNetMessage* pMsg, bool bLocal, bool bForceReliable, bool bVoice);

	static bool VProcessStringCmd(CClient* pClient, NET_StringCmd* pMsg);
	static bool VProcessSetConVar(CClient* pClient, NET_SetConVar* pMsg);

private:
	// Stub reimplementation to avoid the 'no overrider' compiler errors in the
	// CServer class (contains a static array of MAX_PLAYERS of this class).
	virtual void* ConnectionStart(INetChannelHandler* chan) { return nullptr; }
	virtual void ConnectionClosing(const char* reason, int unk) {}
	virtual void ConnectionCrashed(const char* reason) {}
	virtual void PacketStart(int incoming_sequence, int outgoing_acknowledged) {}
	virtual void PacketEnd(void) {};
	virtual void FileRequested(const char* fileName, unsigned int transferID) {}
	virtual void ChannelDisconnect(const char* fileName) {}

	virtual void* ProcessStringCmd(void) { return nullptr; }
	virtual void* ProcessScriptMessage(void) { return nullptr; }
	virtual void* ProcessSetConVar(void) { return nullptr; }
	virtual bool nullsub_0(void) { return false; }
	virtual char ProcessSignonState(void* msg) { return false; } // NET_SignonState
	virtual void* ProcessMove(void) { return nullptr; }
	virtual void* ProcessVoiceData(void) { return nullptr; }
	virtual void* ProcessDurangoVoiceData(void) { return nullptr; }
	virtual bool nullsub_1(void) { return false; }
	virtual void* ProcessLoadingProgress(void) { return nullptr; }
	virtual void* ProcessPersistenceRequestSave(void) { return nullptr; }
	virtual bool nullsub_2(void) { return false; }
	virtual bool nullsub_3(void) { return false; }
	virtual void* ProcessSetPlaylistVarOverride(void) { return nullptr; }
	virtual void* ProcessClaimClientSidePickup(void) { return nullptr; }
	virtual void* ProcessCmdKeyValues(void) { return nullptr; }
	virtual void* ProcessClientTick(void) { return nullptr; }
	virtual void* ProcessClientSayText(void) { return nullptr; }
	virtual bool nullsub_4(void) { return false; }
	virtual bool nullsub_5(void) { return false; }
	virtual bool nullsub_6(void) { return false; }
	virtual void* ProcessScriptMessageChecksum(void) { return nullptr; }

private:
	uint32_t m_nUserID;
	edict_t m_nHandle;
	char m_szServerName[256];
	char m_szClientName[256];
	char m_szMachineName[256];
	int m_nCommandTick;
	bool m_bUsePersistence_MAYBE;
	char pad_0016[59];
	int64_t m_iTeamNum;
	KeyValues* m_ConVars;
	bool m_bConVarsChanged;
	bool m_bSendServerInfo;
	bool m_bSendSignonData;
	bool m_bFullStateAchieved;
	char pad_0368[4];
	CServer* m_pServer;
	char pad_0378[20];
	int m_nDisconnectTick;
	bool m_bKickedByFairFight_MAYBE;
	char pad_0398[3];
	int m_nSendtableCRC;
	int m_nMmDev;
	char pad_039C[4];
	CNetChan* m_NetChannel;
	char pad_03A8[8];
	SIGNONSTATE m_nSignonState;
	int unk0;
	uint64_t m_nNucleusID;
	int unk1;
	int unk2;
	int m_nDeltaTick;
	int m_nStringTableAckTick;
	int m_nSignonTick;
	int m_nBaselineUpdateTick_MAYBE;
	char pad_03C0[448];
#if defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	int unk3;
	int m_nForceWaitForTick;
#endif
	bool m_bFakePlayer;
	bool m_bReceivedPacket;
	bool m_bLowViolence;
	bool m_bFullyAuthenticated;
	char pad_05A4[24];
	PERSISTENCE m_nPersistenceState;
	char pad_05C0[48];
	ServerDataBlock m_DataBlock;
	char pad_4A3D8[60];
	int m_LastMovementTick;
#if defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	char pad_4A418[86];
#endif
	char pad_4A46E[80];
};
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
static_assert(sizeof(CClient) == 0x4A440);
#else
static_assert(sizeof(CClient) == 0x4A4C0);
#endif

/* ==== CBASECLIENT ===================================================================================================================================================== */
inline CMemory p_CClient_Connect;
inline bool(*v_CClient_Connect)(CClient* pClient, const char* szName, CNetChan* pNetChan, bool bFakePlayer, CUtlVector<NET_SetConVar::cvar_t>* conVars, char* szMessage, int nMessageSize);

inline CMemory p_CClient_Disconnect;
inline bool(*v_CClient_Disconnect)(CClient* pClient, const Reputation_t nRepLvl, const char* szReason, ...);

inline CMemory p_CClient_Clear;
inline void(*v_CClient_Clear)(CClient* pClient);

inline CMemory p_CClient_ActivatePlayer;
inline void(*v_CClient_ActivatePlayer)(CClient* pClient);

inline CMemory p_CClient_SetSignonState;
inline bool(*v_CClient_SetSignonState)(CClient* pClient, SIGNONSTATE signon);

inline CMemory p_CClient_SendNetMsgEx;
inline bool(*v_CClient_SendNetMsgEx)(CClient* pClient, CNetMessage* pMsg, bool bLocal, bool bForceReliable, bool bVoice);

inline CMemory p_CClient_SendSnapshot;
inline void*(*v_CClient_SendSnapshot)(CClient* pClient, CClientFrame* pFrame, int nTick, int nTickAck);

inline CMemory p_CClient_ProcessStringCmd;
inline bool(*v_CClient_ProcessStringCmd)(CClient* pClient, NET_StringCmd* pMsg);

inline CMemory p_CClient_ProcessSetConVar;
inline bool(*v_CClient_ProcessSetConVar)(CClient* pClient, NET_SetConVar* pMsg);

///////////////////////////////////////////////////////////////////////////////
class VClient : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CClient::Connect", p_CClient_Connect.GetPtr());
		LogFunAdr("CClient::Disconnect", p_CClient_Disconnect.GetPtr());
		LogFunAdr("CClient::Clear", p_CClient_Clear.GetPtr());
		LogFunAdr("CClient::ActivatePlayer", p_CClient_ActivatePlayer.GetPtr());
		LogFunAdr("CClient::SetSignonState", p_CClient_SetSignonState.GetPtr());
		LogFunAdr("CClient::SendNetMsgEx", p_CClient_SendNetMsgEx.GetPtr());
		LogFunAdr("CClient::SendSnapshot", p_CClient_SendSnapshot.GetPtr());
		LogFunAdr("CClient::ProcessStringCmd", p_CClient_ProcessStringCmd.GetPtr());
		LogFunAdr("CClient::ProcessSetConVar", p_CClient_ProcessSetConVar.GetPtr());
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
		p_CClient_ActivatePlayer = g_GameDll.FindPatternSIMD("40 53 57 41 57 48 83 EC 30 8B 81 ?? ?? ?? ??");
		p_CClient_SendNetMsg = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 41 56 41 57 48 83 EC 30 48 8B 05 ?? ?? ?? ?? 45 0F B6 F1");
		p_CClient_SendSnapshot = g_GameDll.FindPatternSIMD("44 89 44 24 ?? 48 89 4C 24 ?? 55 53 56 57 41 55");
		p_CClient_ProcessStringCmd = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 55 48 81 EC ?? ?? ?? ?? 49 8B D8");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CClient_ActivatePlayer = g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 8B 81 B0 03 ?? ?? 48 8B D9 C6");
		p_CClient_SendNetMsgEx = g_GameDll.FindPatternSIMD("40 53 55 56 57 41 56 48 83 EC 40 48 8B 05 ?? ?? ?? ??");
		p_CClient_SendSnapshot = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 55 56 41 55 41 56 41 57 48 8D 6C 24 ??");
		p_CClient_ProcessStringCmd = g_GameDll.FindPatternSIMD("48 89 6C 24 ?? 57 48 81 EC ?? ?? ?? ?? 48 8B 7A 20");
#endif // !GAMEDLL_S0 || !GAMEDLL_S1

		p_CClient_ProcessSetConVar = g_GameDll.FindPatternSIMD("48 83 EC 28 48 83 C2 20");
		p_CClient_SetSignonState = g_GameDll.FindPatternSIMD("48 8B C4 48 89 58 10 48 89 70 18 57 48 81 EC ?? ?? ?? ?? 0F 29 70 E8 8B F2");

		v_CClient_Connect    = p_CClient_Connect.RCast<bool (*)(CClient*, const char*, CNetChan*, bool, CUtlVector<NET_SetConVar::cvar_t>*, char*, int)>();
		v_CClient_Disconnect = p_CClient_Disconnect.RCast<bool (*)(CClient*, const Reputation_t, const char*, ...)>();
		v_CClient_Clear      = p_CClient_Clear.RCast<void (*)(CClient*)>();
		v_CClient_ActivatePlayer = p_CClient_ActivatePlayer.RCast<void (*)(CClient* pClient)>();
		v_CClient_SetSignonState = p_CClient_SetSignonState.RCast<bool (*)(CClient*, SIGNONSTATE)>();
		v_CClient_SendNetMsgEx = p_CClient_SendNetMsgEx.RCast<bool (*)(CClient*, CNetMessage*, bool, bool, bool)>();
		v_CClient_SendSnapshot = p_CClient_SendSnapshot.RCast<void* (*)(CClient*, CClientFrame*, int, int)>();

		v_CClient_ProcessStringCmd = p_CClient_ProcessStringCmd.RCast<bool (*)(CClient*, NET_StringCmd*)>();
		v_CClient_ProcessSetConVar = p_CClient_ProcessSetConVar.RCast<bool (*)(CClient*, NET_SetConVar*)>();
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
