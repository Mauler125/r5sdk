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

	FactoryInfo_t() : m_pFactoryPtr(nullptr) {}
	FactoryInfo_t(const uintptr_t factoryPtr, const string& factoryFullName, const string& factoryName, const string& factoryVersion) :
		m_pFactoryPtr(factoryPtr), m_szFactoryFullName(factoryFullName), m_szFactoryName(factoryName), m_szFactoryVersion(factoryVersion) {}
	FactoryInfo_t(const uintptr_t factoryPtr, const string& factoryFullName) :
		m_pFactoryPtr(factoryPtr), m_szFactoryFullName(factoryFullName) {}
};

#endif // INTERFACE_H
