//=============================================================================//
//
// Purpose: Net system utilities
//
//=============================================================================//

#include "core/stdafx.h"
#include "engine/net.h"

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
		case WSASYSNOTREADY    : return "WSASYSNOTREADY";
		case WSAVERNOTSUPPORTED: return "WSAVERNOTSUPPORTED";
		case WSANOTINITIALISED : return "WSANOTINITIALISED";
		case WSAHOST_NOT_FOUND : return "WSAHOST_NOT_FOUND";
		case WSATRY_AGAIN      : return "WSATRY_AGAIN";
		case WSANO_RECOVERY    : return "WSANO_RECOVERY";
		case WSANO_DATA        : return "WSANO_DATA";
	default                    : return "UNKNOWN ERROR";
	}
}
