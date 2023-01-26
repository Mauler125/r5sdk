#pragma once
#include "tier1/NetAdr2.h"
#include "tier1/mempool.h"
#include "common/protocol.h"
#include "public/inetmsghandler.h"
#include "public/isnapshotmgr.h"
#include "engine/datablock.h"
#include "engine/net_chan.h"
#include "engine/debugoverlay.h"
#include "engine/clockdriftmgr.h"
#include "engine/framesnapshot.h"
#include "engine/packed_entity.h"

class CClientSnapshotManager : public IClientSnapshotManager
{
public:
	virtual ~CClientSnapshotManager(void){};

	void* m_Frames;
	CUtlMemoryPool m_ClientFramePool;
};
#pragma pack(push, 4)
///////////////////////////////////////////////////////////////////////////////
class CClientState : CS_INetChannelHandler, IConnectionlessPacketHandler, IServerMessageHandler, CClientSnapshotManager
{
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

	int m_Socket;
	int _padding_maybe;
	CNetChan* m_NetChannel;
	double m_flConnectTime;
	_DWORD m_nRetryNumber;
	_DWORD m_nChallengeRetryLimit;
	_BYTE encrypted_connection_MAYBE;
	_BYTE gap79[3];
	v_netadr_t addr;
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
	int m_nProcesseedDeltaTick;
	int m_nProcessedStringTableAckTick;
	bool m_bPendingTicksAvailable;
	__declspec(align(4)) _BYTE m_bPaused;
	char field_161;
	char field_162;
	_DWORD dword164;
	int m_nViewEntity_MAYBE_ClientSlot_Plus_One;
	int m_nPlayerSlot;
	char m_szLevelFileName[64];
	char m_szLevelBaseName[64];
	char field_1F0[64];
	char field_230[64];
	_BYTE m_szServerAddresString[128];
	int m_bInMpLobbyMenu;
	int m_nTeam;
	_DWORD m_nMaxClients;
	__declspec(align(8)) _DWORD reconnect_unk;
	float m_flTickTime;
	float m_flOldTickTime;
	_BYTE m_bSignonChallengeReceived;
	_DWORD challenge;
	v_netadr_t challengeAddr;
	_BYTE byte33C;
	_QWORD m_pServerClasses;
	int m_nServerClasses;
	int m_nServerClassBits;
	__int64 m_StringTableContainer;
	char m_PersistenceData[98304];
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	char pads0[8];
#endif
	_BYTE m_bPersistenceBaselineRecvd;
	__unaligned __declspec(align(4)) _QWORD m_nPersistenceBaselineEntries;
	bool m_bRestrictServerCommands;
	bool m_bRestrictClientCommands;
	char buffer_0x400[1024];
	NetDataBlockReceiver blockReceiver;
	char client_requested_disconnect;
	char error_message[512];
	_BYTE gap18CA1[3];
	_DWORD last_usercmd_time_from_server_maybe;
	CFrameSnapshot* prev_frame_maybe;
	_QWORD qword18CB0;
	CFrameSnapshot* current_frame_maybe;
	_BYTE gap18CC0[8];
	char m_bClockCorrectionEnabled;
	char m_b_unknown;
	bool m_bLocalPredictionInitialized_MAYBE;
	int m_nServerTick;
	int dword18CD0;
	int field_18CD4;
	float m_flFrameTime;
	int outgoing_command;
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
	float field_18D30;
	float m_flUnk1;
	float m_flUnk2;
	int dword18D3C;
	int dword18D40;
	char gap18D44[4];
	__int64 qword18D48;
	__int64 qword18D50;
	__int64 qword18D58;
	int dword18D60;
	char gap18D64[4];
	__int64 qword18D68;
	char gap18D70[8];
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
	__unaligned __declspec(align(1)) __int64 qword249FC;
	char gap24A04[4];
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
#pragma pack(pop)
#ifndef DEDICATED
extern CClientState* g_pClientState;
extern CClientState** g_pClientState_Shifted; // Shifted by 0x10 forward!
#endif // DEDICATED

/* ==== CCLIENTSTATE ==================================================================================================================================================== */
inline CMemory p_CClientState__RunFrame;
inline auto CClientState__RunFrame = p_CClientState__RunFrame.RCast<void(*)(CClientState* thisptr)>();

inline CMemory p_CClientState__Disconnect; /*48 89 5C 24 ?? 55 57 41 56 48 83 EC 30 0F B6 EA*/
inline auto CClientState__Disconnect = p_CClientState__Disconnect.RCast<void(*)(CClientState* thisptr, bool bSendTrackingContext)>();


///////////////////////////////////////////////////////////////////////////////
class VClientState : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CClientState::RunFrame", p_CClientState__RunFrame.GetPtr());
		LogFunAdr("CClientState::Disconnect", p_CClientState__Disconnect.GetPtr());
		LogVarAdr("g_pClientState", reinterpret_cast<uintptr_t>(g_pClientState));
		LogVarAdr("g_pClientState_Shifted", reinterpret_cast<uintptr_t>(g_pClientState_Shifted));
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CClientState__RunFrame = g_GameDll.FindPatternSIMD("48 89 4C 24 ?? 57 48 81 EC ?? ?? ?? ?? 83 B9 ?? ?? ?? ?? ??");
		CClientState__RunFrame = p_CClientState__RunFrame.RCast<void(*)(CClientState* thisptr)>();

		p_CClientState__Disconnect = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 55 57 41 56 48 83 EC 30 0F B6 EA");
		CClientState__Disconnect = p_CClientState__Disconnect.RCast<void(*)(CClientState* thisptr, bool bSendTrackingContext)>(); /*48 89 5C 24 ?? 55 57 41 56 48 83 EC 30 0F B6 EA*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CClientState__RunFrame = g_GameDll.FindPatternSIMD("40 53 48 81 EC ?? ?? ?? ?? 83 B9 ?? ?? ?? ?? ?? 48 8B D9 7D 0B");
		CClientState__RunFrame = p_CClientState__RunFrame.RCast<void(*)(CClientState* thisptr)>();

		p_CClientState__Disconnect = g_GameDll.FindPatternSIMD("40 56 57 41 54 41 55 41 57 48 83 EC 30 44 0F B6 FA");
		CClientState__Disconnect = p_CClientState__Disconnect.RCast<void(*)(CClientState* thisptr, bool bSendTrackingContext)>(); /*40 56 57 41 54 41 55 41 57 48 83 EC 30 44 0F B6 FA*/
#endif
	}
	virtual void GetVar(void) const
	{
		g_pClientState = g_GameDll.FindPatternSIMD("0F 84 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? 48 83 C4 28").FindPatternSelf("48 8D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<CClientState*>(); /*0F 84 ? ? ? ? 48 8D 0D ? ? ? ? 48 83 C4 28*/
		g_pClientState_Shifted = g_GameDll.FindPatternSIMD("80 3D ?? ?? ?? ?? ?? 74 14 66 0F 6E 05 ?? ?? ?? ??").ResolveRelativeAddress(0x2, 0x7).RCast<CClientState**>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////
