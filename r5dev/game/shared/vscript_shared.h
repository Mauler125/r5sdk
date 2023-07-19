#ifndef VSCRIPT_SHARED_H
#define VSCRIPT_SHARED_H
#include "vscript/languages/squirrel_re/include/squirrel.h"
#include "vscript/languages/squirrel_re/vsquirrel.h"

inline CMemory p_Script_Remote_BeginRegisteringFunctions;
inline void*(*Script_Remote_BeginRegisteringFunctions)(void);

inline CMemory p_RestoreRemoteChecksumsFromSaveGame;
inline void*(*RestoreRemoteChecksumsFromSaveGame)(void* a1, void* a2);

inline uint32_t* g_nServerRemoteChecksum = nullptr;
inline uint32_t* g_nClientRemoteChecksum = nullptr;

namespace VScriptCode
{
	namespace Shared
	{
		SQRESULT StubUnsupported(HSQUIRRELVM v);
		SQRESULT GetSDKVersion(HSQUIRRELVM v);
		SQRESULT GetAvailableMaps(HSQUIRRELVM v);
		SQRESULT GetAvailablePlaylists(HSQUIRRELVM v);
	}
}

void Script_RegisterCommonAbstractions(CSquirrelVM* s);

#define DEFINE_SCRIPTFUNC_STUBBED(s, functionName, returnType)          \
	s->RegisterFunction(#functionName, MKSTRING(Script_##functionName), \
	"Stub function; not supported on this build.", #returnType, "",     \
	&VScriptCode::Shared::StubUnsupported);                             \

#define DEFINE_SHARED_SCRIPTFUNC_NAMED(s, functionName, helpString,     \
	returnType, parameters)                                             \
	s->RegisterFunction(#functionName, MKSTRING(Script_##functionName), \
	helpString, returnType, parameters, VScriptCode::Shared::##functionName);\

///////////////////////////////////////////////////////////////////////////////
class VScriptShared : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("Remote_BeginRegisteringFunctions", p_Script_Remote_BeginRegisteringFunctions.GetPtr());
		LogFunAdr("RestoreRemoteChecksumsFromSaveGame", p_RestoreRemoteChecksumsFromSaveGame.GetPtr());
		LogVarAdr("g_nServerRemoteChecksum", reinterpret_cast<uintptr_t>(g_nServerRemoteChecksum));
		LogVarAdr("g_nClientRemoteChecksum", reinterpret_cast<uintptr_t>(g_nClientRemoteChecksum));
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
		g_nServerRemoteChecksum = p_RestoreRemoteChecksumsFromSaveGame.Offset(0x1C0).FindPatternSelf("48 8D 15", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7).RCast<uint32_t*>();
		g_nClientRemoteChecksum = p_Script_Remote_BeginRegisteringFunctions.Offset(0x0).FindPatternSelf("89 05", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x6).RCast<uint32_t*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

#endif // !VSCRIPT_SHARED_H
