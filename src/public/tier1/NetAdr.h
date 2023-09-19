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
	CNetAdr(void)            { Clear(); }
	CNetAdr(const char* pch) { SetFromString(pch); }
	void	Clear(void);

	inline void	SetIP(IN6_ADDR* inAdr)        { adr = *inAdr; }
	inline void	SetPort(uint16_t newport)     { port = newport; }
	inline void	SetType(netadrtype_t newtype) { type = newtype; }

	bool	SetFromSockadr(struct sockaddr_storage* s);
	bool	SetFromString(const char* pch, bool bUseDNS = false);

	inline netadrtype_t	GetType(void) const { return type; }
	inline uint16_t		GetPort(void) const { return port; }

	bool		CompareAdr(const CNetAdr& other) const;
	inline bool	ComparePort(const CNetAdr& other) const { return port == other.port; }
	inline bool	IsLoopback(void) const { return type == netadrtype_t::NA_LOOPBACK; } // true if engine loopback buffers are used.

	const char*	ToString(bool onlyBase = false) const;
	void		ToString(char* pchBuffer, size_t unBufferSize, bool onlyBase = false) const;
	void		ToAdrinfo(addrinfo* pHint) const;
	void		ToSockadr(struct sockaddr_storage* s) const;

private:
	netadrtype_t type;
	IN6_ADDR adr;
	uint16_t port;
	bool field_16;
	bool reliable;
};

typedef class CNetAdr netadr_t;

#endif // TIER1_NETADR_H
