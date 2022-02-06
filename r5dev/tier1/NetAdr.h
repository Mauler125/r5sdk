#pragma once

enum class netadrtype_t
{
	NA_NULL = 0,
	NA_LOOPBACK,
	NA_BROADCAST,
	NA_IP,
};

typedef struct netadr_s netadr_t;
typedef struct netadr_s
{
public:
	netadr_s(void);
	netadr_s(std::uint32_t unIP, std::uint16_t usPort);
	netadr_s(const char* pch);
	void	Clear(void);

	void	SetType(netadrtype_t type);
	void	SetPort(std::uint16_t port);
	bool	SetFromSockadr(const struct sockaddr* s);
	void	SetIP(std::uint8_t b1, std::uint8_t b2, std::uint8_t b3, std::uint8_t b4);
	void	SetIP(std::uint32_t unIP);
	void	SetIPAndPort(std::uint32_t unIP, std::uint16_t usPort) { SetIP(unIP); SetPort(usPort); }
	bool	SetFromString(const char* pch, bool bUseDNS = false);
	void	SetFromSocket(int hSocket);

	bool	CompareAdr(const netadr_s& a, bool onlyBase = false) const;
	bool	CompareClassBAdr(const netadr_s& a) const;
	bool	CompareClassCAdr(const netadr_s& a) const;

	netadrtype_t    GetType(void) const;
	std::uint16_t   GetPort(void) const;

	const char* ToString(bool onlyBase = false) const;

	void	ToString(char* pchBuffer, std::uint32_t unBufferSize, bool onlyBase = false) const;
	template< size_t maxLenInChars >
	void	ToString_safe(char(&pDest)[maxLenInChars], bool onlyBase = false) const
	{
		ToString(&pDest[0], maxLenInChars, onlyBase);
	}

	//[xxxx::xxxx:xxxx:xxxx:xxxx]:00000

	void ToSockadr(struct sockaddr* s) const;

	// Returns 0xAABBCCDD for AA.BB.CC.DD on all platforms, which is the same format used by SetIP().
	std::uint32_t	GetIPHostByteOrder(void) const;

	// Returns a number that depends on the platform.  In most cases, this probably should not be used.
	std::uint32_t	GetIPNetworkByteOrder(void) const;

	bool	IsValid(void)        const;	// ip & port != 0
	bool	IsBaseAdrValid(void) const;	// ip != 0
	bool	IsLocalhost(void)    const;	// true, if this is the localhost IP 
	bool	IsLoopback(void)     const;	// true if engine loopback buffers are used
	bool	IsReservedAdr(void)  const;	// true, if this is a private LAN IP

	bool operator==(const netadr_s& netadr) const { return (CompareAdr(netadr)); }
	bool operator!=(const netadr_s& netadr) const { return !(CompareAdr(netadr)); }
	bool operator<(const netadr_s& netadr) const;

public:	// members are public to avoid to much changes
	netadrtype_t    type;
	std::uint8_t    ip[4];
	std::uint16_t   port;
} netadr_t;
