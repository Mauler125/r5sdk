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
// Purpose: constructor (use this when string contains <[IP]:PORT>).
// Input  : svInAdr - 
//-----------------------------------------------------------------------------
CNetAdr2::CNetAdr2(const string& svInAdr)
{
	SetIPAndPort(svInAdr);
}

//-----------------------------------------------------------------------------
// Purpose: constructor (expects string format <IPv4/IPv6> <PORT>).
// Input  : &svInAdr - 
//			&svInPort - 
//-----------------------------------------------------------------------------
CNetAdr2::CNetAdr2(const string& svInAdr, const string& svInPort)
{
	SetIPAndPort(svInAdr, svInPort);
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
// Input  : *svInAdr - 
//-----------------------------------------------------------------------------
void CNetAdr2::SetIP(const string& svInAdr)
{
	m_svip = '[' + svInAdr + ']';
}

//-----------------------------------------------------------------------------
// Purpose: sets the port.
// Input  : *svInPort - 
//-----------------------------------------------------------------------------
void CNetAdr2::SetPort(const string& svInPort)
{
	m_svip += ':' + svInPort;
}

//-----------------------------------------------------------------------------
// Purpose: sets the IP address and port.
// Input  : *svInAdr - 
//-----------------------------------------------------------------------------
void CNetAdr2::SetIPAndPort(string svInAdr)
{
	SetType(netadrtype_t::NA_IP);
	if (svInAdr.find("loopback") != string::npos || svInAdr.find("::1") != string::npos)
	{
		SetType(netadrtype_t::NA_LOOPBACK);
		svInAdr = "[127.0.0.1]:" + GetPort(svInAdr);
	}
	else if (svInAdr.find("localhost") != string::npos)
	{
		svInAdr = "[127.0.0.1]:" + GetPort(svInAdr);
	}
	// [IP]:PORT
	m_svip = svInAdr;
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
// Purpose: sets the IP address and port.
// Input  : *svInAdr - 
//			*svInPort - 
//-----------------------------------------------------------------------------
void CNetAdr2::SetIPAndPort(string svInAdr, string svInPort)
{
	SetType(netadrtype_t::NA_IP);
	if (svInAdr.compare("loopback") == 0 || svInAdr.compare("::1") == 0)
	{
		SetType(netadrtype_t::NA_LOOPBACK);
	}
	else if (svInAdr.compare("localhost") == 0)
	{
		svInAdr = "127.0.0.1";
	}

	if (svInAdr.find("[") != string::npos || svInAdr.find("]") != string::npos)
	{
		svInAdr = GetBase(svInAdr);
	}

	m_svip = "[" + svInAdr + "]:" + svInPort;
	SetVersion();

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
// Purpose: sets the type.
// Input  : *type - 
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
string CNetAdr2::GetBase(void) const
{
	static std::regex rx("[^\\[]*.(.*)(\\]).*");
	std::smatch smRegexMatches;
	std::regex_search(m_svip, smRegexMatches, rx);

	if (smRegexMatches.size() > 0)
	{
		return smRegexMatches[1].str();
	}
	else
	{
		return "127.0.0.1";
	}
}

//-----------------------------------------------------------------------------
// Purpose: removes brackets and port from IP address.
// Input  : svInAdr - 
//-----------------------------------------------------------------------------
string CNetAdr2::GetBase(const string& svInAdr) const
{
	static std::regex rx("[^\\[]*.(.*)(\\]).*");
	std::smatch smRegexMatches;
	std::regex_search(svInAdr, smRegexMatches, rx);

	if (smRegexMatches.size() > 0)
	{
		return smRegexMatches[1].str();
	}
	else
	{
		return "127.0.0.1";
	}
}

//-----------------------------------------------------------------------------
// Purpose: gets the IP address.
// Input  : bBaseOnly - 
//-----------------------------------------------------------------------------
string CNetAdr2::GetIP(bool bBaseOnly = false) const
{
	if (GetType() == netadrtype_t::NA_LOOPBACK)
	{
		return "loopback";
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
// Purpose: removes brackets and IP address from port.
//-----------------------------------------------------------------------------
string CNetAdr2::GetPort(void) const
{
	string svport = m_svip;
	static std::regex rx(".*\\]:");
	svport = std::regex_replace(svport, rx, "");

	if (!IsValidPort(svport))
	{
		return "37015";
	}
	return svport;
}

//-----------------------------------------------------------------------------
// Purpose: removes brackets and IP address from port.
// Input  : svInPort - 
//-----------------------------------------------------------------------------
string CNetAdr2::GetPort(string svInPort) const
{
	static std::regex rx(".*\\]:");
	svInPort = std::regex_replace(svInPort, rx, "");

	if (!IsValidPort(svInPort))
	{
		return "37015";
	}
	return svInPort;
}

//-----------------------------------------------------------------------------
// Purpose: returns the IP address and port.
//-----------------------------------------------------------------------------
string CNetAdr2::GetIPAndPort(void) const
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
vector<string> CNetAdr2::GetParts(void) const
{
	vector<string> results;

	// Make sure we have a valid address.
	if (m_version == netadrversion_t::NA_INVALID || m_type != netadrtype_t::NA_IP)
	{
		assert(m_version == netadrversion_t::NA_INVALID && "Invalid IP address for 'GetParts()'.");
		return results;
	}

	string svIpAdr = m_svip, svDelim;
	string::size_type prev_pos = 0, curr_pos = 0;

	// 000.000.000.000 -> vparts.
	if (m_version == netadrversion_t::NA_V4)
	{
		svDelim = '.';
	}
	// 0000:0000:0000:0000:0000:0000:0000:0000 -> vparts.
	else if (m_version == netadrversion_t::NA_V6)
	{
		svDelim = ':';
		StringReplace(svIpAdr, "::", ":");
	}

	while ((curr_pos = svIpAdr.find(svDelim, curr_pos)) != string::npos)
	{
		string substr(svIpAdr.substr(prev_pos, curr_pos - prev_pos));

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
		if (GetType() == netadrtype_t::NA_IP)
		{
			reinterpret_cast<sockaddr_in*>(pSadr)->sin_family = AF_INET;
			reinterpret_cast<sockaddr_in*>(pSadr)->sin_port = htons(stoi(GetPort()));
			inet_pton(AF_INET, GetBase().c_str(), &reinterpret_cast<sockaddr_in*>(pSadr)->sin_addr.s_addr);
		}
		else if (GetType() == netadrtype_t::NA_LOOPBACK)
		{
			reinterpret_cast<sockaddr_in*>(pSadr)->sin_family = AF_INET;
			reinterpret_cast<sockaddr_in*>(pSadr)->sin_port = htons(stoi(GetPort()));
			reinterpret_cast<sockaddr_in*>(pSadr)->sin_addr.s_addr = INADDR_LOOPBACK;
		}
	}
	else if (GetVersion() == netadrversion_t::NA_V6)
	{
		if (GetType() == netadrtype_t::NA_IP)
		{
			reinterpret_cast<sockaddr_in6*>(pSadr)->sin6_family = AF_INET6;
			reinterpret_cast<sockaddr_in6*>(pSadr)->sin6_port = htons(stoi(GetPort()));
			inet_pton(AF_INET6, GetBase().c_str(), &reinterpret_cast<sockaddr_in6*>(pSadr)->sin6_addr);
		}
		else if (GetType() == netadrtype_t::NA_LOOPBACK)
		{
			reinterpret_cast<sockaddr_in6*>(pSadr)->sin6_family = AF_INET6;
			reinterpret_cast<sockaddr_in6*>(pSadr)->sin6_port = htons(stoi(GetPort()));
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
		hint.ai_flags    = AI_PASSIVE;
		hint.ai_family   = AF_INET;
		hint.ai_socktype = SOCK_STREAM;
		hint.ai_protocol = IPPROTO_TCP;

		results = getaddrinfo(GetBase().c_str(), GetPort().c_str(), &hint, &pHint);
		if (results != 0)
		{
			WCHAR* wszError = gai_strerror(results);
			_bstr_t bStr(wszError);
			const char* pszError = bStr;
#ifndef NETCONSOLE
			Warning(eDLL_T::ENGINE, "Address info translation failed (%s)\n", pszError);
#else
			printf("Address info translation failed (%s)\n", pszError);
#endif // !NETCONSOLE
		}
	}
	else if (GetVersion() == netadrversion_t::NA_V6)
	{
		hint.ai_flags    = AI_PASSIVE;
		hint.ai_family   = AF_INET6;
		hint.ai_socktype = SOCK_STREAM;
		hint.ai_protocol = IPPROTO_TCP;

		results = getaddrinfo(GetBase().c_str(), GetPort().c_str(), &hint, &pHint);
		if (results != 0)
		{
			WCHAR* wszError = gai_strerror(results);
			_bstr_t bStr(wszError);
			const char* pszError = bStr;
#ifndef NETCONSOLE
			Warning(eDLL_T::ENGINE, "Address info translation failed (%s)\n", pszError);
#else
			printf("Address info translation failed (%s)\n", pszError);
#endif // !NETCONSOLE
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns true if this is a valid port string.
//-----------------------------------------------------------------------------
bool CNetAdr2::IsValidPort(const string& svInPort) const
{
	for (char const& c : svInPort)
	{
		if (std::isdigit(c) == 0)
		{
			return false;
		}
	}
	return true;
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
		vector<string> ip_parts = GetParts();

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
		vector<string> v0 = netAdr2.GetParts();
		vector<string> v1 = GetParts();

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
		vector<string> v0 = netAdr2.GetParts();
		vector<string> v1 = GetParts();

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
