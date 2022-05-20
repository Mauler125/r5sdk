//=============================================================================//
//
// Purpose: Net system utilities
//
//=============================================================================//

#include "core/stdafx.h"
#include "engine/net.h"
#ifndef NETCONSOLE
#include "core/logdef.h"
#include "tier1/cvar.h"
#include "vstdlib/callback.h"
#include "mathlib/color.h"
#include "engine/sys_utils.h"
#include "engine/net.h"
#include "engine/net_chan.h"
#ifndef CLIENT_DLL
#include "engine/client/client.h"
#endif // !CLIENT_DLL
#endif // !NETCONSOLE

#ifndef NETCONSOLE
//-----------------------------------------------------------------------------
// Purpose: hook and log the receive datagram
// Input  : iSocket - 
//			*pInpacket - 
//			bEncrypted - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool NET_ReceiveDatagram(int iSocket, netpacket_s* pInpacket, bool bEncrypted)
{
	bool result = v_NET_ReceiveDatagram(iSocket, pInpacket, net_encryptionEnable->GetBool());
	if (result && net_tracePayload->GetBool())
	{
		// Log received packet data.
		HexDump("[+] NET_ReceiveDatagram", "net_trace", &pInpacket->pData[NULL], pInpacket->wiresize);
	}
	return result;
}

//-----------------------------------------------------------------------------
// Purpose: hook and log the send datagram
// Input  : s - 
//			*pPayload - 
//			iLenght - 
//			*pAdr - 
//			bEncrypt - 
// Output : outgoing sequence number for this packet
//-----------------------------------------------------------------------------
int NET_SendDatagram(SOCKET s, void* pPayload, int iLenght, v_netadr_t* pAdr, bool bEncrypt)
{
	int result = v_NET_SendDatagram(s, pPayload, iLenght, pAdr, net_encryptionEnable->GetBool());
	if (result && net_tracePayload->GetBool())
	{
		// Log transmitted packet data.
		HexDump("[+] NET_SendDatagram", "net_trace", pPayload, iLenght);
	}
	return result;
}

//-----------------------------------------------------------------------------
// Purpose: sets the user specified encryption key
// Input  : *svNetKey - 
//-----------------------------------------------------------------------------
void NET_SetKey(const string& svNetKey)
{
	g_szNetKey.clear();
	g_szNetKey = svNetKey;

	DevMsg(eDLL_T::ENGINE, "______________________________________________________________\n");
	DevMsg(eDLL_T::ENGINE, "] NET_KEY ----------------------------------------------------\n");
	DevMsg(eDLL_T::ENGINE, "] BASE64: %s%s%s\n", g_svGreyB.c_str(), g_szNetKey.c_str(), g_svReset.c_str());
	DevMsg(eDLL_T::ENGINE, "--------------------------------------------------------------\n");

	v_NET_SetKey(g_pNetKey, g_szNetKey.c_str());
}

//-----------------------------------------------------------------------------
// Purpose: calculates and sets the encryption key
//-----------------------------------------------------------------------------
void NET_GenerateKey()
{
	g_szNetKey.clear();
	net_useRandomKey->SetValue(1);

	BCRYPT_ALG_HANDLE hAlgorithm;
	if (BCryptOpenAlgorithmProvider(&hAlgorithm, L"RNG", 0, 0) < 0)
	{
		Error(eDLL_T::ENGINE, "Failed to open rng algorithm\n");
		return;
	}
	unsigned char pBuffer[0x10u];
	if (BCryptGenRandom(hAlgorithm, pBuffer, 0x10u, 0) < 0)
	{
		Error(eDLL_T::ENGINE, "Failed to generate random data\n");
		return;
	}

	for (int i = 0; i < 0x10u; i++)
	{
		g_szNetKey += pBuffer[i];
	}

	g_szNetKey = Base64Encode(g_szNetKey);

	DevMsg(eDLL_T::ENGINE, "______________________________________________________________\n");
	DevMsg(eDLL_T::ENGINE, "] NET_KEY ----------------------------------------------------\n");
	DevMsg(eDLL_T::ENGINE, "] BASE64: %s%s%s\n", g_svGreyB.c_str(), g_szNetKey.c_str(), g_svReset.c_str());
	DevMsg(eDLL_T::ENGINE, "--------------------------------------------------------------\n");

	v_NET_SetKey(g_pNetKey, g_szNetKey.c_str());
}

//-----------------------------------------------------------------------------
// Purpose: hook and log the client's signonstate to the console
// Input  : *fmt - 
//			... - 
//-----------------------------------------------------------------------------
void NET_PrintFunc(const char* fmt, ...)
{
	static char buf[1024];

	va_list args;
	va_start(args, fmt);

	vsnprintf(buf, sizeof(buf), fmt, args);

	buf[sizeof(buf) - 1] = 0;
	va_end(args);

	DevMsg(eDLL_T::CLIENT, "%s", buf);
}

//-----------------------------------------------------------------------------
// Purpose: shutdown netchannel
// Input  : *this - 
//			*szReason - 
//			bBadRep - 
//			bRemoveNow - 
//-----------------------------------------------------------------------------
void NET_Shutdown(void* thisptr, const char* szReason, uint8_t bBadRep, bool bRemoveNow)
{
	_DownloadPlaylists_f(); // Re-load playlist from disk after getting disconnected from the server.
	v_NET_Shutdown(thisptr, szReason, bBadRep, bRemoveNow);
}

//-----------------------------------------------------------------------------
// Purpose: disconnect the client and shutdown netchannel
// Input  : *pClient - 
//			nIndex - 
//			*szReason - 
//			bBadRep - 
//			bRemoveNow - 
//-----------------------------------------------------------------------------
void NET_DisconnectClient(CClient* pClient, int nIndex, const char* szReason, uint8_t bBadRep, bool bRemoveNow)
{
#ifndef CLIENT_DLL
	if (!pClient || std::strlen(szReason) == NULL || !pClient->GetNetChan())
	{
		return;
	}

	v_NET_Shutdown(pClient->GetNetChan(), szReason, bBadRep, bRemoveNow); // Shutdown netchan.
	pClient->SetNetChan(nullptr);                                         // Null netchan.
	CBaseClient_Clear(pClient);                                           // Reset CClient instance for client.
	g_bIsPersistenceVarSet[nIndex] = false;                               // Reset Persistence var.
#endif // !CLIENT_DLL
}
#endif // !NETCONSOLE

//-----------------------------------------------------------------------------
// Purpose: returns the WSA error code
//-----------------------------------------------------------------------------
const char* NET_ErrorString(int iCode)
{
	switch (iCode)
	{
		case WSAEINTR          : return "WSAEINTR";
		case WSAEBADF          : return "WSAEBADF";
		case WSAEACCES         : return "WSAEACCES";
		case WSAEDISCON        : return "WSAEDISCON";
		case WSAEFAULT         : return "WSAEFAULT";
		case WSAEINVAL         : return "WSAEINVAL";
		case WSAEMFILE         : return "WSAEMFILE";
		case WSAEWOULDBLOCK    : return "WSAEWOULDBLOCK";
		case WSAEINPROGRESS    : return "WSAEINPROGRESS";
		case WSAEALREADY       : return "WSAEALREADY";
		case WSAENOTSOCK       : return "WSAENOTSOCK";
		case WSAEDESTADDRREQ   : return "WSAEDESTADDRREQ";
		case WSAEMSGSIZE       : return "WSAEMSGSIZE";
		case WSAEPROTOTYPE     : return "WSAEPROTOTYPE";
		case WSAENOPROTOOPT    : return "WSAENOPROTOOPT";
		case WSAEPROTONOSUPPORT: return "WSAEPROTONOSUPPORT";
		case WSAESOCKTNOSUPPORT: return "WSAESOCKTNOSUPPORT";
		case WSAEOPNOTSUPP     : return "WSAEOPNOTSUPP";
		case WSAEPFNOSUPPORT   : return "WSAEPFNOSUPPORT";
		case WSAEAFNOSUPPORT   : return "WSAEAFNOSUPPORT";
		case WSAEADDRINUSE     : return "WSAEADDRINUSE";
		case WSAEADDRNOTAVAIL  : return "WSAEADDRNOTAVAIL";
		case WSAENETDOWN       : return "WSAENETDOWN";
		case WSAENETUNREACH    : return "WSAENETUNREACH";
		case WSAENETRESET      : return "WSAENETRESET";
		case WSAECONNABORTED   : return "WSWSAECONNABORTEDAEINTR";
		case WSAECONNRESET     : return "WSAECONNRESET";
		case WSAENOBUFS        : return "WSAENOBUFS";
		case WSAEISCONN        : return "WSAEISCONN";
		case WSAENOTCONN       : return "WSAENOTCONN";
		case WSAESHUTDOWN      : return "WSAESHUTDOWN";
		case WSAETOOMANYREFS   : return "WSAETOOMANYREFS";
		case WSAETIMEDOUT      : return "WSAETIMEDOUT";
		case WSAECONNREFUSED   : return "WSAECONNREFUSED";
		case WSAELOOP          : return "WSAELOOP";
		case WSAENAMETOOLONG   : return "WSAENAMETOOLONG";
		case WSAEHOSTDOWN      : return "WSAEHOSTDOWN";
		case WSAEPROCLIM       : return "WSAEPROCLIM";
		case WSASYSNOTREADY    : return "WSASYSNOTREADY";
		case WSAVERNOTSUPPORTED: return "WSAVERNOTSUPPORTED";
		case WSANOTINITIALISED : return "WSANOTINITIALISED";
		case WSAHOST_NOT_FOUND : return "WSAHOST_NOT_FOUND";
		case WSATRY_AGAIN      : return "WSATRY_AGAIN";
		case WSANO_RECOVERY    : return "WSANO_RECOVERY";
		case WSANO_DATA        : return "WSANO_DATA";
	default                    : return "UNKNOWN_ERROR";
	}
}

#ifndef NETCONSOLE
///////////////////////////////////////////////////////////////////////////////
void NET_Attach()
{
	DetourAttach((LPVOID*)&v_NET_ReceiveDatagram, &NET_ReceiveDatagram);
	DetourAttach((LPVOID*)&v_NET_SendDatagram, &NET_SendDatagram);
	DetourAttach((LPVOID*)&v_NET_PrintFunc, &NET_PrintFunc);
#ifndef DEDICATED
	DetourAttach((LPVOID*)&v_NET_Shutdown, &NET_Shutdown);
#endif
}

void NET_Detach()
{
	DetourDetach((LPVOID*)&v_NET_ReceiveDatagram, &NET_ReceiveDatagram);
	DetourDetach((LPVOID*)&v_NET_SendDatagram, &NET_SendDatagram);
	DetourDetach((LPVOID*)&v_NET_PrintFunc, &NET_PrintFunc);
#ifndef DEDICATED
	DetourDetach((LPVOID*)&v_NET_Shutdown, &NET_Shutdown);
#endif
}

///////////////////////////////////////////////////////////////////////////////
string g_szNetKey = "WDNWLmJYQ2ZlM0VoTid3Yg==";
uintptr_t g_pNetKey = NULL;
#endif // !NETCONSOLE
