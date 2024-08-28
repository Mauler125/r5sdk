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
		SQRESULT SetPlayerClassVar(HSQUIRRELVM v);
	}
}

void Script_RegisterCommonAbstractions(CSquirrelVM* s);
void Script_RegisterListenServerConstants(CSquirrelVM* s);

#define DEFINE_SHARED_SCRIPTFUNC_NAMED(s, functionName, helpString,          \
	returnType, parameters)                                                  \
	s->RegisterFunction(#functionName, MKSTRING(Script_##functionName),      \
	helpString, returnType, parameters, VScriptCode::Shared::##functionName);

struct ScriptClassDesc_t // sizeof=0x34
{                                       // XREF: .data:ScriptClassDesc_t g_C_BaseAnimating_ScriptDesc/r
                                        // .data:ScriptClassDesc_t g_C_BaseEntity_ScriptDesc/r
    const char *m_pszScriptName;        // XREF: InitC_BaseAnimatingScriptDesc(void)+26/w
                                        // InitC_BaseEntityScriptDesc(void)+15/w
    const char *m_pszClassname;         // XREF: InitC_BaseAnimatingScriptDesc(void)+2B/w
                                        // InitC_BaseEntityScriptDesc(void)+1A/w
    const char *m_pszDescription;       // XREF: InitC_BaseAnimatingScriptDesc(void)+1C/w
                                        // InitC_BaseEntityScriptDesc(void)+42/w
    ScriptClassDesc_t *m_pBaseDesc;     // XREF: InitC_BaseAnimatingScriptDesc(void)+38/w
                                        // InitC_BaseEntityScriptDesc(void)+25/w
    CUtlVector<ScriptFunctionBinding_t,CUtlMemory<ScriptFunctionBinding_t,int> > m_FunctionBindings;
                                        // XREF: InitC_BaseAnimatingScriptDesc(void)+47/r
                                        // InitC_BaseAnimatingScriptDesc(void)+4D/o ...
    void *(__cdecl *m_pfnConstruct)();
    void (__cdecl *m_pfnDestruct)(void *);
    void *pHelper;     // XREF: InitC_BaseAnimatingScriptDesc(void)+42/w
                                        // InitC_BaseEntityScriptDesc(void)+2B/w
    ScriptClassDesc_t *m_pNextDesc;     // XREF: _dynamic_initializer_for__g_C_BaseAnimating_ScriptDesc__+F/w
                                        // _dynamic_initializer_for__g_C_BaseEntity_ScriptDesc__+F/w
};

//inline void* EntityConstant_CBaseEntity_ClientUI;
//inline void* EntityConstant_CBaseEntity_Server;
//typedef void* (*sq_getentityfrominstanceType)(CSquirrelVM* sqvm, SQObject* pInstance, char** ppEntityConstant);
//inline sq_getentityfrominstanceType sq_getentityfrominstance;
//sq_getobjectType sq_getobject;
typedef SQBool(*sq_getthisentityType)(HSQUIRRELVM, void** ppEntity);
sq_getthisentityType sq_getthisentity;

//template <typename T> inline T* getentity(CSquirrelVM* sqvm, SQInteger iStackPos)
//{
//	SQObject obj;
//	sq_getobject(sqvm, iStackPos, &obj);
//
//	// there are entity constants for other types, but seemingly CBaseEntity's is the only one needed
//	return (T*)sq_getentityfrominstance(m_pSQVM, &obj, __sq_GetEntityConstant_CBaseEntity());
//}
///////////////////////////////////////////////////////////////////////////////

ScriptClassDesc_t* g_C_Player_ScriptDesc;
ScriptClassDesc_t* g_CPlayer_ScriptDesc;
bool isValidPlayerSettingsKeyName(std::string_view key);
bool isValidPlayerSettingsKeyValue(std::string_view value);

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
		g_GameDll.FindPatternSIMD("48 89 5C 24 ? 57 48 81 EC ? ? ? ? 48 8B 41 ? 48 8B FA 48 8B D9 F7 00").GetPtr(sq_getthisentity);
		//g_GameDll.FindPatternSIMD("48 85 D2 75 ? 8B 05 ? ? ? ? 83 F8 ? 7D").GetPtr(sq_getentityfrominstance);
	}
	virtual void GetVar(void) const
	{
		g_nServerRemoteChecksum = CMemory(v_RestoreRemoteChecksumsFromSaveGame).Offset(0x1C0).FindPatternSelf("48 8D 15", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7).RCast<uint32_t*>();
		g_nClientRemoteChecksum = CMemory(v_Script_Remote_BeginRegisteringFunctions).Offset(0x0).FindPatternSelf("89 05", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x6).RCast<uint32_t*>();
		//EntityConstant_CBaseEntity_ClientUI =*(g_GameDll.FindPatternSIMD("48 89 05 ? ? ? ? 48 8D 05 ? ? ? ? 48 89 05 ? ? ? ? 48 C7 05 ? ? ? ? ? ? ? ? 48 C7 05 ? ? ? ? ? ? ? ? E8").ResolveRelativeAddressSelf(3, 7).RCast<void**>());
		//EntityConstant_CBaseEntity_Server = *(g_GameDll.FindPatternSIMD("48 89 05 ? ? ? ? 33 D2 48 8D 05 ? ? ? ? 48 C7 05").ResolveRelativeAddressSelf(3, 7).RCast<void**>());
		g_C_Player_ScriptDesc = g_GameDll.FindPatternSIMD("4C 8D 0D ? ? ? ? 48 89 74 24 ? 4C 8D 05 ? ? ? ? 48 8B CD 48 8D 15 ? ? ? ? E8 ? ? ? ? 4C 8D 0D ? ? ? ? 48 89 74 24 ? 4C 8D 05 ? ? ? ? 48 8B CD 48 8D 15 ? ? ? ? E8 ? ? ? ? 4C 8D 0D ? ? ? ? 48 89 74 24 ? 4C 8D 05 ? ? ? ? 48 8B CD 48 8D 15 ? ? ? ? E8 ? ? ? ? 4C 8D 0D ? ? ? ? 48 89 74 24 ? 4C 8D 05 ? ? ? ? 48 8B CD 48 8D 15 ? ? ? ? E8 ? ? ? ? 4C 8D 0D ? ? ? ? 48 89 74 24 ? 4C 8D 05 ? ? ? ? 48 8B CD 48 8D 15 ? ? ? ? E8 ? ? ? ? 4C 8D 0D ? ? ? ? 48 89 74 24 ? 4C 8D 05 ? ? ? ? 48 8B CD 48 8D 15 ? ? ? ? E8 ? ? ? ? 4C 8D 0D").ResolveRelativeAddressSelf(3, 7).RCast<ScriptClassDesc_t*>();
		g_CPlayer_ScriptDesc = g_GameDll.FindPatternSIMD("48 89 05 ? ? ? ? 33 D2 48 8D 05 ? ? ? ? 48 89 05").ResolveRelativeAddressSelf(3, 7).RCast<ScriptClassDesc_t*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // !VSCRIPT_SHARED_H
