#pragma once

/*-----------------------------------------------------------------------------
 * _interfaces.h
 *-----------------------------------------------------------------------------*/

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

struct FactoryInfo
{
	ADDRESS m_pFactoryPtr;
	std::string m_szFactoryFullName;
	std::string m_szFactoryName;
	std::string m_szFactoryVersion;

	FactoryInfo() : m_szFactoryFullName(std::string()), m_szFactoryName(std::string()), m_szFactoryVersion(std::string()), m_pFactoryPtr(nullptr) {}
	FactoryInfo(std::string factoryFullName, std::string factoryName, std::string factoryVersion, std::uintptr_t factoryPtr) : m_szFactoryFullName(factoryFullName), m_szFactoryName(factoryName), m_szFactoryVersion(factoryVersion), m_pFactoryPtr(factoryPtr) {}
	FactoryInfo(std::string factoryFullName, std::uintptr_t factoryPtr) : m_szFactoryFullName(factoryFullName), m_szFactoryName(std::string()), m_szFactoryVersion(std::string()), m_pFactoryPtr(factoryPtr) {}
};

//-----------------------------------------------------------------------------
// Class to hold all factories (interfaces)
//-----------------------------------------------------------------------------
class IFactory
{
public:

	void GetFactoriesFromRegister();
	void AddFactory(FactoryInfo factoryInfo);;
	ADDRESS GetFactoryPtr(const std::string& factoryName, bool versionLess = true);

	std::vector<FactoryInfo> factories = {};
};
extern IFactory* g_pFactory;

namespace
{
	/* ==== s_pInterfaceRegs ==================================================================================================================================================== */
	InterfaceGlobals_t* s_pInterfacesRegs = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\xE9\x00\x00\x00\x00\xCC\xCC\x89\x91\x00\x00\x00\x00", "x????xxxx????").FollowNearCallSelf().FindPatternSelf("48 8B 1D", ADDRESS::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).DerefSelf().RCast<InterfaceGlobals_t*>();
}

///////////////////////////////////////////////////////////////////////////////
class HFactory : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| VAR: s_pInterfacesRegs                    : 0x" << std::hex << std::uppercase << s_pInterfacesRegs << std::setw(0) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HFactory);