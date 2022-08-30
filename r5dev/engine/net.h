#pragma once

#ifndef NETCONSOLE
#include "engine/net_chan.h"
#define MAX_STREAMS         2
#define FRAGMENT_BITS       8
#define FRAGMENT_SIZE       (1<<FRAGMENT_BITS)

// user message
#define MAX_USER_MSG_DATA 511 // <-- 255 in Valve Source.

#define NETMSG_TYPE_BITS	8	// must be 2^NETMSG_TYPE_BITS > SVC_LASTMSG (6 in Valve Source).
#define NETMSG_LENGTH_BITS	12	// 512 bytes (11 in Valve Source, 256 bytes).
#define NET_MIN_MESSAGE 5 // Even connectionless packets require int32 value (-1) + 1 byte content

constexpr unsigned int AES_128_KEY_SIZE = 16;
constexpr const char* DEFAULT_NET_ENCRYPTION_KEY = "WDNWLmJYQ2ZlM0VoTid3Yg==";

/* ==== CNETCHAN ======================================================================================================================================================== */
inline CMemory p_NET_Init;
inline auto v_NET_Init = p_NET_Init.RCast<void* (*)(bool bDeveloper)>();

inline CMemory p_NET_Shutdown;
inline auto v_NET_Shutdown = p_NET_Shutdown.RCast<void (*)(void* thisptr, const char* szReason, uint8_t bBadRep, bool bRemoveNow)>();

inline CMemory p_NET_SetKey;
inline auto v_NET_SetKey = p_NET_SetKey.RCast<void (*)(uintptr_t pKey, const char* szHash)>();

inline CMemory p_NET_ReceiveDatagram;
inline auto v_NET_ReceiveDatagram = p_NET_ReceiveDatagram.RCast<bool (*)(int iSocket, netpacket_s* pInpacket, bool bRaw)>();

inline CMemory p_NET_SendDatagram;
inline auto v_NET_SendDatagram = p_NET_SendDatagram.RCast<int (*)(SOCKET s, void* pPayload, int iLenght, v_netadr_t* pAdr, bool bEncrypted)>();

inline CMemory p_NET_PrintFunc;
inline auto v_NET_PrintFunc = p_NET_PrintFunc.RCast<void(*)(const char* fmt)>();

///////////////////////////////////////////////////////////////////////////////
bool NET_ReceiveDatagram(int iSocket, netpacket_s* pInpacket, bool bRaw);
int  NET_SendDatagram(SOCKET s, void* pPayload, int iLenght, v_netadr_t* pAdr, bool bEncrypted);
void NET_SetKey(const string& svNetKey);
void NET_GenerateKey();
void NET_PrintFunc(const char* fmt, ...);
void NET_Shutdown(void* thisptr, const char* szReason, uint8_t bBadRep, bool bRemoveNow);
void NET_DisconnectClient(CClient* pClient, int nIndex, const char* szReason, uint8_t bBadRep, bool bRemoveNow);

void NET_Attach();
void NET_Detach();

///////////////////////////////////////////////////////////////////////////////
extern string g_svNetKey;
extern uintptr_t g_pNetKey;
inline std::mutex g_NetKeyMutex;

///////////////////////////////////////////////////////////////////////////////
class VNet : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: NET_Init                             : {:#18x} |\n", p_NET_Init.GetPtr());
		spdlog::debug("| FUN: NET_Shutdown                         : {:#18x} |\n", p_NET_Shutdown.GetPtr());
		spdlog::debug("| FUN: NET_SetKey                           : {:#18x} |\n", p_NET_SetKey.GetPtr());
		spdlog::debug("| FUN: NET_ReceiveDatagram                  : {:#18x} |\n", p_NET_ReceiveDatagram.GetPtr());
		spdlog::debug("| FUN: NET_SendDatagram                     : {:#18x} |\n", p_NET_SendDatagram.GetPtr());
		spdlog::debug("| FUN: NET_PrintFunc                        : {:#18x} |\n", p_NET_PrintFunc.GetPtr());
		spdlog::debug("| VAR: g_pNetKey                            : {:#18x} |\n", g_pNetKey);
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1) || defined (GAMEDLL_S2)
		p_NET_Init     = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x48\x89\x7C\x24\x20\x41\x54\x41\x56\x41\x57\x48\x81\xEC\xC0\x01\x00"), "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
		p_NET_Shutdown = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x6C\x24\x18\x56\x57\x41\x56\x48\x83\xEC\x30\x83\xB9\xD8"), "xxxxxxxxxxxxxxxx");
#elif defined (GAMEDLL_S3)
		p_NET_Init     = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x48\x89\x7C\x24\x20\x41\x54\x41\x56\x41\x57\x48\x81\xEC\xF0\x01\x00"), "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
		p_NET_Shutdown = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x6C\x24\x18\x56\x57\x41\x56\x48\x83\xEC\x30\x83\xB9\xD0"), "xxxxxxxxxxxxxxxx");
#endif
		p_NET_SetKey          = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x57\x48\x83\xEC\x20\x48\x8B\xF9\x41\xB8"), "xxxxxxxxxxxxxxxxxxxxxxxxx");
		p_NET_ReceiveDatagram = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x74\x24\x18\x48\x89\x7C\x24\x20\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\xAC\x24\x50\xEB"), "xxxxxxxxxxxxxxxxxxxxxxxxx");
		p_NET_SendDatagram    = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x57\x41\x56\x41\x57\x48\x81\xEC\x00\x05\x00\x00"), "xxxxxxxxxxxxxxxxxxxxxxx?xxx");
		p_NET_PrintFunc       = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x54\x24\x10\x4C\x89\x44\x24\x18\x4C\x89\x4C\x24\x20\xC3\x48"), "xxxxxxxxxxxxxxxxx");

		v_NET_Init            = p_NET_Init.RCast<void* (*)(bool)>();                                        /*48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 48 89 7C 24 20 41 54 41 56 41 57 48 81 EC F0 01 00*/
		v_NET_Shutdown        = p_NET_Shutdown.RCast<void (*)(void*, const char*, uint8_t, bool)>();        /*48 89 6C 24 18 56 57 41 56 48 83 EC 30 83 B9 D0*/
		v_NET_SetKey          = p_NET_SetKey.RCast<void (*)(uintptr_t, const char*)>();                     /*48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 20 48 8B F9 41 B8*/
		v_NET_ReceiveDatagram = p_NET_ReceiveDatagram.RCast<bool (*)(int, netpacket_s*, bool)>();           /*E8 ?? ?? ?? ?? 84 C0 75 35 48 8B D3*/
		v_NET_SendDatagram    = p_NET_SendDatagram.RCast<int (*)(SOCKET, void*, int, v_netadr_t*, bool)>(); /*48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 41 56 41 57 48 81 EC ?? 05 00 00*/
		v_NET_PrintFunc       = p_NET_PrintFunc.RCast<void(*)(const char*)>();                              /*48 89 54 24 10 4C 89 44 24 18 4C 89 4C 24 20 C3 48*/

	}
	virtual void GetVar(void) const
	{
		g_pNetKey = g_GameDll.FindString("client:NetEncryption_NewKey").FindPatternSelf("48 8D ? ? ? ? ? 48 3B", CMemory::Direction::UP, 300).ResolveRelativeAddressSelf(0x3, 0x7).GetPtr();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////
REGISTER(VNet);
#endif // !NETCONSOLE

const char* NET_ErrorString(int iCode);
