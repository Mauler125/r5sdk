#include "pch.h"
#include "hooks.h"

namespace Hooks
{
	NET_ReceiveDatagramFn originalNET_ReceiveDatagram = nullptr;
	NET_SendDatagramFn originalNET_SendDatagram = nullptr;
}

typedef unsigned __int64 QWORD;

struct __declspec(align(8)) netpacket_t
{
	DWORD family_maybe;
	sockaddr_in sin;
	WORD sin_port;
	BYTE gap16;
	BYTE byte17;
	DWORD source;
	double received;
	unsigned __int8* data;
	QWORD label;
	BYTE byte38;
	QWORD qword40;
	QWORD qword48;
	BYTE gap50[8];
	QWORD qword58;
	QWORD qword60;
	QWORD qword68;
	int less_than_12;
	DWORD wiresize;
	BYTE gap78[8];
	QWORD qword80;
};

//-----------------------------------------------------------------------------
// Hook and log the receive datagram
//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------
// Hook and log send datagram
//-----------------------------------------------------------------------------

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