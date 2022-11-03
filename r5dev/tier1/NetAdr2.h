#pragma once
#include "tier1/bitbuf.h"

enum class netadrtype_t
{
	NA_NULL = 0,
	NA_LOOPBACK,
	NA_IP,
};

enum class netadrversion_t
{
	NA_INVALID = -1,
	NA_V4 = 4,
	NA_V6 = 6,
};

class CNetAdr2
{
public:
	CNetAdr2(void) {};
	CNetAdr2(const string& svInAdr);
	CNetAdr2(const string& svInAdr, const string& svInPort);
	~CNetAdr2(void);

	void SetIP(const string& svInAdr);
	void SetPort(const string& svInPort);
	void SetIPAndPort(string svInAdr);
	void SetIPAndPort(string svInAdr, string svInPort);
	void SetType(const netadrtype_t& type);
	void SetVersion(void);
	void SetFromSocket(const int& hSocket);
	bool SetFromSockadr(sockaddr_storage* s);

	string GetIP(bool bBaseOnly) const;
	string GetPort(void) const;
	string GetPort(string svInPort) const;
	string GetIPAndPort(void) const;
	netadrtype_t GetType(void) const;
	netadrversion_t GetVersion(void) const;
	string GetBase(void) const;
	string GetBase(const string& svInAdr) const;
	vector<string> GetParts(void) const;
	int GetSize(void) const;
	int GetFamily(void) const;

	void ToSockadr(sockaddr_storage* pSadr) const;
	void ToAdrinfo(addrinfo* pHint) const;

	bool IsValidPort(const string& svInPort) const;
	bool IsLocalhost(void) const;
	bool IsLoopback(void) const;
	bool IsReservedAdr(void) const;

	bool CompareAdr(const CNetAdr2& adr2, bool bBaseOnly) const;
	bool CompareClassBAdr(const CNetAdr2& adr2) const;
	bool CompareClassCAdr(const CNetAdr2& adr2) const;

	void Clear(void);

private:
	string            m_svip;
	netadrtype_t      m_type;
	netadrversion_t   m_version;
	sockaddr_storage* m_sadr;
};

class v_netadr_t // !TODO: Move this to 'NetAdr.h' instead and adjust existing class to new system.
{
public:
	inline netadrtype_t GetType(void) const
	{
		return this->type;
	}
	inline void GetAddress(char* pchBuffer, uint32_t nBufferLen) const
	{
		assert(nBufferLen >= INET6_ADDRSTRLEN);
		inet_ntop(AF_INET6, &this->adr, pchBuffer, nBufferLen);
	}
	inline uint16_t GetPort(void) const
	{
		return this->port;
	}
	inline bool IsReliable(void) const
	{
		return this->reliable;
	}
	netadrtype_t type{};
	IN6_ADDR     adr{};
	uint16_t     port{};
	bool         field_16{};
	bool         reliable{};
};
