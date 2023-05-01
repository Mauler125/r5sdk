#ifndef TIER1_NETADR_H
#define TIER1_NETADR_H

#define NET_IPV4_UNSPEC "0.0.0.0"
#define NET_IPV6_UNSPEC "::"
#define NET_IPV6_LOOPBACK "::1"

enum class netadrtype_t
{
	NA_NULL = 0,
	NA_LOOPBACK,
	NA_IP,
};

class CNetAdr
{
public:
	CNetAdr(void);
	CNetAdr(const char* pch);
	void	Clear(void);

	void	SetIP(IN6_ADDR* inAdr);
	void	SetPort(uint16_t port);
	void	SetType(netadrtype_t type);

	bool	SetFromSockadr(struct sockaddr_storage* s);
	bool	SetFromString(const char* pch, bool bUseDNS = false);

	netadrtype_t	GetType(void) const;
	uint16_t		GetPort(void) const;

	bool	CompareAdr(const CNetAdr& other) const;
	bool	ComparePort(const CNetAdr& other) const;

	const char*		ToString(bool onlyBase = false) const;
	void	ToString(char* pchBuffer, size_t unBufferSize, bool onlyBase = false) const;
	void	ToAdrinfo(addrinfo* pHint) const;

	void	ToSockadr(struct sockaddr_storage* s) const;
	bool	IsLoopback(void) const; // true if engine loopback buffers are used.

private:
	netadrtype_t type;
	IN6_ADDR adr;
	uint16_t port;
	bool field_16;
	bool reliable;
};

typedef class CNetAdr netadr_t;

#endif // TIER1_NETADR_H
