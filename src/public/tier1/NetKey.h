#pragma once
#include "NetAdr.h"

class CNetKey
{
public:
	const char* GetBase64NetKey(void) const;

private:
	netadr_t m_Adr;
	char m_Pad0[0x18];
	char m_UnkData0[0xFF0];
	char m_Pad1[0x40];
	char m_UnkData1[0xC0];
	char m_Pad2[0x160];
	LPCRITICAL_SECTION m_Mutex;
	char m_Pad3[0x20];
	bool m_bUnknown;
	char m_RandomUnknown[0x23];
	int m_nSize;
	char m_szBase64[0x2D];
};
static_assert(sizeof(CNetKey) == 0x1300);

typedef class CNetKey netkey_t;