#ifndef VSCRIPT_SHARED_H
#define VSCRIPT_SHARED_H
#include "vscript/languages/squirrel_re/include/squirrel.h"

inline CMemory p_Script_Remote_BeginRegisteringFunctions;
inline auto Script_Remote_BeginRegisteringFunctions = p_Script_Remote_BeginRegisteringFunctions.RCast<void* (*)(void)>();

inline CMemory p_RestoreRemoteChecksumsFromSaveGame;
inline auto RestoreRemoteChecksumsFromSaveGame = p_RestoreRemoteChecksumsFromSaveGame.RCast<void* (*)(void* a1, void* a2)>();

#ifndef CLIENT_DLL
inline uint32_t* g_nServerRemoteChecksum = nullptr;
#endif // !CLIENT_DLL
#ifndef DEDICATED
inline uint32_t* g_nClientRemoteChecksum = nullptr;
#endif // !DEDICATED

namespace VScriptCode
{
	namespace SHARED
	{
		SQRESULT SDKNativeTest(HSQUIRRELVM v);
		SQRESULT GetSDKVersion(HSQUIRRELVM v);
		SQRESULT GetAvailableMaps(HSQUIRRELVM v);
		SQRESULT GetAvailablePlaylists(HSQUIRRELVM v);
		SQRESULT ShutdownHostGame(HSQUIRRELVM v);
#ifndef DEDICATED
		SQRESULT IsClientDLL(HSQUIRRELVM v);
#endif // !DEDICATED
		SQRESULT IsServerActive(HSQUIRRELVM v);
#ifndef CLIENT_DLL
		SQRESULT KickPlayerByName(HSQUIRRELVM v);
		SQRESULT KickPlayerById(HSQUIRRELVM v);
		SQRESULT BanPlayerByName(HSQUIRRELVM v);
		SQRESULT BanPlayerById(HSQUIRRELVM v);
		SQRESULT UnbanPlayer(HSQUIRRELVM v);
#endif // !CLIENT_DLL
	}
#ifndef CLIENT_DLL
	namespace SERVER
	{
		SQRESULT GetNumHumanPlayers(HSQUIRRELVM v);
		SQRESULT GetNumFakeClients(HSQUIRRELVM v);
		SQRESULT IsDedicated(HSQUIRRELVM v);
	}
#endif // !CLIENT_DLL
#ifndef DEDICATED
	namespace CLIENT
	{
	}
	namespace UI
	{
		SQRESULT RefreshServerCount(HSQUIRRELVM v);
		SQRESULT GetServerName(HSQUIRRELVM v);
		SQRESULT GetServerDescription(HSQUIRRELVM v);
		SQRESULT GetServerMap(HSQUIRRELVM v);
		SQRESULT GetServerPlaylist(HSQUIRRELVM v);
		SQRESULT GetServerCurrentPlayers(HSQUIRRELVM v);
		SQRESULT GetServerMaxPlayers(HSQUIRRELVM v);
		SQRESULT GetServerCount(HSQUIRRELVM v);
		SQRESULT GetPromoData(HSQUIRRELVM v);
		SQRESULT ConnectToListedServer(HSQUIRRELVM v);
		SQRESULT CreateServer(HSQUIRRELVM v);
		SQRESULT ConnectToHiddenServer(HSQUIRRELVM v);
		SQRESULT GetHiddenServerName(HSQUIRRELVM v);
		SQRESULT ConnectToServer(HSQUIRRELVM v);
	}
#endif // !DEDICATED
}

///////////////////////////////////////////////////////////////////////////////
class VScriptShared : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("Remote_BeginRegisteringFunctions", p_Script_Remote_BeginRegisteringFunctions.GetPtr());
		LogFunAdr("RestoreRemoteChecksumsFromSaveGame", p_RestoreRemoteChecksumsFromSaveGame.GetPtr());
#ifndef CLIENT_DLL
		LogVarAdr("g_nServerRemoteChecksum", reinterpret_cast<uintptr_t>(g_nServerRemoteChecksum));
#endif // !CLIENT_DLL
#ifndef DEDICATED
		LogVarAdr("g_nClientRemoteChecksum", reinterpret_cast<uintptr_t>(g_nClientRemoteChecksum));
#endif // !DEDICATED
	}
	virtual void GetFun(void) const
	{
		p_Script_Remote_BeginRegisteringFunctions = g_GameDll.FindPatternSIMD("48 83 EC 28 83 3D ?? ?? ?? ?? ?? 74 10");
		p_RestoreRemoteChecksumsFromSaveGame      = g_GameDll.FindPatternSIMD("48 89 4C 24 ?? 41 54 48 83 EC 40");

		Script_Remote_BeginRegisteringFunctions = p_Script_Remote_BeginRegisteringFunctions.RCast<void* (*)(void)>();          /*48 83 EC 28 83 3D ?? ?? ?? ?? ?? 74 10*/
		RestoreRemoteChecksumsFromSaveGame      = p_RestoreRemoteChecksumsFromSaveGame.RCast<void* (*)(void* a1, void* a2)>(); /*48 89 4C 24 ?? 41 54 48 83 EC 40*/
	}
	virtual void GetVar(void) const
	{
#ifndef CLIENT_DLL
		g_nServerRemoteChecksum = p_RestoreRemoteChecksumsFromSaveGame.Offset(0x1C0).FindPatternSelf("48 8D 15", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7).RCast<uint32_t*>();
#endif // !CLIENT_DLL
#ifndef DEDICATED
		g_nClientRemoteChecksum = p_Script_Remote_BeginRegisteringFunctions.Offset(0x0).FindPatternSelf("89 05", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x6).RCast<uint32_t*>();
#endif // !DEDICATED
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

#endif // !VSCRIPT_SHARED_H
