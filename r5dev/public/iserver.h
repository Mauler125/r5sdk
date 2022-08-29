#ifndef ISERVER_H
#define ISERVER_H
#include "inetchannel.h"
#include "inetmsghandler.h"

abstract_class IServer : public IConnectionlessPacketHandler
{
public:
	virtual ~IServer(void) = 0;
	virtual bool ConnectionlessPacketHandler(netpacket_t* pInPacket) = 0;
};

#endif // ISERVER_H