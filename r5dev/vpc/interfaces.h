#pragma once

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
#define INTERFACEVERSION_SERVERGAMECLIENTS        "ServerGameClients004"
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
#define FILESYSTEM_INTERFACE_VERSION              "VFileSystem017"
 //-----------------------------------------------------------------------------

enum class InterfaceStatus_t : int
{
	IFACE_OK = 0,
	IFACE_FAILED
};

//-----------------------------------------------------------------------------
// Mapping of interface string to globals
//-----------------------------------------------------------------------------
typedef void* (*InstantiateInterfaceFn)();
struct InterfaceGlobals_t
{
	InstantiateInterfaceFn m_pInterfacePtr;
	const char* m_pInterfaceName;
	InterfaceGlobals_t* m_pNextInterfacePtr;
};
//-----------------------------------------------------------------------------

struct FactoryInfo
{
	ADDRESS m_pFactoryPtr;
	string m_szFactoryFullName;
	string m_szFactoryName;
	string m_szFactoryVersion;

	FactoryInfo() : m_szFactoryFullName(string()), m_szFactoryName(string()), m_szFactoryVersion(string()), m_pFactoryPtr(nullptr) {}
	FactoryInfo(string factoryFullName, string factoryName, string factoryVersion, uintptr_t factoryPtr) : 
		m_szFactoryFullName(factoryFullName), m_szFactoryName(factoryName), m_szFactoryVersion(factoryVersion), m_pFactoryPtr(factoryPtr) {}
	FactoryInfo(string factoryFullName, uintptr_t factoryPtr) : 
		m_szFactoryFullName(factoryFullName), m_szFactoryName(string()), m_szFactoryVersion(string()), m_pFactoryPtr(factoryPtr) {}
};

//-----------------------------------------------------------------------------
// Class to hold all factories (interfaces)
//-----------------------------------------------------------------------------
class CFactory
{
public:
	void AddFactory(const string& svFactoryName, void* pFactory);
	void AddFactory(FactoryInfo factoryInfo);
	int GetVersionIndex(const string& svInterfaceName) const;
	void GetFactoriesFromRegister(void);
	ADDRESS GetFactoryPtr(const string& factoryName, bool versionLess = true) const;

private:
	vector<FactoryInfo> m_vFactories;
};
extern CFactory* g_pFactory;

/* ==== s_pInterfaceRegs ==================================================================================================================================================== */
inline ADDRESS s_pInterfacesRegs = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\xE9\x00\x00\x00\x00\xCC\xCC\x89\x91\x00\x00\x00\x00"), "x????xxxx????")
	.FollowNearCallSelf().FindPatternSelf("48 8B 1D", ADDRESS::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7);

///////////////////////////////////////////////////////////////////////////////
class HFactory : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| VAR: s_pInterfacesRegs                    : 0x" << std::hex << std::uppercase << s_pInterfacesRegs.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HFactory);