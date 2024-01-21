#pragma once
#include "tier1/interface.h"
#include "pluginsdk/ifactory.h"

/*-----------------------------------------------------------------------------
 * _interfaces.h
 *-----------------------------------------------------------------------------*/

#define VENGINE_LAUNCHER_API_VERSION              "VENGINE_LAUNCHER_API_VERSION004"
#define VENGINE_GAMEUIFUNCS_VERSION               "VENGINE_GAMEUIFUNCS_VERSION005"

#define VENGINE_RENDERVIEW_INTERFACE_VERSION      "VEngineRenderView014"
#define VENGINE_RANDOM_INTERFACE_VERSION          "VEngineRandom001"
#define VENGINE_HUDMODEL_INTERFACE_VERSION        "VEngineModel016"
#define MATERIALSYSTEM_CONFIG_VERSION             "VMaterialSystemConfig004"

#define SERVER_DLL_SHARED_APPSYSTEMS              "VServerDllSharedAppSystems001"
#define INTERFACEVERSION_SERVERGAMECLIENTS_NEW    "ServerGameClients004"
#define INTERFACEVERSION_SERVERGAMECLIENTS        "ServerGameClients003"
#define INTERFACEVERSION_SERVERGAMEENTS           "ServerGameEnts002"
#define INTERFACEVERSION_SERVERGAMEDLL            "ServerGameDLL005"

#define VCLIENT_PREDICTION_INTERFACE_VERSION      "VClientPrediction001"
#define VCLIENTENTITYLIST_INTERFACE_VERSION       "VClientEntityList003"
#define CLIENT_DLL_INTERFACE_VERSION              "VClient018"
#define CLIENTRENDERTARGETS_INTERFACE_VERSION     "ClientRenderTargets001"
#define INTERFACEVERSION_ENGINETRACE_CLIENT       "EngineTraceClient004"
#define INTERFACEVERSION_ENGINETRACEDECALS_CLIENT "EngineTraceClientDecals004"

#define VGUI_SYSTEM_INTERFACE_VERSION             "VGUI_System010"
#define GAMEUI_INTERFACE_VERSION                  "GameUI011"

#define RUNGAMEENGINE_INTERFACE_VERSION           "RunGameEngine005"
#define EVENTSYSTEM_INTERFACE_VERSION             "EventSystem001"

#define CVAR_QUERY_INTERFACE_VERSION              "VCvarQuery001"
#define VPHYSICS_DEBUG_OVERLAY_INTERFACE_VERSION  "VPhysicsDebugOverlay001"
#define VDEBUG_OVERLAY_INTERFACE_VERSION          "VDebugOverlay004"
#define SOUNDCARD_INTERFACE_VERSION               "ISoundC002"

#define SHADERSYSTEM_INTERFACE_VERSION            "ShaderSystem002"
#define FACTORY_INTERFACE_VERSION                 "VFactorySystem002"
#define FILESYSTEM_INTERFACE_VERSION              "VFileSystem017"
#define BASEFILESYSTEM_INTERFACE_VERSION          "VBaseFileSystem011"
#define KEYVALUESSYSTEM_INTERFACE_VERSION         "VKeyValuesSystem001"

//-----------------------------------------------------------------------------
// Class to hold all factories (interfaces)
//-----------------------------------------------------------------------------
class CFactorySystem : public IFactorySystem
{
public:
	virtual void AddFactory(InstantiateInterfaceFn createFn, const char* pName) const override;
	virtual void* GetFactory(const char* pName)                                 const override;
	virtual const char* GetVersion(void)                                        const override;
};

extern CFactorySystem g_FactorySystem;
PLATFORM_INTERFACE IFactorySystem* GetFactorySystem();

///////////////////////////////////////////////////////////////////////////////
inline void*(*v_CreateInterfaceInternal)(const char* pName, int* pReturnCode);

class VFactory : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CreateInterfaceInternal", v_CreateInterfaceInternal);
		LogVarAdr("s_pInterfaceRegs", s_ppInterfaceRegs);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 8B 1D ?? ?? ?? ?? 48 8B FA").GetPtr(v_CreateInterfaceInternal);
	}
	virtual void GetVar(void) const
	{
		s_ppInterfaceRegs = g_GameDll.FindPatternSIMD("E9 ?? ?? ?? ?? CC CC 89 91 ?? ?? ?? ??")
			.FollowNearCallSelf().FindPatternSelf("48 8B 1D", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).RCast<InterfaceReg**>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool /*bAttach*/) const { }
};
///////////////////////////////////////////////////////////////////////////////
