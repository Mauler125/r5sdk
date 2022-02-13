//===========================================================================//
// 
// Purpose: Protocol-agnostic implementation of the CNetAdr class.
// 
//===========================================================================//

#include <core/stdafx.h>
#include <tier1/NetAdr2.h>
#ifndef NETCONSOLE
#include <engine/sys_utils.h> // !! IMPLEMENT 'Warning(..)' !!
#endif // !NETCONSOLE

//-----------------------------------------------------------------------------
// Purpose: constructor (use this when string contains <[IP]:PORT> or 'loopback'/'localhost').
// Input  : svInAdr - 
//-----------------------------------------------------------------------------
CNetAdr2::CNetAdr2(std::string svInAdr)
{
	SetType(netadrtype_t::NA_IP);
	if (strcmp(svInAdr.c_str(), "loopback") == 0 || strcmp(svInAdr.c_str(), "::1") == 0)
	{
		SetType(netadrtype_t::NA_LOOPBACK);
		svInAdr = "[127.0.0.1" + GetPort(svInAdr);
	}
	else if (strcmp(svInAdr.c_str(), "localhost"))
	{
		svInAdr = "[127.0.0.1" + GetPort(svInAdr);
	}

	// [IP]:PORT
	m_svip = GetBase(svInAdr);
	SetVersion();

	if (GetVersion() == netadrversion_t::NA_V4)
	{
		reinterpret_cast<sockaddr_in*>(&m_sadr)->sin_port = htons(stoi(GetPort()));
	}
	else if (GetVersion() == netadrversion_t::NA_V6)
	{
		reinterpret_cast<sockaddr_in6*>(&m_sadr)->sin6_port = htons(stoi(GetPort()));
	}
}

//-----------------------------------------------------------------------------
// Purpose: constructor (expects string format <IPv4/IPv6> <PORT>).
// Input  : svInAdr - 
//			svInPort - 
//-----------------------------------------------------------------------------
CNetAdr2::CNetAdr2(std::string svInAdr, std::string svInPort)
{
	SetType(netadrtype_t::NA_IP);

	if (strcmp(svInAdr.c_str(), "loopback") == 0 || strcmp(svInAdr.c_str(), "::1") == 0)
	{
		SetType(netadrtype_t::NA_LOOPBACK);
	}
	else if (strcmp(svInAdr.c_str(), "localhost") == 0)
	{
		svInAdr = "127.0.0.1";
	}

	if (strstr(svInAdr.c_str(), "["))
	{
		svInAdr = GetBase(svInAdr);
	}

	SetIPAndPort(svInAdr, svInPort);

	if (m_version == netadrversion_t::NA_V4)
	{
		reinterpret_cast<sockaddr_in*>(&m_sadr)->sin_port = htons(stoi(GetPort()));
	}
	else if (m_version == netadrversion_t::NA_V6)
	{
		reinterpret_cast<sockaddr_in6*>(&m_sadr)->sin6_port = htons(stoi(GetPort()));
	}
}

//-----------------------------------------------------------------------------
// Purpose: destructor.
//-----------------------------------------------------------------------------
CNetAdr2::~CNetAdr2(void)
{
	Clear();
}

//-----------------------------------------------------------------------------
// Purpose: sets the IP address.
// Input  : svInAdr - 
//-----------------------------------------------------------------------------
void CNetAdr2::SetIP(const std::string& svInAdr)
{
	m_svip = "[" + svInAdr + "]";
}

//-----------------------------------------------------------------------------
// Purpose: sets the port.
// Input  : svInPort - 
//-----------------------------------------------------------------------------
void CNetAdr2::SetPort(const std::string& svInPort)
{
	m_svip += ":" + svInPort;
}

//-----------------------------------------------------------------------------
// Purpose: sets the IP address and port.
// Input  : svInAdr - 
//			svInPort - 
//-----------------------------------------------------------------------------
void CNetAdr2::SetIPAndPort(const std::string& svInAdr, const std::string& svInPort)
{
	m_svip = "[" + svInAdr + "]:" + svInPort;
	SetVersion();
}

//-----------------------------------------------------------------------------
// Purpose: sets the type.
// Input  : type - 
//-----------------------------------------------------------------------------
void CNetAdr2::SetType(const netadrtype_t& type)
{
	m_type = type;
}

//-----------------------------------------------------------------------------
// Purpose: sets the IP version (IPv4/IPv6/INVALID) based on input.
//-----------------------------------------------------------------------------
void CNetAdr2::SetVersion(void)
{
	if (inet_pton(reinterpret_cast<sockaddr_in*>(&m_sadr)->sin_family, 
		GetBase().c_str(), &reinterpret_cast<sockaddr_in*>(m_sadr)->sin_addr) &&
		!strstr(GetBase().c_str(), "::"))
	{
		m_version = netadrversion_t::NA_V4;
		return;
	}
	else if (inet_pton(reinterpret_cast<sockaddr_in6*>(&m_sadr)->sin6_family, 
		GetBase().c_str(), &reinterpret_cast<sockaddr_in6*>(m_sadr)->sin6_addr))
	{
		m_version = netadrversion_t::NA_V6;
		return;
	}
	m_version = netadrversion_t::NA_INVALID;
}

//-----------------------------------------------------------------------------
// Purpose: sets IP address and port from sockaddr struct.
// Input  : hSocket - 
//-----------------------------------------------------------------------------
void CNetAdr2::SetFromSocket(const int& hSocket)
{
	Clear();
	m_type = netadrtype_t::NA_IP;

	sockaddr_storage address{};
	socklen_t namelen = sizeof(address);
	if (getsockname(hSocket, (sockaddr*)&address, &namelen) == 0)
	{
		SetFromSockadr(&address);
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets fields based on 'sockaddr' input.
// Input  : *s - 
//-----------------------------------------------------------------------------
bool CNetAdr2::SetFromSockadr(sockaddr_storage* s)
{
	if (reinterpret_cast<sockaddr_in*>(s)->sin_family == AF_INET)
	{
		char szAdrv4[INET_ADDRSTRLEN]{};
		sockaddr_in* pAdrv4 = reinterpret_cast<sockaddr_in*>(s);

		inet_ntop(pAdrv4->sin_family, &pAdrv4->sin_addr, szAdrv4, sizeof(sockaddr_in));
		SetIPAndPort(szAdrv4, std::to_string(ntohs(pAdrv4->sin_port)));
		return true;
	}
	else if (reinterpret_cast<sockaddr_in6*>(s)->sin6_family == AF_INET6)
	{
		char szAdrv6[INET6_ADDRSTRLEN]{};
		sockaddr_in6* pAdrv6 = reinterpret_cast<sockaddr_in6*>(s);

		inet_ntop(pAdrv6->sin6_family, &pAdrv6->sin6_addr, szAdrv6, sizeof(sockaddr_in6));
		SetIPAndPort(szAdrv6, std::to_string(ntohs(pAdrv6->sin6_port)));
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: removes brackets and port from IP address.
//-----------------------------------------------------------------------------
std::string CNetAdr2::GetBase(void) const
{
	std::string svIpAdr = m_svip;
	static std::regex rx("\\].*");
	svIpAdr.erase(0, 1);
	svIpAdr = std::regex_replace(svIpAdr, rx, "");

	return svIpAdr;
}

//-----------------------------------------------------------------------------
// Purpose: removes brackets and port from IP address.
// Input  : svInAdr - 
//-----------------------------------------------------------------------------
std::string CNetAdr2::GetBase(std::string svInAdr) const
{
	static std::regex rx("\\].*");
	svInAdr.erase(0, 1);
	svInAdr = std::regex_replace(svInAdr, rx, "");

	return svInAdr;
}

//-----------------------------------------------------------------------------
// Purpose: gets the IP address.
// Input  : bBaseOnly - 
//-----------------------------------------------------------------------------
std::string CNetAdr2::GetIP(bool bBaseOnly) const
{
	if (GetType() == netadrtype_t::NA_LOOPBACK)
	{
		return "loopback";
	}
	else if (GetType() == netadrtype_t::NA_BROADCAST)
	{
		return "broadcast";
	}
	else if (GetType() == netadrtype_t::NA_IP)
	{
		if (bBaseOnly)
		{
			return GetBase();
		}
		else
		{
			return m_svip;
		}
	}
	else
	{
		return "unknown";
	}
}

//-----------------------------------------------------------------------------
// Purpose: removes brackets and port from IP address.
//-----------------------------------------------------------------------------
std::string CNetAdr2::GetPort(void) const
{
	std::string svport = m_svip;
	static std::regex rx(".*\\]:");
	svport = std::regex_replace(svport, rx, "");

	return svport;
}
std::string CNetAdr2::GetPort(std::string svInPort) const
{
	static std::regex rx(".*\\]:");
	svInPort = std::regex_replace(svInPort, rx, "");

	return svInPort;
}

//-----------------------------------------------------------------------------
// Purpose: returns the IP address and port.
//-----------------------------------------------------------------------------
std::string CNetAdr2::GetIPAndPort(void) const
{
	return m_svip;
}

//-----------------------------------------------------------------------------
// Purpose: returns the address type.
//-----------------------------------------------------------------------------
netadrtype_t CNetAdr2::GetType(void) const
{
	return m_type;
}

//-----------------------------------------------------------------------------
// Purpose: returns the IP version.
//-----------------------------------------------------------------------------
netadrversion_t CNetAdr2::GetVersion(void) const
{
	return m_version;
}

//-----------------------------------------------------------------------------
// Purpose: splits IP address into parts by their delimiters.
// Output : string vector containing IP parts.
//-----------------------------------------------------------------------------
std::vector<std::string> CNetAdr2::GetParts(void) const
{
	std::vector<std::string> results;

	// Make sure we have a valid address.
	if (m_version == netadrversion_t::NA_INVALID || m_type != netadrtype_t::NA_IP)
	{
		assert(m_version == netadrversion_t::NA_INVALID && "Invalid IP address for 'GetParts()'.");
		return results;
	}

	std::string svIpAdr = m_svip, svDelim;
	std::string::size_type prev_pos = 0, curr_pos = 0;

	// 000.000.000.000 -> vparts.
	if (m_version == netadrversion_t::NA_V4)
	{
		svDelim = ".";
	}
	// 0000:0000:0000:0000:0000:0000:0000:0000 -> vparts.
	else if (m_version == netadrversion_t::NA_V6)
	{
		svDelim = ":";
		StringReplace(svIpAdr, "::", ":");
	}

	while ((curr_pos = svIpAdr.find(svDelim, curr_pos)) != std::string::npos)
	{
		std::string substr(svIpAdr.substr(prev_pos, curr_pos - prev_pos));

		results.push_back(substr);
		prev_pos = ++curr_pos;
	}
	results.push_back(m_svip.substr(prev_pos, curr_pos - prev_pos));

	return results;
}

//-----------------------------------------------------------------------------
// Purpose: returns the size of the network family struct.
//-----------------------------------------------------------------------------
int CNetAdr2::GetSize(void) const
{
	if (GetVersion() == netadrversion_t::NA_V4)
	{
		return sizeof(sockaddr_in);
	}
	else if (GetVersion() == netadrversion_t::NA_V6)
	{
		return sizeof(sockaddr_in6);
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: returns the network family version.
//-----------------------------------------------------------------------------
int CNetAdr2::GetFamily(void) const
{
	if (GetVersion() == netadrversion_t::NA_V4)
	{
		return AF_INET;
	}
	else if (GetVersion() == netadrversion_t::NA_V6)
	{
		return AF_INET6;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: sets fields from 'sockaddr'.
// Input  : *pSadr - 
//-----------------------------------------------------------------------------
void CNetAdr2::ToSockadr(sockaddr_storage* pSadr) const
{
	if (GetVersion() == netadrversion_t::NA_V4)
	{
		if (GetType() == netadrtype_t::NA_BROADCAST)
		{
			reinterpret_cast<sockaddr_in*>(pSadr)->sin_family = AF_INET;
			reinterpret_cast<sockaddr_in*>(pSadr)->sin_port = htons(stoi(GetPort()));
			reinterpret_cast<sockaddr_in*>(pSadr)->sin_addr.s_addr = INADDR_BROADCAST;
		}
		else if (GetType() == netadrtype_t::NA_IP)
		{
			reinterpret_cast<sockaddr_in*>(pSadr)->sin_family = AF_INET;
			reinterpret_cast<sockaddr_in*>(pSadr)->sin_port = htons(stoi(GetPort()));;
			inet_pton(AF_INET, GetBase().c_str(), &reinterpret_cast<sockaddr_in*>(pSadr)->sin_addr.s_addr);
		}
		else if (GetType() == netadrtype_t::NA_LOOPBACK)
		{
			reinterpret_cast<sockaddr_in*>(pSadr)->sin_family = AF_INET;
			reinterpret_cast<sockaddr_in*>(pSadr)->sin_port = htons(stoi(GetPort()));;
			reinterpret_cast<sockaddr_in*>(pSadr)->sin_addr.s_addr = INADDR_LOOPBACK;
		}
	}
	else if (GetVersion() == netadrversion_t::NA_V6)
	{
		if (GetType() == netadrtype_t::NA_IP)
		{
			reinterpret_cast<sockaddr_in6*>(pSadr)->sin6_family = AF_INET6;
			reinterpret_cast<sockaddr_in6*>(pSadr)->sin6_port = htons(stoi(GetPort()));;
			inet_pton(AF_INET6, GetBase().c_str(), &reinterpret_cast<sockaddr_in6*>(pSadr)->sin6_addr);
		}
		else if (GetType() == netadrtype_t::NA_LOOPBACK)
		{
			reinterpret_cast<sockaddr_in6*>(pSadr)->sin6_family = AF_INET6;
			reinterpret_cast<sockaddr_in6*>(pSadr)->sin6_port = htons(stoi(GetPort()));;
			reinterpret_cast<sockaddr_in6*>(pSadr)->sin6_addr = in6addr_loopback;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets fields from 'addrinfo'.
// Input  : *pHint - 
//-----------------------------------------------------------------------------
void CNetAdr2::ToAdrinfo(addrinfo* pHint) const
{
	int results{ };
	addrinfo hint{ }; // <-- TODO: Pass these instead.
	if (GetVersion() == netadrversion_t::NA_V4)
	{
		hint.ai_family = AF_INET;
		hint.ai_socktype = SOCK_STREAM;
		hint.ai_protocol = IPPROTO_TCP;
		hint.ai_flags = AI_PASSIVE;
		results = getaddrinfo(GetBase().c_str(), GetPort().c_str(), &hint, &pHint);
		if (results != 0)
		{
			// TODO: Implement 'Warning(..)' instead!
#ifndef NETCONSOLE
			DevMsg(eDLL_T::ENGINE, "Address info translation failed (%s)\n", gai_strerror(results));
#else
			printf("Address info translation failed (%s)\n", gai_strerror(results));
#endif // !NETCONSOLE
		}
	}
	else if (GetVersion() == netadrversion_t::NA_V6)
	{
		hint.ai_family = AF_INET6;
		hint.ai_socktype = SOCK_STREAM;
		hint.ai_protocol = IPPROTO_TCP;
		hint.ai_flags = AI_PASSIVE;
		results = getaddrinfo(GetBase().c_str(), GetPort().c_str(), &hint, &pHint);
		if (results != 0)
		{
			// TODO: Implement 'Warning(..)' instead!
#ifndef NETCONSOLE
			DevMsg(eDLL_T::ENGINE, "Address info translation failed (%s)\n", gai_strerror(results));
#else
			printf("Address info translation failed (%s)\n", gai_strerror(results));
#endif // !NETCONSOLE
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns true if we are localhost.
//-----------------------------------------------------------------------------
bool CNetAdr2::IsLocalhost(void) const
{
	return (strcmp(GetBase().c_str(), "127.0.0.1") == 0);
}

//-----------------------------------------------------------------------------
// Purpose: returns true if we use the loopback buffers.
//-----------------------------------------------------------------------------
bool CNetAdr2::IsLoopback(void) const
{
	return GetType() == netadrtype_t::NA_LOOPBACK;
}

//-----------------------------------------------------------------------------
// Purpose: check if address is reserved and not routable.
//-----------------------------------------------------------------------------
bool CNetAdr2::IsReservedAdr(void) const
{
	if (GetType() == netadrtype_t::NA_LOOPBACK)
	{
		return true;
	}

	if (GetType() == netadrtype_t::NA_IP)
	{
		std::vector<std::string> ip_parts = GetParts();

		int n0 = stoi(ip_parts[0]);
		int n1 = stoi(ip_parts[1]);

		if ((n0 == 10)                          || // 10.x.x.x is reserved
			(n0 == 127)                         || // 127.x.x.x 
			(n0 == 172 && n1 >= 16 && n1 <= 31) || // 172.16.x.x - 172.31.x.x
			(n0 == 192 && n1 >= 168))              // 192.168.x.x
		{
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: compares IP for equality (IPv4/IPv6).
// Input  : *netAdr2 - 
//			bBaseOnly - 
// Output : true if equal, false otherwise.
//-----------------------------------------------------------------------------
bool CNetAdr2::CompareAdr(const CNetAdr2& netAdr2, bool bBaseOnly) const
{
	if (netAdr2.GetType() != GetType())
	{
		return false;
	}

	if (GetType() == netadrtype_t::NA_LOOPBACK)
	{
		return true;
	}

	if (GetType() == netadrtype_t::NA_BROADCAST)
	{
		return true;
	}

	if (GetType() == netadrtype_t::NA_IP)
	{
		if (!bBaseOnly && 
			(strcmp(netAdr2.GetPort().c_str(), GetPort().c_str()) != 0))
		{
			return false;
		}

		if (strcmp(netAdr2.GetBase().c_str(), GetBase().c_str()) == 0)
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: compares Class-B IP for equality.
// Input  : *netAdr2 - 
// Output : true if equal, false otherwise.
//-----------------------------------------------------------------------------
bool CNetAdr2::CompareClassBAdr(const CNetAdr2& netAdr2) const
{
	if (netAdr2.m_version != netadrversion_t::NA_V4 && m_version != netadrversion_t::NA_V4)
	{
		return false;
	}

	if (netAdr2.GetType() != GetType())
	{
		return false;
	}

	if (GetType() == netadrtype_t::NA_LOOPBACK)
	{
		return true;
	}

	if (GetType() == netadrtype_t::NA_IP)
	{
		std::vector<std::string> v0 = netAdr2.GetParts();
		std::vector<std::string> v1 = GetParts();

		if (strcmp(v0[0].c_str(), v1[0].c_str()) == 0 && 
			strcmp(v0[1].c_str(), v1[1].c_str()) == 0)
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: compares Class-C IP for equality.
// Input  : *netAdr2 - 
// Output : true if equal, false otherwise.
//-----------------------------------------------------------------------------
bool CNetAdr2::CompareClassCAdr(const CNetAdr2& netAdr2) const
{
	if (netAdr2.GetVersion() != netadrversion_t::NA_V4 && GetVersion() != netadrversion_t::NA_V4)
	{
		return false;
	}

	if (netAdr2.GetType() != GetType())
	{
		return false;
	}

	if (GetType() == netadrtype_t::NA_LOOPBACK)
	{
		return true;
	}

	if (GetType() == netadrtype_t::NA_IP)
	{
		std::vector<std::string> v0 = netAdr2.GetParts();
		std::vector<std::string> v1 = GetParts();

		if (strcmp(v0[0].c_str(), v1[0].c_str()) == 0 && 
			strcmp(v0[1].c_str(), v1[1].c_str()) == 0 && 
			strcmp(v0[2].c_str(), v1[2].c_str()) == 0)
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: clears IP address.
//-----------------------------------------------------------------------------
void CNetAdr2::Clear(void)
{
	m_svip.clear();
	m_type    = netadrtype_t::NA_NULL;
	m_version = netadrversion_t::NA_INVALID;
	m_sadr    = {};
}
