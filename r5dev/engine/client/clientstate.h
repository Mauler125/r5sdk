#pragma once
#include "tier1/NetAdr.h"
#include "tier1/mempool.h"
#include "common/protocol.h"
#include "public/inetmsghandler.h"
#include "public/isnapshotmgr.h"
#include "engine/net_chan.h"
#include "engine/networkstringtable.h"
#include "engine/debugoverlay.h"
#include "engine/clockdriftmgr.h"
#include "engine/framesnapshot.h"
#include "engine/packed_entity.h"
#include "datablock_receiver.h"

struct connectparams_t
{
	const char* netAdr;
	const char* netKey;
	int unkReconnect;
	int unk;
	bool challengeRequest;
	bool asSpectator_MAYBE;
};

class CClientSnapshotManager : public IClientSnapshotManager
{
public:
	virtual ~CClientSnapshotManager(void){};

	void* m_Frames;
	CUtlMemoryPool m_ClientFramePool;
};

///////////////////////////////////////////////////////////////////////////////
class CClientState : CS_INetChannelHandler, IConnectionlessPacketHandler, IServerMessageHandler, CClientSnapshotManager
{
	friend class ClientDataBlockReceiver;
public: // Hook statics.
	static void VConnectionClosing(CClientState* thisptr, const char* szReason);
	static bool _ProcessStringCmd(CClientState* thisptr, NET_StringCmd* msg);
	static bool VProcessServerTick(CClientState* thisptr, SVC_ServerTick* msg);
	static bool _ProcessCreateStringTable(CClientState* thisptr, SVC_CreateStringTable* msg);
	static bool _ProcessUserMessage(CClientState* thisptr, SVC_UserMessage* msg);
	static void VConnect(CClientState* thisptr, connectparams_t* connectParams);


public:
	bool IsPaused() const;
	bool IsActive(void) const;
	bool IsConnected(void) const;
	bool IsConnecting(void) const;

	float GetClientTime() const;

	int GetTick() const;

	int GetServerTickCount() const;	// Get the server tick count.
	void SetServerTickCount(int tick);

	int GetClientTickCount() const;	// Get the client tick count.
	void SetClientTickCount(int tick);

	float GetFrameTime(void) const;

	bool Authenticate(connectparams_t* connectParams, char* const reasonBuf, const size_t reasonBufLen) const;

protected:
	FORCEINLINE CClientState* GetShiftedBasePointer(void)
	{
		// NOTE: you must check in the disassembler if the CClientState method
		// you detour is shifting the 'this' pointer with 16 bytes forward, if
		// so, you need to call this and use this pointer instead! The shifting
		// happens as part of a compiler optimization that truncated the vtable
		// pointers off so the 'this' pointer points directly to the class data
		char* const pShifted = reinterpret_cast<char*>(this) - 0x10;
		return reinterpret_cast<CClientState*>(pShifted);
	}

public:
	int m_Socket;
	int _padding_maybe;
	CNetChan* m_NetChannel;
	double m_flConnectTime;
	_DWORD m_nRetryNumber;
	_DWORD m_nChallengeRetryLimit;
	_BYTE encrypted_connection_MAYBE;
	_BYTE gap79[3];
	netadr_t addr;
	bool m_bUnk_used_during_auth;
	char m_bSendChallengeRequest;
	_BYTE m_bDoNetParamsReconnect_MAYBE;
	_BYTE field_97;
	SIGNONSTATE m_nSignonState;
	_BYTE gap9C[4];
	double m_flNextCmdTime;
	int m_nServerCount;
	int m_nInSequenceNr;
	float m_flClockDriftFrameTime;
	CClockDriftMgr m_ClockDriftMgr;
	_BYTE field_148;
	_BYTE field_149;
	int m_nDeltaTick;
	int m_nStringTableAckTick;
	int m_nProcessedDeltaTick;
	int m_nProcessedStringTableAckTick;
	bool m_bPendingTicksAvailable;
	_BYTE field_15D;
	_BYTE field_15E;
	_BYTE field_15F;
	_BYTE m_bPaused;
	char field_161;
	char field_162;
	_DWORD dword164;
	int m_nViewEntity_MAYBE_ClientSlot_Plus_One;
	int m_nPlayerSlot;
	char m_szLevelFileName[64];
	char m_szLevelBaseName[64];
	char m_szLastLevelBaseName[64];
	char m_szSkyBoxBaseName[64];
	char m_szServerAddresString[128];
	int m_bInMpLobbyMenu;
	int m_nTeam;
	_DWORD m_nMaxClients;
	_DWORD field_2FC;
	_DWORD reconnect_unk;
	float m_flTickTime;
	float m_flOldTickTime;
	_BYTE m_bSignonChallengeReceived;
	_DWORD challenge;
	netadr_t challengeAddr;
	bool m_bUseLocalSendTableFile;
	_QWORD m_pServerClasses;
	int m_nServerClasses;
	int m_nServerClassBits;
	CNetworkStringTableContainer* m_StringTableContainer;
	char m_PersistenceData[98304];
	_BYTE m_bPersistenceBaselineRecvd;
	int m_nPersistenceBaselineEntries;
	int field_18350;
	bool m_bRestrictServerCommands;
	bool m_bRestrictClientCommands;
	char buffer_0x400[1024];
	ClientDataBlockReceiver m_DataBlockReceiver;
	char client_requested_disconnect;
	char error_message[512];
	_BYTE gap18CA1[3];
	_DWORD last_usercmd_time_from_server_maybe;
	CFrameSnapshot* m_PrevFrameSnapshot;
	_QWORD qword18CB0;
	CFrameSnapshot* m_CurrFrameSnapshot;
	_BYTE gap18CC0[8];
	char m_bClockCorrectionEnabled;
	char m_b_unknown;
	bool m_bLocalPredictionInitialized_MAYBE;
	int m_nServerTick;
	int dword18CD0;
	int field_18CD4;
	float m_flFrameTime;
	int m_nOutgoingCommandNr;
	int current_movement_sequence_number;
	char gap18CE4[4];
	__int64 qword18CE8;
	int field_18CF0;
	int hit_prespawn;
	int field_18CF8;
	int dword18CFC;
	float m_flClockDriftUnknown_rounded;
	char something_with_prediction;
	char field_18D05;
	char gap18D06[2];
	int dword18D08;
	char gap18D0C[13];
	char do_local_prediction_update;
	char gap18D1A[2];
	int dword18D1C;
	__int64 qword18D20;
	int dword18D28;
	int dword18D2C;
	Vector3D field_18D30;
	int dword18D3C;
	int dword18D40;
	char gap18D44[4];
	__int64 qword18D48;
	__int64 qword18D50;
	__int64 qword18D58;
	int dword18D60;
	char gap18D64[4];
	__int64 qword18D68;
	char buffer_47128[47128];
	char entitlements_bitfield[16];
	__int64 maybe_some_ll_stuff;
	__int64 qword245A8;
	__int64 qword245B0;
	__int64 qword245B8;
	__int64 qword245C0;
	__int64 qword245C8;
	__int64 qword245D0;
	__int64 qword245D8;
	__int64 qword245E0;
	__int64 qword245E8;
	__int64 qword245F0;
	int dword245F8;
	char gap245FC[1024];
	int dword249EC;
	__int64 m_pModelPrecacheTable;
	__int64 qword24A10;
	__int64 m_pInstanceBaselineTable;
	__int64 m_pLightStyleTable;
	__int64 m_pUserInfoTable;
	__int64 m_pServerStartupTable;
	PackedEntity m_pEntityBaselines_maybe[4096];
	char byte34A38;
	char field_34A39[7];
};
static_assert(sizeof(CClientState) == 0x34A20);

#ifndef DEDICATED
extern CClientState* g_pClientState;
extern CClientState** g_pClientState_Shifted; // Shifted by 0x10 forward!
#endif // DEDICATED

/* ==== CCLIENTSTATE ==================================================================================================================================================== */
inline void(*CClientState__RunFrame)(CClientState* thisptr);
inline void(*CClientState__Connect)(CClientState* thisptr, connectparams_t* connectParams);
inline void(*CClientState__Disconnect)(CClientState* thisptr, bool bSendTrackingContext);
inline void(*CClientState__ConnectionClosing)(CClientState* thisptr, const char* szReason);
inline bool(*CClientState__HookClientStringTable)(CClientState* thisptr, const char* tableName);

inline bool(*CClientState__ProcessStringCmd)(CClientState* thisptr, NET_StringCmd* msg);
inline bool(*CClientState__ProcessServerTick)(CClientState* thisptr, SVC_ServerTick* msg);
inline bool(*CClientState__ProcessCreateStringTable)(CClientState* thisptr, SVC_CreateStringTable* msg);
inline bool(*CClientState__ProcessUserMessage)(CClientState* thisptr, SVC_UserMessage* msg);

///////////////////////////////////////////////////////////////////////////////
class VClientState : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CClientState::RunFrame", CClientState__RunFrame);
		LogFunAdr("CClientState::Connect", CClientState__Connect);
		LogFunAdr("CClientState::Disconnect", CClientState__Disconnect);
		LogFunAdr("CClientState::ConnectionClosing", CClientState__ConnectionClosing);
		LogFunAdr("CClientState::HookClientStringTable", CClientState__HookClientStringTable);
		LogFunAdr("CClientState::ProcessStringCmd", CClientState__ProcessStringCmd);
		LogFunAdr("CClientState::ProcessServerTick", CClientState__ProcessServerTick);
		LogFunAdr("CClientState::ProcessCreateStringTable", CClientState__ProcessCreateStringTable);
		LogFunAdr("CClientState::ProcessUserMessage", CClientState__ProcessUserMessage);
		LogVarAdr("g_ClientState", g_pClientState);
		LogVarAdr("g_ClientState_Shifted", g_pClientState_Shifted);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("40 53 48 81 EC ?? ?? ?? ?? 83 B9 ?? ?? ?? ?? ?? 48 8B D9 7D 0B").GetPtr(CClientState__RunFrame);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 56 48 81 EC ?? ?? ?? ?? 48 8B 32").GetPtr(CClientState__Connect);
		g_GameDll.FindPatternSIMD("40 56 57 41 54 41 55 41 57 48 83 EC 30 44 0F B6 FA").GetPtr(CClientState__Disconnect);
		g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 83 B9 ?? ?? ?? ?? ?? 48 8B DA 0F 8E ?? ?? ?? ??").GetPtr(CClientState__ConnectionClosing);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 57 48 83 EC 20 48 8B D9 48 8B FA 48 8B 89 ?? ?? ?? ?? 48 85 C9 0F 84 ?? ?? ?? ??").GetPtr(CClientState__HookClientStringTable);
		g_GameDll.FindPatternSIMD("40 53 48 81 EC ?? ?? ?? ?? 80 B9 ?? ?? ?? ?? ?? 48 8B DA").GetPtr(CClientState__ProcessStringCmd);
		g_GameDll.FindPatternSIMD("40 57 48 83 EC 20 83 B9 ?? ?? ?? ?? ?? 48 8B F9 7C 66").GetPtr(CClientState__ProcessServerTick);
		g_GameDll.FindPatternSIMD("48 89 4C 24 ?? 53 56 48 81 EC ?? ?? ?? ?? 83 B9 ?? ?? ?? ?? ??").GetPtr(CClientState__ProcessCreateStringTable);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 55 48 8D AC 24 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 83 B9 ?? ?? ?? ?? ??").GetPtr(CClientState__ProcessUserMessage);
	}
	virtual void GetVar(void) const
	{
		g_pClientState = g_GameDll.FindPatternSIMD("0F 84 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? 48 83 C4 28").FindPatternSelf("48 8D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<CClientState*>(); /*0F 84 ? ? ? ? 48 8D 0D ? ? ? ? 48 83 C4 28*/
		g_pClientState_Shifted = reinterpret_cast<CClientState**>(reinterpret_cast<int64_t*>(g_pClientState)+1); // Shift by 8 bytes.
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
