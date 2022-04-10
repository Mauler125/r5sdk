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
#include "vstdlib/completion.h"
#include "mathlib/color.h"
#include "engine/sys_utils.h"
#include "engine/net.h"
#include "engine/net_chan.h"
#ifndef CLIENT_DLL
#include "engine/baseclient.h"
#endif // !CLIENT_DLL
#endif // !NETCONSOLE

#ifndef NETCONSOLE
//-----------------------------------------------------------------------------
// Purpose: shutdown netchannel
//-----------------------------------------------------------------------------
void NET_ShutDown(void* thisptr, const char* szReason, std::uint8_t a1, char a2)
{
#if !defined (GAMEDLL_S0) || !defined (GAMEDLL_S1) // !TEMP UNTIL CHOSTSTATE IS BUILD AGNOSTIC! //
	DownloadPlaylists_f_CompletionFunc(); // Re-load playlist from disk after getting disconnected from the server.
#endif // !GAMEDLL_S0 || !GAMEDLL_S1
	v_NET_Shutdown(thisptr, szReason, a1, a2);
}

//-----------------------------------------------------------------------------
// Purpose: hook and log the receive datagram
//-----------------------------------------------------------------------------
bool NET_ReceiveDatagram(int iSocket, netpacket_s* pInpacket, bool bRaw)
{
	bool result = v_NET_ReceiveDatagram(iSocket, pInpacket, bRaw);
	if (result)
	{
		// Log received packet data.
		HexDump("[+] NET_ReceiveDatagram", 0, &pInpacket->data[NULL], pInpacket->wiresize);
	}
	return result;
}

//-----------------------------------------------------------------------------
// Purpose: hook and log the send datagram
//-----------------------------------------------------------------------------
void* NET_SendDatagram(SOCKET s, const char* szPayload, int iLenght, int nFlags)
{
	void* result = v_NET_SendDatagram(s, szPayload, iLenght, nFlags);
	if (result)
	{
		// Log transmitted packet data.
		HexDump("[+] NET_SendDatagram", 0, szPayload, iLenght);
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
	net_userandomkey->SetValue(1);

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
// Purpose: disconnect the client and shutdown netchannel
//-----------------------------------------------------------------------------
void NET_DisconnectClient(CBaseClient* pClient, int nIndex, const char* szReason, uint8_t unk1, char unk2)
{
#ifndef CLIENT_DLL
	if (!pClient || std::strlen(szReason) == NULL || !pClient->GetNetChan())
	{
		return;
	}

	v_NET_Shutdown(pClient->GetNetChan(), szReason, unk1, unk2); // Shutdown netchan.
	pClient->SetNetChan(nullptr);                              // Null netchan.
	CBaseClient_Clear(pClient);                                // Reset CClient instance for client.
	g_bIsPersistenceVarSet[nIndex] = false;                    // Reset Persistence var.
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
	DetourAttach((LPVOID*)&v_NET_PrintFunc, &NET_PrintFunc);
#ifndef DEDICATED
	DetourAttach((LPVOID*)&v_NET_Shutdown, &NET_ShutDown);
#endif
}

void NET_Detach()
{
	DetourDetach((LPVOID*)&v_NET_PrintFunc, &NET_PrintFunc);
#ifndef DEDICATED
	DetourDetach((LPVOID*)&v_NET_Shutdown, &NET_ShutDown);
#endif
}

void NET_Trace_Attach()
{
	DetourAttach((LPVOID*)&v_NET_ReceiveDatagram, &NET_ReceiveDatagram);
	DetourAttach((LPVOID*)&v_NET_SendDatagram, &NET_SendDatagram);
}

void NET_Trace_Detach()
{
	DetourDetach((LPVOID*)&v_NET_ReceiveDatagram, &NET_ReceiveDatagram);
	DetourDetach((LPVOID*)&v_NET_SendDatagram, &NET_SendDatagram);
}

///////////////////////////////////////////////////////////////////////////////
string g_szNetKey = "WDNWLmJYQ2ZlM0VoTid3Yg==";
uintptr_t g_pNetKey = g_mGameDll.FindString("client:NetEncryption_NewKey").FindPatternSelf("48 8D ? ? ? ? ? 48 3B", CMemory::Direction::UP, 300).ResolveRelativeAddressSelf(0x3, 0x7).GetPtr();
#endif // !NETCONSOLE
