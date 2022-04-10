#pragma once

#ifndef NETCONSOLE
#include "engine/net_chan.h"
#define MAX_STREAMS         2
#define FRAGMENT_BITS       8
#define FRAGMENT_SIZE       (1<<FRAGMENT_BITS)

/* ==== CNETCHAN ======================================================================================================================================================== */
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1) || defined (GAMEDLL_S2)
inline CMemory p_NET_Init = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x48\x89\x7C\x24\x20\x41\x54\x41\x56\x41\x57\x48\x81\xEC\xC0\x01\x00"), "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
inline auto v_NET_Init = p_NET_Init.RCast<void* (*)(char a1)>(); /*48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 48 89 7C 24 20 41 54 41 56 41 57 48 81 EC C0 01 00*/

inline CMemory p_NET_Shutdown = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x6C\x24\x18\x56\x57\x41\x56\x48\x83\xEC\x30\x83\xB9\xD8"), "xxxxxxxxxxxxxxxx");
inline auto v_NET_Shutdown = p_NET_Shutdown.RCast<void (*)(void* thisptr, const char* szReason, uint8_t a3, char a4)>(); /*48 89 6C 24 18 56 57 41 56 48 83 EC 30 83 B9 D8*/
#elif defined (GAMEDLL_S3)
inline CMemory p_NET_Init = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x48\x89\x7C\x24\x20\x41\x54\x41\x56\x41\x57\x48\x81\xEC\xF0\x01\x00"), "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
inline auto v_NET_Init = p_NET_Init.RCast<void* (*)(char a1)>(); /*48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 48 89 7C 24 20 41 54 41 56 41 57 48 81 EC F0 01 00*/

inline CMemory p_NET_Shutdown = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x6C\x24\x18\x56\x57\x41\x56\x48\x83\xEC\x30\x83\xB9\xD0"), "xxxxxxxxxxxxxxxx");
inline auto v_NET_Shutdown = p_NET_Shutdown.RCast<void (*)(void* thisptr, const char* szReason, uint8_t a3, char a4)>(); /*48 89 6C 24 18 56 57 41 56 48 83 EC 30 83 B9 D0*/
#endif
inline CMemory p_NET_SetKey = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x57\x48\x83\xEC\x20\x48\x8B\xF9\x41\xB8"), "xxxxxxxxxxxxxxxxxxxxxxxxx");
inline auto v_NET_SetKey = p_NET_SetKey.RCast<void (*)(uintptr_t pKey, const char* szHash)>(); /*48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 20 48 8B F9 41 B8*/

inline CMemory p_NET_ReceiveDatagram = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x74\x24\x18\x48\x89\x7C\x24\x20\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\xAC\x24\x50\xEB"), "xxxxxxxxxxxxxxxxxxxxxxxxx");
inline auto v_NET_ReceiveDatagram = p_NET_ReceiveDatagram.RCast<bool (*)(int iSocket, netpacket_s* pInpacket, bool bRaw)>(); /*E8 ?? ?? ?? ?? 84 C0 75 35 48 8B D3*/

inline CMemory p_NET_SendDatagram = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x57\x41\x56\x41\x57\x48\x81\xEC\x00\x05\x00\x00"), "xxxxxxxxxxxxxxxxxxxxxxx?xxx");
inline auto v_NET_SendDatagram = p_NET_SendDatagram.RCast<void* (*)(SOCKET s, const char* szPayload, int iLenght, int nFlags)>(); /*48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 41 56 41 57 48 81 EC ?? 05 00 00*/

inline CMemory p_NET_PrintFunc = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x54\x24\x10\x4C\x89\x44\x24\x18\x4C\x89\x4C\x24\x20\xC3\x48"), "xxxxxxxxxxxxxxxxx");
inline auto v_NET_PrintFunc = p_NET_PrintFunc.RCast<void(*)(const char* fmt)>(); /*48 89 54 24 10 4C 89 44 24 18 4C 89 4C 24 20 C3 48*/

///////////////////////////////////////////////////////////////////////////////
bool NET_ReceiveDatagram(int iSocket, netpacket_s* pInpacket, bool bRaw);
void* NET_SendDatagram(SOCKET s, const char* szPayload, int iLenght, int nFlags);
void NET_SetKey(const string& svNetKey);
void NET_GenerateKey();
void NET_PrintFunc(const char* fmt, ...);
void NET_DisconnectClient(CBaseClient* pClient, int nIndex, const char* szReason, uint8_t unk1, char unk2);

void NET_Attach();
void NET_Detach();
void NET_Trace_Attach();
void NET_Trace_Detach();

///////////////////////////////////////////////////////////////////////////////
extern string g_szNetKey;
extern uintptr_t g_pNetKey;

///////////////////////////////////////////////////////////////////////////////
class HNetChan : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: NET_Init                             : 0x" << std::hex << std::uppercase << p_NET_Init.GetPtr()            << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: NET_Shutdown                         : 0x" << std::hex << std::uppercase << p_NET_Shutdown.GetPtr()        << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: NET_SetKey                           : 0x" << std::hex << std::uppercase << p_NET_SetKey.GetPtr()          << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: NET_ReceiveDatagram                  : 0x" << std::hex << std::uppercase << p_NET_ReceiveDatagram.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: NET_SendDatagram                     : 0x" << std::hex << std::uppercase << p_NET_SendDatagram.GetPtr()    << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: NET_PrintFunc                        : 0x" << std::hex << std::uppercase << p_NET_PrintFunc.GetPtr()       << std::setw(npad) << " |" << std::endl;
		std::cout << "| VAR: g_pNetKey                            : 0x" << std::hex << std::uppercase << g_pNetKey << std::setw(npad)   << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////
REGISTER(HNetChan);
#endif // !NETCONSOLE

const char* NET_ErrorString(int iCode);
