//=============================================================================//
//
// Purpose: Net system utilities
//
//=============================================================================//

#include "core/stdafx.h"
#include "engine/net.h"
#ifndef NETCONSOLE
#include "core/logdef.h"
#include "tier0/frametask.h"
#include "tier1/cvar.h"
#include "vpc/keyvalues.h"
#include "vstdlib/callback.h"
#include "mathlib/color.h"
#include "engine/net.h"
#include "engine/net_chan.h"
#include "vpc/keyvalues.h"
#ifndef CLIENT_DLL
#include "engine/server/server.h"
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
int NET_SendDatagram(SOCKET s, void* pPayload, int iLenght, netadr_t* pAdr, bool bEncrypt)
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
// Input  : svNetKey - 
//-----------------------------------------------------------------------------
void NET_SetKey(const string& svNetKey)
{
	std::lock_guard<std::mutex> l(g_NetKeyMutex);

	if (svNetKey.size() == AES_128_B64_ENCODED_SIZE &&
		IsValidBase64(svNetKey, &g_svNetKey)) // Results are tokenized by 'IsValidBase64()'.
	{
		v_NET_SetKey(g_pNetKey, g_svNetKey.c_str());

		DevMsg(eDLL_T::ENGINE, "Installed NetKey: '%s%s%s'\n",
			g_svGreyB.c_str(), g_svNetKey.c_str(), g_svReset.c_str());
	}
	else
	{
		Error(eDLL_T::ENGINE, NO_ERROR, "AES-128 key not encoded or invalid\n");
	}
}

//-----------------------------------------------------------------------------
// Purpose: calculates and sets the encryption key
//-----------------------------------------------------------------------------
void NET_GenerateKey()
{
	if (!net_useRandomKey->GetBool())
	{
		net_useRandomKey->SetValue(1);
		return; // Change callback will handle this.
	}

	BCRYPT_ALG_HANDLE hAlgorithm;
	if (BCryptOpenAlgorithmProvider(&hAlgorithm, L"RNG", 0, 0) < 0)
	{
		Error(eDLL_T::ENGINE, NO_ERROR, "Failed to open rng algorithm\n");
		return;
	}

	uint8_t pBuffer[AES_128_KEY_SIZE];
	if (BCryptGenRandom(hAlgorithm, pBuffer, AES_128_KEY_SIZE, 0) < 0)
	{
		Error(eDLL_T::ENGINE, NO_ERROR, "Failed to generate random data\n");
		return;
	}

	NET_SetKey(Base64Encode(string(reinterpret_cast<char*>(&pBuffer), AES_128_KEY_SIZE)));
}

//-----------------------------------------------------------------------------
// Purpose: hook and log the client's signonstate to the console
// Input  : *fmt - 
//			... - 
//-----------------------------------------------------------------------------
void NET_PrintFunc(const char* fmt, ...)
{
	static char buf[2048];
#ifndef DEDICATED
	const static eDLL_T context = eDLL_T::CLIENT;
#else // !DEDICATED
	const static eDLL_T context = eDLL_T::SERVER;
#endif
	va_list args;
	va_start(args, fmt);

	vsnprintf(buf, sizeof(buf), fmt, args);

	buf[sizeof(buf) - 1] = '\0';
	va_end(args);

	DevMsg(context, "%s", buf);
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
#ifndef DEDICATED
	// Re-load playlist from the disk to replace the one we received from the server.
	_DownloadPlaylists_f();
	KeyValues::InitPlaylists();
#endif // !DEDICATED

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
void NET_RemoveChannel(CClient* pClient, int nIndex, const char* szReason, uint8_t bBadRep, bool bRemoveNow)
{
#ifndef CLIENT_DLL
	if (!pClient || std::strlen(szReason) == NULL || !pClient->GetNetChan())
	{
		return;
	}

	v_NET_Shutdown(pClient->GetNetChan(), szReason, bBadRep, bRemoveNow); // Shutdown NetChannel.
	pClient->Clear();                                                     // Reset CClient instance for client.
	g_ServerPlayer[nIndex].Reset();                                       // Reset ServerPlayer slot.
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
		case WSAEINTR                   : return "WSAEINTR";
		case WSAEBADF                   : return "WSAEBADF";
		case WSAEACCES                  : return "WSAEACCES";
		case WSAEFAULT                  : return "WSAEFAULT";
		case WSAEINVAL                  : return "WSAEINVAL";
		case WSAEMFILE                  : return "WSAEMFILE";
		case WSAEWOULDBLOCK             : return "WSAEWOULDBLOCK";
		case WSAEINPROGRESS             : return "WSAEINPROGRESS";
		case WSAEALREADY                : return "WSAEALREADY";
		case WSAENOTSOCK                : return "WSAENOTSOCK";
		case WSAEDESTADDRREQ            : return "WSAEDESTADDRREQ";
		case WSAEMSGSIZE                : return "WSAEMSGSIZE";
		case WSAEPROTOTYPE              : return "WSAEPROTOTYPE";
		case WSAENOPROTOOPT             : return "WSAENOPROTOOPT";
		case WSAEPROTONOSUPPORT         : return "WSAEPROTONOSUPPORT";
		case WSAESOCKTNOSUPPORT         : return "WSAESOCKTNOSUPPORT";
		case WSAEOPNOTSUPP              : return "WSAEOPNOTSUPP";
		case WSAEPFNOSUPPORT            : return "WSAEPFNOSUPPORT";
		case WSAEAFNOSUPPORT            : return "WSAEAFNOSUPPORT";
		case WSAEADDRINUSE              : return "WSAEADDRINUSE";
		case WSAEADDRNOTAVAIL           : return "WSAEADDRNOTAVAIL";
		case WSAENETDOWN                : return "WSAENETDOWN";
		case WSAENETUNREACH             : return "WSAENETUNREACH";
		case WSAENETRESET               : return "WSAENETRESET";
		case WSAECONNABORTED            : return "WSAECONNABORTED";
		case WSAECONNRESET              : return "WSAECONNRESET";
		case WSAENOBUFS                 : return "WSAENOBUFS";
		case WSAEISCONN                 : return "WSAEISCONN";
		case WSAENOTCONN                : return "WSAENOTCONN";
		case WSAESHUTDOWN               : return "WSAESHUTDOWN";
		case WSAETOOMANYREFS            : return "WSAETOOMANYREFS";
		case WSAETIMEDOUT               : return "WSAETIMEDOUT";
		case WSAECONNREFUSED            : return "WSAECONNREFUSED";
		case WSAELOOP                   : return "WSAELOOP";
		case WSAENAMETOOLONG            : return "WSAENAMETOOLONG";
		case WSAEHOSTDOWN               : return "WSAEHOSTDOWN";
		case WSAEHOSTUNREACH            : return "WSAEHOSTUNREACH";
		case WSAENOTEMPTY               : return "WSAENOTEMPTY";
		case WSAEPROCLIM                : return "WSAEPROCLIM";
		case WSAEUSERS                  : return "WSAEUSERS";
		case WSAEDQUOT                  : return "WSAEDQUOT";
		case WSAESTALE                  : return "WSAESTALE";
		case WSAEREMOTE                 : return "WSAEREMOTE";
		case WSASYSNOTREADY             : return "WSASYSNOTREADY";
		case WSAVERNOTSUPPORTED         : return "WSAVERNOTSUPPORTED";
		case WSANOTINITIALISED          : return "WSANOTINITIALISED";
		case WSAEDISCON                 : return "WSAEDISCON";
		case WSAENOMORE                 : return "WSAENOMORE";
		case WSAECANCELLED              : return "WSAECANCELLED";
		case WSAEINVALIDPROCTABLE       : return "WSAEINVALIDPROCTABLE";
		case WSAEINVALIDPROVIDER        : return "WSAEINVALIDPROVIDER";
		case WSAEPROVIDERFAILEDINIT     : return "WSAEPROVIDERFAILEDINIT";
		case WSASYSCALLFAILURE          : return "WSASYSCALLFAILURE";
		case WSASERVICE_NOT_FOUND       : return "WSASERVICE_NOT_FOUND";
		case WSATYPE_NOT_FOUND          : return "WSATYPE_NOT_FOUND";
		case WSA_E_NO_MORE              : return "WSA_E_NO_MORE";
		case WSA_E_CANCELLED            : return "WSA_E_CANCELLED";
		case WSAEREFUSED                : return "WSAEREFUSED";
		case WSAHOST_NOT_FOUND          : return "WSAHOST_NOT_FOUND";
		case WSATRY_AGAIN               : return "WSATRY_AGAIN";
		case WSANO_RECOVERY             : return "WSANO_RECOVERY";
		case WSANO_DATA                 : return "WSANO_DATA";
		case WSA_QOS_RECEIVERS          : return "WSA_QOS_RECEIVERS";
		case WSA_QOS_SENDERS            : return "WSA_QOS_SENDERS";
		case WSA_QOS_NO_SENDERS         : return "WSA_QOS_NO_SENDERS";
		case WSA_QOS_NO_RECEIVERS       : return "WSA_QOS_NO_RECEIVERS";
		case WSA_QOS_REQUEST_CONFIRMED  : return "WSA_QOS_REQUEST_CONFIRMED";
		case WSA_QOS_ADMISSION_FAILURE  : return "WSA_QOS_ADMISSION_FAILURE";
		case WSA_QOS_POLICY_FAILURE     : return "WSA_QOS_POLICY_FAILURE";
		case WSA_QOS_BAD_STYLE          : return "WSA_QOS_BAD_STYLE";
		case WSA_QOS_BAD_OBJECT         : return "WSA_QOS_BAD_OBJECT";
		case WSA_QOS_TRAFFIC_CTRL_ERROR : return "WSA_QOS_TRAFFIC_CTRL_ERROR";
		case WSA_QOS_GENERIC_ERROR      : return "WSA_QOS_GENERIC_ERROR";
		case WSA_QOS_ESERVICETYPE       : return "WSA_QOS_ESERVICETYPE";
		case WSA_QOS_EFLOWSPEC          : return "WSA_QOS_EFLOWSPEC";
		case WSA_QOS_EPROVSPECBUF       : return "WSA_QOS_EPROVSPECBUF";
		case WSA_QOS_EFILTERSTYLE       : return "WSA_QOS_EFILTERSTYLE";
		case WSA_QOS_EFILTERTYPE        : return "WSA_QOS_EFILTERTYPE";
		case WSA_QOS_EFILTERCOUNT       : return "WSA_QOS_EFILTERCOUNT";
		case WSA_QOS_EOBJLENGTH         : return "WSA_QOS_EOBJLENGTH";
		case WSA_QOS_EFLOWCOUNT         : return "WSA_QOS_EFLOWCOUNT";
		case WSA_QOS_EUNKOWNPSOBJ       : return "WSA_QOS_EUNKOWNPSOBJ";
		case WSA_QOS_EPOLICYOBJ         : return "WSA_QOS_EPOLICYOBJ";
		case WSA_QOS_EFLOWDESC          : return "WSA_QOS_EFLOWDESC";
		case WSA_QOS_EPSFLOWSPEC        : return "WSA_QOS_EPSFLOWSPEC";
		case WSA_QOS_EPSFILTERSPEC      : return "WSA_QOS_EPSFILTERSPEC";
		case WSA_QOS_ESDMODEOBJ         : return "WSA_QOS_ESDMODEOBJ";
		case WSA_QOS_ESHAPERATEOBJ      : return "WSA_QOS_ESHAPERATEOBJ";
		case WSA_QOS_RESERVED_PETYPE    : return "WSA_QOS_RESERVED_PETYPE";
		case WSA_SECURE_HOST_NOT_FOUND  : return "WSA_SECURE_HOST_NOT_FOUND";
		case WSA_IPSEC_NAME_POLICY_ERROR: return "WSA_IPSEC_NAME_POLICY_ERROR";
	default                    : return "UNKNOWN_ERROR";
	}
}

#ifndef NETCONSOLE
///////////////////////////////////////////////////////////////////////////////
void VNet::Attach() const
{
	DetourAttach((LPVOID*)&v_NET_ReceiveDatagram, &NET_ReceiveDatagram);
	DetourAttach((LPVOID*)&v_NET_SendDatagram, &NET_SendDatagram);
	DetourAttach((LPVOID*)&v_NET_PrintFunc, &NET_PrintFunc);
#ifndef DEDICATED
	DetourAttach((LPVOID*)&v_NET_Shutdown, &NET_Shutdown);
#endif
}

void VNet::Detach() const
{
	DetourDetach((LPVOID*)&v_NET_ReceiveDatagram, &NET_ReceiveDatagram);
	DetourDetach((LPVOID*)&v_NET_SendDatagram, &NET_SendDatagram);
	DetourDetach((LPVOID*)&v_NET_PrintFunc, &NET_PrintFunc);
#ifndef DEDICATED
	DetourDetach((LPVOID*)&v_NET_Shutdown, &NET_Shutdown);
#endif
}

///////////////////////////////////////////////////////////////////////////////
string g_svNetKey = DEFAULT_NET_ENCRYPTION_KEY;
uintptr_t g_pNetKey = NULL;
#endif // !NETCONSOLE
