#pragma once
#include "squirrel/sqapi.h"

inline CMemory p_Script_Remote_BeginRegisteringFunctions = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x28\x83\x3D\x00\x00\x00\x00\x00\x74\x10"), "xxxxxx?????xx");
inline auto Script_Remote_BeginRegisteringFunctions = p_Script_Remote_BeginRegisteringFunctions.RCast<void* (*)(void)>(); /*48 83 EC 28 83 3D ?? ?? ?? ?? ?? 74 10*/

inline CMemory p_RestoreRemoteChecksumsFromSaveGame = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x4C\x24\x00\x41\x54\x48\x83\xEC\x40"), "xxxx?xxxxxx");
inline auto RestoreRemoteChecksumsFromSaveGame = p_RestoreRemoteChecksumsFromSaveGame.RCast<void* (*)(void* a1, void* a2)>(); /*48 89 4C 24 ? 41 54 48 83 EC 40*/

/* CHANGE THESE WHEN SWITCHING TO PYLONV2 TO UNSIGNED AGAIN!*/
#ifndef CLIENT_DLL
inline int32_t* g_nServerRemoteChecksum = reinterpret_cast<int32_t*>(p_RestoreRemoteChecksumsFromSaveGame.Offset(0x1C0).FindPatternSelf("48 8D 15", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7).GetPtr());
#endif // !CLIENT_DLL
#ifndef DEDICATED
inline int32_t* g_nClientRemoteChecksum = reinterpret_cast<int32_t*>(p_Script_Remote_BeginRegisteringFunctions.Offset(0x0).FindPatternSelf("89 05", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x6).GetPtr());
#endif // !DEDICATED

namespace VSquirrel
{
	namespace SHARED
	{
		SQRESULT SDKNativeTest(HSQUIRRELVM v);
		SQRESULT GetSDKVersion(HSQUIRRELVM v);
	}
#ifndef CLIENT_DLL
	namespace SERVER
	{
		SQRESULT GetNumHumanPlayers(HSQUIRRELVM v);
		SQRESULT GetNumFakeClients(HSQUIRRELVM v);
	}
#endif // !CLIENT_DLL
#ifndef DEDICATED
	namespace CLIENT
	{
	}
	namespace UI
	{
		SQRESULT GetServerName(HSQUIRRELVM v);
		SQRESULT GetServerPlaylist(HSQUIRRELVM v);
		SQRESULT GetServerMap(HSQUIRRELVM v);
		SQRESULT GetServerCount(HSQUIRRELVM v);
		SQRESULT GetPromoData(HSQUIRRELVM v);
		SQRESULT SetEncKeyAndConnect(HSQUIRRELVM v);
		SQRESULT CreateServerFromMenu(HSQUIRRELVM v);
		SQRESULT JoinPrivateServerFromMenu(HSQUIRRELVM v);
		SQRESULT GetPrivateServerMessage(HSQUIRRELVM v);
		SQRESULT ConnectToIPFromMenu(HSQUIRRELVM v);
		SQRESULT GetAvailableMaps(HSQUIRRELVM v);
	}
#endif // !DEDICATED
}

///////////////////////////////////////////////////////////////////////////////
class HSqInit : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: Remote_BeginRegisteringFunctions     : 0x" << std::hex << std::uppercase << p_Script_Remote_BeginRegisteringFunctions.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: RestoreRemoteChecksumsFromSaveGame   : 0x" << std::hex << std::uppercase << p_RestoreRemoteChecksumsFromSaveGame.GetPtr()      << std::setw(npad) << " |" << std::endl;
#ifndef CLIENT_DLL
		std::cout << "| VAR: g_nServerRemoteChecksum              : 0x" << std::hex << std::uppercase << g_nServerRemoteChecksum                            << std::setw(0)    << " |" << std::endl;
#endif // !CLIENT_DLL
#ifndef DEDICATED
		std::cout << "| VAR: g_nClientRemoteChecksum              : 0x" << std::hex << std::uppercase << g_nClientRemoteChecksum                            << std::setw(0)    << " |" << std::endl;
#endif // !DEDICATED
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HSqInit);
