#ifndef INETCHANNEL_H
#define INETCHANNEL_H

#define NET_FRAMES_BACKUP 128
#define FLOW_OUTGOING     0
#define FLOW_INCOMING     1
#define MAX_FLOWS         2 // in & out

struct IClientMessageHandler
{
	void* __vftable /*VFT*/;
};

struct INetChannelHandler
{
	void* __vftable /*VFT*/;
};

struct CS_INetChannelHandler : INetChannelHandler
{};

typedef struct netpacket_s netpacket_t;
typedef struct __declspec(align(8)) netpacket_s
{
	v_netadr_t from;
	int source;
	double received;
	uint8_t* pData;
	bf_read message;
	int size;
	int wiresize;
	char stream;
	netpacket_s* pNext;
} netpacket_t;
#endif // !INETCHANNEL_H
