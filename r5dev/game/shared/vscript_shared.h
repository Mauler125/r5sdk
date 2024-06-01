#ifndef VSCRIPT_SHARED_H
#define VSCRIPT_SHARED_H
#include "vscript/languages/squirrel_re/include/squirrel.h"
#include "vscript/languages/squirrel_re/vsquirrel.h"

inline void (*v_Script_RegisterCommonEnums_Server)(CSquirrelVM* const s);
inline void (*v_Script_RegisterCommonEnums_Client)(CSquirrelVM* const s);

inline void*(*v_Script_Remote_BeginRegisteringFunctions)(void);
inline void*(*v_RestoreRemoteChecksumsFromSaveGame)(void* a1, void* a2);

inline uint32_t* g_nServerRemoteChecksum = nullptr;
inline uint32_t* g_nClientRemoteChecksum = nullptr;

namespace VScriptCode
{
	namespace Shared
	{
		SQRESULT GetSDKVersion(HSQUIRRELVM v);
		SQRESULT GetAvailableMaps(HSQUIRRELVM v);
		SQRESULT GetAvailablePlaylists(HSQUIRRELVM v);
	}
}

void Script_RegisterCommonAbstractions(CSquirrelVM* s);
void Script_RegisterListenServerConstants(CSquirrelVM* s);

#define DEFINE_SHARED_SCRIPTFUNC_NAMED(s, functionName, helpString,          \
	returnType, parameters)                                                  \
	s->RegisterFunction(#functionName, MKSTRING(Script_##functionName),      \
	helpString, returnType, parameters, VScriptCode::Shared::##functionName);\

///////////////////////////////////////////////////////////////////////////////
class VScriptShared : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("Script_RegisterCommonEnums_Server", v_Script_RegisterCommonEnums_Server);
		LogFunAdr("Script_RegisterCommonEnums_Client", v_Script_RegisterCommonEnums_Client);

		LogFunAdr("Remote_BeginRegisteringFunctions", v_Script_Remote_BeginRegisteringFunctions);
		LogFunAdr("RestoreRemoteChecksumsFromSaveGame", v_RestoreRemoteChecksumsFromSaveGame);

		LogVarAdr("g_nServerRemoteChecksum", g_nServerRemoteChecksum);
		LogVarAdr("g_nClientRemoteChecksum", g_nClientRemoteChecksum);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("E8 ? ? ? ? 48 8B CB E8 ? ? ? ? 48 8B 43 08 BF ? ? ? ? 48 8B 50 60 48 8D 05 ? ? ? ? 48 89 82 ? ? ? ? 65 48 8B 04 25 ? ? ? ? 48 03 38 8B 07 39 05 ? ? ? ? 0F 8F ? ? ? ? 48 8D 05 ? ? ? ? 48 8D 0D ? ? ? ? 48 89 05 ? ? ? ? E8 ? ? ? ? 48 8D 05 ? ? ? ? 89 35 ? ? ? ? 48 89 05 ? ? ? ? 4C 8D 35 ? ? ? ?")
			.FollowNearCallSelf().GetPtr(v_Script_RegisterCommonEnums_Server);
		g_GameDll.FindPatternSIMD("E8 ? ? ? ? 48 8B CB E8 ? ? ? ? 48 8B 43 08 BF ? ? ? ? 48 8B 50 60 48 8D 05 ? ? ? ? 48 89 82 ? ? ? ? 65 48 8B 04 25 ? ? ? ? 48 03 38 8B 07 39 05 ? ? ? ? 0F 8F ? ? ? ? 48 8D 05 ? ? ? ? 48 8D 0D ? ? ? ? 48 89 05 ? ? ? ? E8 ? ? ? ? 48 8D 05 ? ? ? ? 89 35 ? ? ? ? 48 89 05 ? ? ? ? 4C 8D 3D ? ? ? ?")
			.FollowNearCallSelf().GetPtr(v_Script_RegisterCommonEnums_Client);

		g_GameDll.FindPatternSIMD("48 83 EC 28 83 3D ?? ?? ?? ?? ?? 74 10").GetPtr(v_Script_Remote_BeginRegisteringFunctions);
		g_GameDll.FindPatternSIMD("48 89 4C 24 ?? 41 54 48 83 EC 40").GetPtr(v_RestoreRemoteChecksumsFromSaveGame);
	}
	virtual void GetVar(void) const
	{
		g_nServerRemoteChecksum = CMemory(v_RestoreRemoteChecksumsFromSaveGame).Offset(0x1C0).FindPatternSelf("48 8D 15", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7).RCast<uint32_t*>();
		g_nClientRemoteChecksum = CMemory(v_Script_Remote_BeginRegisteringFunctions).Offset(0x0).FindPatternSelf("89 05", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x6).RCast<uint32_t*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // !VSCRIPT_SHARED_H
