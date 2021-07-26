#include "pch.h"
#include "cnetchan.h"

//-----------------------------------------------------------------------------
// Hook and log the receive datagram
//-----------------------------------------------------------------------------
bool HNET_ReceiveDatagram(int sock, void* inpacket, bool raw)
{
	bool result = org_NET_ReceiveDatagram(sock, inpacket, raw);
	if (result)
	{
		int i = NULL;
		netpacket_t* pkt = (netpacket_t*)inpacket;

		// Log received packet data
		HexDump("[+] NET_ReceiveDatagram", 0, &pkt->data[i], pkt->wiresize);
	}
	return result;
}

//-----------------------------------------------------------------------------
// Hook and log send datagram
//-----------------------------------------------------------------------------
unsigned int HNET_SendDatagram(SOCKET s, const char* buf, int len, int flags)
{
	unsigned int result = org_NET_SendDatagram(s, buf, len, flags);
	if (result)
	{
		// Log transmitted packet data
		HexDump("[+] NET_SendDatagram", 0, buf, len);
	}
	return result;
}

///////////////////////////////////////////////////////////////////////////////
void AttachCNetChanHooks()
{
	DetourAttach((LPVOID*)&org_NET_ReceiveDatagram, &HNET_ReceiveDatagram);
	DetourAttach((LPVOID*)&org_NET_SendDatagram, &HNET_SendDatagram);
}

void DetachCNetChanHooks()
{
	DetourDetach((LPVOID*)&org_NET_ReceiveDatagram, &HNET_ReceiveDatagram);
	DetourDetach((LPVOID*)&org_NET_SendDatagram, &HNET_SendDatagram);
}