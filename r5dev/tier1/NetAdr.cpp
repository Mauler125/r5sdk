//===========================================================================//
//
// Purpose: implementation of the CNetAdr class.
// --------------------------------------------------------------------------
// 
// NOTE: This implementation is considered deprecated. I rebuilded this
// not knowing that the engine supports IPv6 as well. I have fully rewritten
// this class in 'tier1/NetAdr2.cpp' in modern C++. Please use this instead.
// This class is for reference material only may some bits in the engine line
// up with this original 'CNetAdr' implementation.
// 
//===========================================================================//

#include "core/stdafx.h"
#include "tier1/netadr.h"
#include "mathlib/swap.h"

//////////////////////////////////////////////////////////////////////
// Constructors
//////////////////////////////////////////////////////////////////////
netadr_s::netadr_s(void)
{
	SetIP(0);
	SetPort(0);
	SetType(netadrtype_t::NA_IP);
}
netadr_s::netadr_s(std::uint32_t unIP, std::uint16_t usPort)
{
	SetIP(unIP);
	SetPort(usPort);
	SetType(netadrtype_t::NA_IP);
}
netadr_s::netadr_s(const char* pch)
{
	SetFromString(pch);
}

//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////
void netadr_t::SetFromSocket(int hSocket)
{
	Clear();
	type = netadrtype_t::NA_IP;

	struct sockaddr address{};
	socklen_t namelen = sizeof(address);
	if (getsockname(hSocket, (struct sockaddr*)&address, &namelen) == 0)
	{
		SetFromSockadr(&address);
	}
}

//////////////////////////////////////////////////////////////////////
// Compares IP for equality
//////////////////////////////////////////////////////////////////////
bool netadr_t::CompareAdr(const netadr_t& a, bool onlyBase) const
{
	if (a.type != type)
	{
		return false;
	}

	if (type == netadrtype_t::NA_LOOPBACK)
	{
		return true;
	}

	if (type == netadrtype_t::NA_BROADCAST)
	{
		return true;
	}

	if (type == netadrtype_t::NA_IP)
	{
		if (!onlyBase && (port != a.port))
		{
			return false;
		}

		if (a.ip[0] == ip[0] && a.ip[1] == ip[1] && a.ip[2] == ip[2] && a.ip[3] == ip[3])
		{
			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////
// Compares Class-B IP for equality
//////////////////////////////////////////////////////////////////////
bool netadr_t::CompareClassBAdr(const netadr_t& a) const
{
	if (a.type != type)
	{
		return false;
	}

	if (type == netadrtype_t::NA_LOOPBACK)
	{
		return true;
	}

	if (type == netadrtype_t::NA_IP)
	{
		if (a.ip[0] == ip[0] && a.ip[1] == ip[1])
		{
			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////
// Compares Class-C IP for equality
//////////////////////////////////////////////////////////////////////
bool netadr_t::CompareClassCAdr(const netadr_t& a) const
{
	if (a.type != type)
	{
		return false;
	}

	if (type == netadrtype_t::NA_LOOPBACK)
	{
		return true;
	}

	if (type == netadrtype_t::NA_IP)
	{
		if (a.ip[0] == ip[0] && a.ip[1] == ip[1] && a.ip[2] == ip[2])
		{
			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////
// Convert address to string
//////////////////////////////////////////////////////////////////////
const char* netadr_t::ToString(bool onlyBase) const
{
	// Select a static buffer.
	static char s[4][64]{};
	static int slot = 0;
	int useSlot = (slot++) % 4;

	// Render into it.
	ToString(s[useSlot], sizeof(s[0]), onlyBase);

	// Pray the caller uses it before it gets clobbered.
	return s[useSlot];
}

//////////////////////////////////////////////////////////////////////
// Convert address to string
//////////////////////////////////////////////////////////////////////
void netadr_t::ToString(char* pchBuffer, std::uint32_t unBufferSize, bool bOnlyBase) const
{
	if (type == netadrtype_t::NA_LOOPBACK)
	{
		memmove(pchBuffer, "loopback", unBufferSize);
	}
	else if (type == netadrtype_t::NA_BROADCAST)
	{
		memmove(pchBuffer, "broadcast", unBufferSize);
	}
	else if (type == netadrtype_t::NA_IP)
	{
		if (bOnlyBase)
		{
			snprintf(pchBuffer, unBufferSize, "%i.%i.%i.%i", ip[0], ip[1], ip[2], ip[3]);
		}
		else
		{
			snprintf(pchBuffer, unBufferSize, "%i.%i.%i.%i:%i", ip[0], ip[1], ip[2], ip[3], ntohs(port));
		}
	}
	else
	{
		memmove(pchBuffer, "unknown", unBufferSize);
	}
}

//////////////////////////////////////////////////////////////////////
// Clears IP
//////////////////////////////////////////////////////////////////////
void netadr_t::Clear(void)
{
	ip[0] = ip[1] = ip[2] = ip[3] = 0;
	port = 0;
	type = netadrtype_t::NA_NULL;
}

//////////////////////////////////////////////////////////////////////
// Sets IP
//////////////////////////////////////////////////////////////////////
void netadr_t::SetIP(std::uint8_t b1, std::uint8_t b2, std::uint8_t b3, std::uint8_t b4)
{
	ip[0] = b1;
	ip[1] = b2;
	ip[2] = b3;
	ip[3] = b4;
}

//////////////////////////////////////////////////////////////////////
// Sets IP
//////////////////////////////////////////////////////////////////////
void netadr_t::SetIP(std::uint32_t unIP)
{
	*((std::uint32_t*)ip) = DWordSwapC(unIP);
}

//////////////////////////////////////////////////////////////////////
// Sets type
//////////////////////////////////////////////////////////////////////
void netadr_t::SetType(netadrtype_t newtype)
{
	type = newtype;
}

//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////
netadrtype_t netadr_t::GetType(void) const
{
	return type;
}

//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////
std::uint16_t netadr_t::GetPort(void) const
{
	return WordSwapC(port);
}

//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////
std::uint32_t netadr_t::GetIPNetworkByteOrder(void) const
{
	return *(std::uint32_t*)&ip;
}

//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////
std::uint32_t netadr_t::GetIPHostByteOrder(void) const
{
	return DWordSwapC(GetIPNetworkByteOrder());
}

//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////
void netadr_t::ToSockadr(struct sockaddr* s) const
{
	memset(s, 0, sizeof(struct sockaddr));

	if (type == netadrtype_t::NA_BROADCAST)
	{
		((struct sockaddr_in*)s)->sin_family = AF_INET;
		((struct sockaddr_in*)s)->sin_port = port;
		((struct sockaddr_in*)s)->sin_addr.s_addr = INADDR_BROADCAST;
	}
	else if (type == netadrtype_t::NA_IP)
	{
		((struct sockaddr_in*)s)->sin_family = AF_INET;
		((struct sockaddr_in*)s)->sin_addr.s_addr = *(int*)&ip;
		((struct sockaddr_in*)s)->sin_port = port;
	}
	else if (type == netadrtype_t::NA_LOOPBACK)
	{
		((struct sockaddr_in*)s)->sin_family = AF_INET;
		((struct sockaddr_in*)s)->sin_port = port;
		((struct sockaddr_in*)s)->sin_addr.s_addr = INADDR_LOOPBACK;
	}
}

//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////
bool netadr_t::SetFromSockadr(const struct sockaddr* s)
{
	if (s->sa_family == AF_INET)
	{
		type = netadrtype_t::NA_IP;
		*(int*)&ip = ((struct sockaddr_in*)s)->sin_addr.s_addr;
		port = ((struct sockaddr_in*)s)->sin_port;
		return true;
	}
	else
	{
		Clear();
		return false;
	}
}

//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////
bool netadr_t::IsValid(void) const
{
	return ((port != 0) && (type != netadrtype_t::NA_NULL) &&
		(ip[0] != 0 || ip[1] != 0 || ip[2] != 0 || ip[3] != 0));
}

//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////
bool netadr_t::IsBaseAdrValid(void) const
{
	return ((type != netadrtype_t::NA_NULL) &&
		(ip[0] != 0 || ip[1] != 0 || ip[2] != 0 || ip[3] != 0));
}

//////////////////////////////////////////////////////////////////////
// Returns true if we are localhost
//////////////////////////////////////////////////////////////////////
bool netadr_t::IsLocalhost(void) const
{
	return (ip[0] == 127) && (ip[1] == 0) && (ip[2] == 0) && (ip[3] == 1);
}

//////////////////////////////////////////////////////////////////////
// Returns true if we use the loopback buffers
//////////////////////////////////////////////////////////////////////
bool netadr_t::IsLoopback(void) const
{
	return type == netadrtype_t::NA_LOOPBACK;
}

//////////////////////////////////////////////////////////////////////
// Check if address is reserved and not routable.
//////////////////////////////////////////////////////////////////////
bool netadr_t::IsReservedAdr(void) const
{
	if (type == netadrtype_t::NA_LOOPBACK)
	{
		return true;
	}

	if (type == netadrtype_t::NA_IP)
	{
		if ((ip[0] == 10)                                || // 10.x.x.x is reserved
			(ip[0] == 127)                               || // 127.x.x.x 
			(ip[0] == 172 && ip[1] >= 16 && ip[1] <= 31) || // 172.16.x.x  - 172.31.x.x
			(ip[0] == 192 && ip[1] >= 168))                 // 192.168.x.x
		{
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////
void netadr_t::SetPort(std::uint16_t newport)
{
	port = WordSwapC(newport);
}

//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////
bool netadr_t::SetFromString(const char* szIpAdr, bool bUseDNS)
{
	Clear();

	if (!szIpAdr)
	{
		Assert(szIpAdr, "Invalid call: 'szIpAdr' was nullptr.");
		return false;
	}

	type = netadrtype_t::NA_IP;

	char szAddress[128]{};
	strcpy_s(szAddress, szIpAdr);

	if (!_strnicmp(szAddress, "loopback", 8))
	{
		char szNewAddress[128]{};
		type = netadrtype_t::NA_LOOPBACK;

		strcpy_s(szNewAddress, "127.0.0.1");
		strcat_s(szNewAddress, szAddress + 8); // copy anything after "loopback"

		strcpy_s(szAddress, szNewAddress);
	}

	if (!_strnicmp(szAddress, "localhost", 9))
	{
		memcpy(szAddress, "127.0.0.1", 9); // Note use of memcpy allows us to keep the colon and rest of string since localhost and 127.0.0.1 are both 9 characters.
	}

	// IPv4 starts with a number and has a dot.
	if (szAddress[0] >= '0' && szAddress[0] <= '9' && strchr(szAddress, '.'))
	{
		int i0 = -1, i1 = -1, i2 = -1, i3 = -1, n0 = 0; // Initialize port as zero
		int nRes = sscanf_s(szAddress, "%d.%d.%d.%d:%d", &i0, &i1, &i2, &i3, &n0);
		if (
			nRes < 4
			|| i0 < 0 || i0 > 255
			|| i1 < 0 || i1 > 255
			|| i2 < 0 || i2 > 255
			|| i3 < 0 || i3 > 255
			|| n0 < 0 || n0 > 65535
			)
		{
			return false;
		}

		SetIP(i0, i1, i2, i3);
		SetPort((std::uint16_t)n0);
		return true;
	}
}

//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////
bool netadr_t::operator<(const netadr_t& netadr) const
{
	if (*((std::uint32_t*)netadr.ip) < *((std::uint32_t*)ip))
	{
		return true;
	}
	else if (*((std::uint32_t*)netadr.ip) > *((std::uint32_t*)ip))
	{
		return false;
	}
	return (netadr.port < port);
}
