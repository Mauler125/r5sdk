#ifndef INETCHANNEL_H
#define INETCHANNEL_H

#define NET_FRAMES_BACKUP 128
#define FLOW_OUTGOING     0
#define FLOW_INCOMING     1
#define MAX_FLOWS         2 // in & out

struct INetChannelHandler
{
	void* iNetMessageHandlerVTable /*VFT*/;
};
#endif // !INETCHANNEL_H
