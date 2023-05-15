//=============================================================================//
// 
// Purpose: 
// 
//=============================================================================//

#ifndef NET_CHAN_H
#define NET_CHAN_H

#include "tier1/bitbuf.h"
#include "tier1/NetAdr.h"
#include "tier1/NetKey.h"
#include "tier1/utlmemory.h"
#include "tier1/utlvector.h"
#include "common/netmessages.h"
#include "common/protocol.h"
#include "public/inetchannel.h"

#define NET_FRAMES_BACKUP 128
#define NET_CHANNELNAME_MAXLEN 32
#define NET_FRAMES_MASK   (NET_FRAMES_BACKUP-1)

//-----------------------------------------------------------------------------
// Purpose: forward declarations
//-----------------------------------------------------------------------------
class CClient;

//-----------------------------------------------------------------------------
struct netframe_t
{
	float one;
	float two;
	float three;
	float four;
	float five;
	float six;
};

//-----------------------------------------------------------------------------
struct netflow_t
{
	float nextcompute;
	float avgbytespersec;
	float avgpacketspersec;
	float avgloss;
	float avgchoke;
	float avglatency;
	float latency;
	int64_t totalpackets;
	int64_t totalbytes;
	netframe_t frames[NET_FRAMES_BACKUP];
	netframe_t current_frame;
};

//-----------------------------------------------------------------------------
struct dataFragments_t
{
	char* data;
	int64_t block_size;
	bool m_bIsCompressed;
	uint8_t gap11[7];
	int64_t m_nRawSize;
	bool m_bFirstFragment;
	bool m_bLastFragment;
	bool m_bIsOutbound;
	int transferID;
	int m_nTransferSize;
	int m_nCurrentOffset;
};

//-----------------------------------------------------------------------------
enum EBufType
{
	BUF_RELIABLE = 0,
	BUF_UNRELIABLE,
	BUF_VOICE
};

//-----------------------------------------------------------------------------
class CNetChan
{
public:
	const char* GetName(void) const;
	const char* GetAddress(bool onlyBase = false) const;
	int         GetPort(void) const;
	int         GetDataRate(void) const;
	int         GetBufferSize(void) const;

	float       GetLatency(int flow) const;
	float       GetAvgChoke(int flow) const;
	float       GetAvgLatency(int flow) const;
	float       GetAvgLoss(int flow) const;
	float       GetAvgPackets(int flow) const;
	float       GetAvgData(int flow) const;

	int64_t     GetTotalData(int flow) const;
	int64_t     GetTotalPackets(int flow) const;
	int         GetSequenceNr(int flow) const;

	float       GetTimeoutSeconds(void) const;
	double      GetTimeConnected(void) const;
	int         GetSocket(void) const;

	const bf_write& GetStreamVoice(void) const;
	const netadr_t& GetRemoteAddress(void) const;

	bool        IsOverflowed(void) const;

	void Clear(bool bStopProcessing);
	inline void Shutdown(const char* szReason, uint8_t bBadRep, bool bRemoveNow)
	{ _Shutdown(this, szReason, bBadRep, bRemoveNow); }

	static void _Shutdown(CNetChan* pChan, const char* szReason, uint8_t bBadRep, bool bRemoveNow);
	static bool _ProcessMessages(CNetChan* pChan, bf_read* pMsg);

	void SetRemoteFramerate(float flFrameTime, float flFrameTimeStdDeviation);
	void SetRemoteCPUStatistics(uint8_t nStats);
	//-----------------------------------------------------------------------------
private:
	bool                m_bProcessingMessages;
	bool                m_bShouldDelete;
	bool                m_bStopProcessing;
	bool                shutting_down;
	int                 m_nOutSequenceNr;
	int                 m_nInSequenceNr;
	int                 m_nOutSequenceNrAck;
	int                 m_nChokedPackets;
	int                 unknown_challenge_var;
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1) || defined (GAMEDLL_S2)
	char                pad[8];
#endif
	int                 m_nLastRecvFlags;
	RTL_SRWLOCK         LOCK;
	bf_write            m_StreamReliable;
	CUtlMemory<byte>    m_ReliableDataBuffer;
	bf_write            m_StreamUnreliable;
	CUtlMemory<byte>    m_UnreliableDataBuffer;
	bf_write            m_StreamVoice;
	CUtlMemory<byte>    m_VoiceDataBuffer;
	int                 m_Socket;
	int                 m_MaxReliablePayloadSize;
	double              last_received;
	double              connect_time;
	uint32_t            m_Rate;
	int                 padding_maybe;
	double              m_fClearTime;
	CUtlVector<dataFragments_t*> m_WaitingList;
	dataFragments_t     m_ReceiveList;
	int                 m_nSubOutFragmentsAck;
	int                 m_nSubInFragments;
	int                 m_nNonceHost;
	uint32_t            m_nNonceRemote;
	bool                m_bReceivedRemoteNonce;
	bool                m_bInReliableState;
	bool                m_bPendingRemoteNonceAck;
	uint32_t            m_nSubOutSequenceNr;
	int                 m_nLastRecvNonce;
	bool                m_bUseCompression;
	uint32_t            dword168;
	float               m_Timeout;
	INetChannelHandler* m_MessageHandler;
	CUtlVector<INetMessage*> m_NetMessages;
	uint64_t            qword198;
	int                 m_nQueuedPackets;
	float               m_flRemoteFrameTime;
	float               m_flRemoteFrameTimeStdDeviation;
	uint8_t             m_nServerCPU;
	int                 m_nMaxRoutablePayloadSize;
	int                 m_nSplitPacketSequence;
	int64_t             m_StreamSendBuffer;
	bf_write            m_StreamSend;
	uint8_t             m_bInMatch_maybe;
	netflow_t           m_DataFlow[2];
	int                 m_nLifetimePacketsDropped;
	int                 m_nSessionPacketsDropped;
	int                 m_nSequencesSkipped_MAYBE;
	int                 m_nSessionRecvs;
	uint32_t            m_nLiftimeRecvs;
	bool                m_bPad;
	char                m_Name[NET_CHANNELNAME_MAXLEN];
	netadr_t            remote_address;
};
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1) || defined (GAMEDLL_S2)
static_assert(sizeof(CNetChan) == 0x1AD0);
#else
static_assert(sizeof(CNetChan) == 0x1AC8);
#endif

inline CMemory p_NetChan_Clear;
inline auto v_NetChan_Clear = p_NetChan_Clear.RCast<void (*)(CNetChan* pChannel, bool bStopProcessing)>();

inline CMemory p_NetChan_Shutdown;
inline auto v_NetChan_Shutdown = p_NetChan_Shutdown.RCast<void (*)(void* thisptr, const char* szReason, uint8_t bBadRep, bool bRemoveNow)>();

inline CMemory p_NetChan_ProcessMessages;
inline auto v_NetChan_ProcessMessages = p_NetChan_ProcessMessages.RCast<bool (*)(CNetChan* pChan, bf_read* pMsg)>();
///////////////////////////////////////////////////////////////////////////////
class VNetChan : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CNetChan::Clear", p_NetChan_Clear.GetPtr());
		LogFunAdr("CNetChan::Shutdown", p_NetChan_Shutdown.GetPtr());
		LogFunAdr("CNetChan::ProcessMessages", p_NetChan_ProcessMessages.GetPtr());
	}
	virtual void GetFun(void) const
	{
		p_NetChan_Clear = g_GameDll.FindPatternSIMD("88 54 24 10 53 55 57");
		v_NetChan_Clear = p_NetChan_Clear.RCast<void (*)(CNetChan*, bool)>();

		p_NetChan_Shutdown = g_GameDll.FindPatternSIMD("48 89 6C 24 18 56 57 41 56 48 83 EC 30 83 B9");
		v_NetChan_Shutdown = p_NetChan_Shutdown.RCast<void (*)(void*, const char*, uint8_t, bool)>();

		p_NetChan_ProcessMessages = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 57 48 81 EC ?? ?? ?? ?? 48 8B FA");
		v_NetChan_ProcessMessages = p_NetChan_ProcessMessages.RCast<bool (*)(CNetChan*, bf_read*)>();
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const;
	virtual void Detach(void) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // NET_CHAN_H
