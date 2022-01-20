#pragma once
#include "squirrel/sqapi.h"

namespace
{
	ADDRESS p_Script_Remote_BeginRegisteringFunctions = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x83\xEC\x28\x83\x3D\x00\x00\x00\x00\x00\x74\x10", "xxxxxx?????xx");
	void* Script_Remote_BeginRegisteringFunctions = (void*)p_Script_Remote_BeginRegisteringFunctions.GetPtr(); /*48 83 EC 28 83 3D ?? ?? ?? ?? ?? 74 10*/

	ADDRESS p_RestoreRemoteChecksumsFromSaveGame = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x4C\x24\x00\x41\x54\x48\x83\xEC\x40", "xxxx?xxxxxx");
	void* (*RestoreRemoteChecksumsFromSaveGame)(void* a1, void* a2) = (void* (*)(void*, void*))p_RestoreRemoteChecksumsFromSaveGame.GetPtr(); /*48 89 4C 24 ? 41 54 48 83 EC 40*/

	std::uint32_t* g_nServerRemoteChecksum = reinterpret_cast<std::uint32_t*>(p_RestoreRemoteChecksumsFromSaveGame.Offset(0x1C0).FindPatternSelf("48 8D 15", ADDRESS::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7).GetPtr());
	std::uint32_t* g_nClientRemoteChecksum = reinterpret_cast<std::uint32_t*>(p_Script_Remote_BeginRegisteringFunctions.Offset(0x0).FindPatternSelf("89 05", ADDRESS::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x6).GetPtr());
}

namespace VSquirrel
{
	namespace SHARED
	{
		SQRESULT Script_NativeTest(void* sqvm);
	}
	namespace SERVER
	{
	}
#ifndef DEDICATED
	namespace CLIENT
	{
	}
	namespace UI
	{
		SQRESULT GetServerName(void* sqvm);
		SQRESULT GetServerPlaylist(void* sqvm);
		SQRESULT GetServerMap(void* sqvm);
		SQRESULT GetServerCount(void* sqvm);
		SQRESULT GetSDKVersion(void* sqvm);
		SQRESULT GetPromoData(void* sqvm);
		SQRESULT SetEncKeyAndConnect(void* sqvm);
		SQRESULT CreateServerFromMenu(void* sqvm);
		SQRESULT JoinPrivateServerFromMenu(void* sqvm);
		SQRESULT GetPrivateServerMessage(void* sqvm);
		SQRESULT ConnectToIPFromMenu(void* sqvm);
		SQRESULT GetAvailableMaps(void* sqvm);
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
		std::cout << "| VAR: g_nServerRemoteChecksum              : 0x" << std::hex << std::uppercase << g_nServerRemoteChecksum                            << std::setw(0)    << " |" << std::endl;
		std::cout << "| VAR: g_nClientRemoteChecksum              : 0x" << std::hex << std::uppercase << g_nClientRemoteChecksum                            << std::setw(0)    << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HSqInit);
