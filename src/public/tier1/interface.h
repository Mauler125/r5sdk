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

class InterfaceReg
{
public:
	InterfaceReg(InstantiateInterfaceFn fn, const char* pName);

public:
	InstantiateInterfaceFn m_CreateFn;
	const char* m_pName;
	InterfaceReg* m_pNext;
};
extern InterfaceReg** s_ppInterfaceRegs;

//-----------------------------------------------------------------------------
// Use this to expose an interface that can have multiple instances.
// e.g.:
// EXPOSE_INTERFACE( CInterfaceImp, IInterface, "MyInterface001" )
// This will expose a class called CInterfaceImp that implements IInterface (a pure class)
// clients can receive a pointer to this class by calling CreateInterface( "MyInterface001" )
//
// In practice, the shared header file defines the interface (IInterface) and version name ("MyInterface001")
// so that each component can use these names/vtables to communicate
//
// A single class can support multiple interfaces through multiple inheritance
//
// Use this if you want to write the factory function.
#define EXPOSE_INTERFACE_FN(functionName, interfaceName, versionName) \
	{	\
		static InterfaceReg __g_Create##interfaceName##_reg(functionName, versionName); \
	}
#define EXPOSE_INTERFACE(className, interfaceName, versionName) \
	{	\
		static void* __Create##className##_interface() {return static_cast<interfaceName *>( new className );} \
		static InterfaceReg __g_Create##className##_reg(__Create##className##_interface, versionName ); \
	}

// Use this to expose a singleton interface with a global variable you've created.
#define EXPOSE_SINGLE_INTERFACE_GLOBALVAR(className, interfaceName, versionName, globalVarName) \
	{ \
		static void* __Create##className##interfaceName##_interface() {return static_cast<interfaceName *>( &globalVarName );} \
		static InterfaceReg __g_Create##className##interfaceName##_reg(__Create##className##interfaceName##_interface, versionName); \
	}

// Use this to expose a singleton interface. This creates the global variable for you automatically.
#define EXPOSE_SINGLE_INTERFACE(className, interfaceName, versionName) \
	{	\
		static className __g_##className##_singleton; \
	}	\
	EXPOSE_SINGLE_INTERFACE_GLOBALVAR(className, interfaceName, versionName, __g_##className##_singleton)

#endif // INTERFACE_H
