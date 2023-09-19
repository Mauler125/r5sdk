//===========================================================================//
//
// Purpose: implementation of the CNetAdr class.
// --------------------------------------------------------------------------
//===========================================================================//

#include "tier1/NetAdr.h"
#include "tier1/strtools.h"

//////////////////////////////////////////////////////////////////////
// Clears IP.
//////////////////////////////////////////////////////////////////////
void CNetAdr::Clear(void)
{
	adr = { };
	port = 0;
	reliable = 0;
	type = netadrtype_t::NA_NULL;
}

//////////////////////////////////////////////////////////////////////
// Compares two addresses.
//////////////////////////////////////////////////////////////////////
bool CNetAdr::CompareAdr(const CNetAdr& other) const
{
	if (type != other.type)
	{
		return false;
	}
	if (type == netadrtype_t::NA_LOOPBACK)
	{
		return true;
	}
	if (type == netadrtype_t::NA_IP)
	{
		return (CompareIPv6(adr, other.adr) == 0);
	}

	return false;
}

//////////////////////////////////////////////////////////////////////
// Convert address to string.
//////////////////////////////////////////////////////////////////////
const char* CNetAdr::ToString(bool bOnlyBase) const
{
	// Main thread only due to use of static buffers.
	//Assert(ThreadInMainThread());

	// Select a static buffer.
	static char s[4][128];
	static int slot = 0;
	int useSlot = (slot++) % 4;

	// Render into it.
	ToString(s[useSlot], sizeof(s[0]), bOnlyBase);

	// Pray the caller uses it before it gets clobbered.
	return s[useSlot];
}

//////////////////////////////////////////////////////////////////////
// Convert address to string.
//////////////////////////////////////////////////////////////////////
void CNetAdr::ToString(char* pchBuffer, size_t unBufferSize, bool bOnlyBase) const
{
	if (type == netadrtype_t::NA_NULL)
	{
		strncpy(pchBuffer, "null", unBufferSize);
	}
	else if (type == netadrtype_t::NA_LOOPBACK)
	{
		strncpy(pchBuffer, "loopback", unBufferSize);
	}
	else if (type == netadrtype_t::NA_IP)
	{
		char pStringBuf[128];
		inet_ntop(AF_INET6, &adr, pStringBuf, INET6_ADDRSTRLEN);

		if (bOnlyBase)
		{
			snprintf(pchBuffer, unBufferSize, "%s", pStringBuf);
		}
		else
		{
			snprintf(pchBuffer, unBufferSize, "[%s]:%i", pStringBuf, ntohs(port));
		}
	}
	else
	{
		memmove(pchBuffer, "unknown", unBufferSize);
	}
}

//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////
void CNetAdr::ToAdrinfo(addrinfo* pHint) const
{
	addrinfo hint{};
	hint.ai_flags = AI_PASSIVE;
	hint.ai_family = AF_INET6;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_protocol = IPPROTO_TCP;

	char szBuffer[33];
	int results = getaddrinfo(ToString(true), _itoa(GetPort(), szBuffer, 10), &hint, &pHint);

	if (results != 0)
	{
		CHAR* wszError = gai_strerror(results);
		Warning(eDLL_T::COMMON, "Address info translation failed: (%s)\n", wszError);
	}
}

//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////
void CNetAdr::ToSockadr(struct sockaddr_storage* pSadr) const
{
	memset(pSadr, NULL, sizeof(struct sockaddr));

	if (GetType() == netadrtype_t::NA_IP)
	{
		reinterpret_cast<sockaddr_in6*>(pSadr)->sin6_family = AF_INET6;
		reinterpret_cast<sockaddr_in6*>(pSadr)->sin6_port = port;
		inet_pton(AF_INET6, ToString(true), &reinterpret_cast<sockaddr_in6*>(pSadr)->sin6_addr);
	}
	else if (GetType() == netadrtype_t::NA_LOOPBACK)
	{
		reinterpret_cast<sockaddr_in6*>(pSadr)->sin6_family = AF_INET6;
		reinterpret_cast<sockaddr_in6*>(pSadr)->sin6_port = port;
		reinterpret_cast<sockaddr_in6*>(pSadr)->sin6_addr = in6addr_loopback;
	}
}

//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////
bool CNetAdr::SetFromSockadr(struct sockaddr_storage* s)
{
	char szAdrv6[INET6_ADDRSTRLEN]{};
	sockaddr_in6* pAdrv6 = reinterpret_cast<sockaddr_in6*>(s);

	inet_ntop(pAdrv6->sin6_family, &pAdrv6->sin6_addr, szAdrv6, sizeof(sockaddr_in6)); // TODO: Error check?

	SetFromString(szAdrv6);
	SetPort(pAdrv6->sin6_port);

	return true;
}

//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////
bool CNetAdr::SetFromString(const char* pch, bool bUseDNS)
{
	Clear();
	if (!pch)
	{
		Assert(pch, "Invalid call: 'szIpAdr' was nullptr.");
		return false;
	}

	SetType(netadrtype_t::NA_IP);

	char szAddress[128];
	strncpy(szAddress, pch, sizeof(szAddress));

	char* pszAddress = szAddress;
	szAddress[sizeof(szAddress) - 1] = '\0';

	if (szAddress[0] == '[') // Skip bracket.
	{
		pszAddress = &szAddress[1];
	}

	char* bracketEnd = strchr(szAddress, ']');
	if (bracketEnd) // Get and remove the last bracket.
	{
		*bracketEnd = '\0';

		char* portStart = &bracketEnd[1];
		char* pchColon = strrchr(portStart, ':');

		if (pchColon && strchr(portStart, ':') == pchColon)
		{
			pchColon[0] = '\0'; // Set the port.
			SetPort(uint16_t(htons(uint16_t(atoi(&pchColon[1])))));
		}
	}

	if (!strchr(pszAddress, ':'))
	{
		char szNewAddressV4[128];
		V_snprintf(szNewAddressV4, sizeof(szNewAddressV4), "::FFFF:%s", pszAddress);

		if (inet_pton(AF_INET6, szNewAddressV4, &this->adr) > 0)
		{
			return true;
		}
	}
	else // Address is formatted as IPv6.
	{
		if (inet_pton(AF_INET6, pszAddress, &this->adr) > 0)
		{
			return true;
		}
	}

	if (bUseDNS) // Perform DNS lookup instead.
	{
		ADDRINFOA pHints{};
		PADDRINFOA ppResult = nullptr;

		pHints.ai_family = AF_INET6;
		pHints.ai_flags = AI_ALL | AI_V4MAPPED;
		pHints.ai_socktype = NULL;
		pHints.ai_addrlen = NULL;
		pHints.ai_canonname = nullptr;
		pHints.ai_addr = nullptr;
		pHints.ai_next = nullptr;

		if (getaddrinfo(pszAddress, nullptr, &pHints, &ppResult))
		{
			freeaddrinfo(ppResult);
			return false;
		}

		SetIP(reinterpret_cast<IN6_ADDR*>(&ppResult->ai_addr->sa_data[6]));
		freeaddrinfo(ppResult);

		return true;
	}

	return false;
}
