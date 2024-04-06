#ifndef INETCHANNEL_H
#define INETCHANNEL_H

#define NET_FRAMES_BACKUP 128
#define FLOW_OUTGOING     0
#define FLOW_INCOMING     1
#define MAX_FLOWS         2 // in & out
#include "tier1/NetAdr.h"

class IClientMessageHandler
{
public:
	virtual ~IClientMessageHandler(void) {};
	virtual void* ProcessStringCmd(void) = 0;
	virtual void* ProcessScriptMessage(void) = 0;
	virtual void* ProcessSetConVar(void) = 0;
	virtual bool nullsub_0(void) = 0;
	virtual char ProcessSignonState(void* msg) = 0; // NET_SignonState
	virtual void* ProcessMove(void) = 0;
	virtual void* ProcessVoiceData(void) = 0;
	virtual void* ProcessDurangoVoiceData(void) = 0;
	virtual bool nullsub_1(void) = 0;
	virtual void* ProcessLoadingProgress(void) = 0;
	virtual void* ProcessPersistenceRequestSave(void) = 0;
	virtual bool nullsub_2(void) = 0;
	virtual bool nullsub_3(void) = 0;
	virtual void* ProcessSetPlaylistVarOverride(void) = 0;
	virtual void* ProcessClaimClientSidePickup(void) = 0;
	virtual void* ProcessCmdKeyValues(void) = 0;
	virtual void* ProcessClientTick(void) = 0;
	virtual void* ProcessClientSayText(void) = 0;
	virtual bool nullsub_4(void) = 0;
	virtual bool nullsub_5(void) = 0;
	virtual bool nullsub_6(void) = 0;
	virtual void* ProcessScriptMessageChecksum(void) = 0;
};

class INetChannelHandler
{
public:
	virtual ~INetChannelHandler(void) {};
	virtual void*ConnectionStart(INetChannelHandler* chan) = 0;
	virtual void ConnectionClosing(const char* reason, int unk) = 0;
	virtual void ConnectionCrashed(const char* reason) = 0;
	virtual void PacketStart(int incoming_sequence, int outgoing_acknowledged) = 0;
	virtual void PacketEnd(void) = 0;
	virtual void FileRequested(const char* fileName, unsigned int transferID) = 0;
	virtual void ChannelDisconnect(const char* fileName) = 0;
};

struct CS_INetChannelHandler : INetChannelHandler
{};

enum netsocket_e
{
	NS_CLIENT = 0,	// client socket
	NS_SERVER,		// server socket

	// unknown as this seems unused in R5, but if the socket equals to this in
	// CServer::ConnectionlessPacketHandler() in case C2S_Challenge, the packet
	// sent back won't be encrypted
	NS_UNK0,

	// used for chat room, communities, discord presence, EA/Origin, etc
	NS_PRESENCE,

	MAX_SOCKETS // 4 in R5
};

typedef struct netpacket_s
{
	netadr_t from;
	int source;
	double received;
	uint8_t* pData;
	bf_read message;
	int size;
	int wiresize;
	char stream;
	netpacket_s* pNext;
} netpacket_t;
static_assert(sizeof(netpacket_t) == 0x88);

typedef struct nettick_s
{
	int m_nServerTick;
	int m_nClientTick;
	float m_flHostFrameTime;
	float m_flHostFrameTimeStdDeviation;
	bool m_bStruggling;
	uint8_t m_nServerCPU;
	int m_nCommandTick;
} nettick_t;
static_assert(sizeof(nettick_s) == 0x18);

#endif // !INETCHANNEL_H
