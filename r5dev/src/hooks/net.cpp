#include "pch.h"
#include "hooks.h"

namespace Hooks
{
	NET_ReceiveDatagramFn originalNET_ReceiveDatagram = nullptr;
	NET_SendDatagramFn originalNET_SendDatagram = nullptr;
}

bool Hooks::NET_ReceiveDatagram(int sock, void* inpacket, bool raw)
{
	bool result = originalNET_ReceiveDatagram(sock, inpacket, raw);
	if (result)
	{
		int i = NULL;
		netpacket_t* pkt = (netpacket_t*)inpacket;

		///////////////////////////////////////////////////////////////////////////
		// Log received packet data
		HexDump("[+] NET_ReceiveDatagram", 0, &pkt->data[i], pkt->wiresize);
	}

	return result;
}

unsigned int Hooks::NET_SendDatagram(SOCKET s, const char* buf, int len, int flags)
{
	unsigned int result = originalNET_SendDatagram(s, buf, len, flags);
	if (result)
	{
		///////////////////////////////////////////////////////////////////////////
		// Log transmitted packet data
		HexDump("[+] NET_SendDatagram", 0, buf, len);
	}

	return result;
}