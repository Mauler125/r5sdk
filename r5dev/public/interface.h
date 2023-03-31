#ifndef INTERFACE_H
#define INTERFACE_H

enum class InterfaceStatus_t : int
{
	IFACE_OK = 0,
	IFACE_FAILED
};

//-----------------------------------------------------------------------------
// Mapping of interface string to globals
//-----------------------------------------------------------------------------
typedef void* (*CreateInterfaceFn)(const char* pName, int* pReturnCode);
typedef void* (*InstantiateInterfaceFn)();
typedef HINSTANCE CSysModule;

struct InterfaceGlobals_t
{
	InstantiateInterfaceFn m_pInterfacePtr;
	const char* m_pInterfaceName;
	InterfaceGlobals_t* m_pNextInterfacePtr;
};
//-----------------------------------------------------------------------------

struct FactoryInfo_t
{
	CMemory m_pFactoryPtr;
	string m_szFactoryFullName;
	string m_szFactoryName;
	string m_szFactoryVersion;

	FactoryInfo_t() : m_szFactoryFullName(string()), m_szFactoryName(string()), m_szFactoryVersion(string()), m_pFactoryPtr(nullptr) {}
	FactoryInfo_t(string factoryFullName, string factoryName, string factoryVersion, uintptr_t factoryPtr) :
		m_szFactoryFullName(factoryFullName), m_szFactoryName(factoryName), m_szFactoryVersion(factoryVersion), m_pFactoryPtr(factoryPtr) {}
	FactoryInfo_t(string factoryFullName, uintptr_t factoryPtr) :
		m_szFactoryFullName(factoryFullName), m_szFactoryName(string()), m_szFactoryVersion(string()), m_pFactoryPtr(factoryPtr) {}
};

#endif // INTERFACE_H
