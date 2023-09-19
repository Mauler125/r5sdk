//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: An application framework 
//
// $Revision: $
// $NoKeywords: $
//===========================================================================//

#ifndef IAPPSYSTEM_H
#define IAPPSYSTEM_H

#include <vpc/interfaces.h>

//-----------------------------------------------------------------------------
// Specifies a module + interface name for initialization
//-----------------------------------------------------------------------------
struct AppSystemInfo_t
{
	const char* m_pModuleName;
	const char* m_pInterfaceName;
};

//-----------------------------------------------------------------------------
// Client systems are singleton objects in the client codebase responsible for
// various tasks
// The order in which the client systems appear in this list are the
// order in which they are initialized and updated. They are shut down in
// reverse order from which they are initialized.
//-----------------------------------------------------------------------------
enum InitReturnVal_t
{
	INIT_FAILED = 0,
	INIT_OK,

	INIT_LAST_VAL,
};

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
enum AppSystemTier_t
{
	APP_SYSTEM_TIER0 = 0,
	APP_SYSTEM_TIER1,
	APP_SYSTEM_TIER2,
	APP_SYSTEM_TIER3,

	APP_SYSTEM_TIER_OTHER,
};

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
abstract_class IAppSystem
{
public:
	virtual ~IAppSystem() = 0; // Prepended on each class derived class in assembly.

	// Here's where the app systems get to learn about each other 
	virtual bool Connect(CreateInterfaceFn factory) = 0;
	virtual void Disconnect() = 0;

	// Here's where systems can access other interfaces implemented by this object
	// Returns NULL if it doesn't implement the requested interface
	virtual void* QueryInterface(const char* pInterfaceName) = 0;

	// Init, shutdown
	virtual InitReturnVal_t Init() = 0;
	virtual void Shutdown() = 0;

	// Returns all dependent libraries
	//virtual const AppSystemInfo_t* GetDependencies() { return NULL; }

	// Returns the tier
	virtual AppSystemTier_t GetTier() = 0;

	// Reconnect to a particular interface
	virtual void Reconnect(CreateInterfaceFn factory, const char* pInterfaceName) = 0;
};

//-----------------------------------------------------------------------------
// Helper empty implementation of an IAppSystem
//-----------------------------------------------------------------------------
template< class IInterface >
class CBaseAppSystem : public IInterface
{
public:
	virtual ~CBaseAppSystem() = 0; // Prepended on each class derived class in assembly.

	// Here's where the app systems get to learn about each other 
	virtual bool Connect(CreateInterfaceFn factory) = 0;
	virtual void Disconnect() = 0;

	// Here's where systems can access other interfaces implemented by this object
	// Returns NULL if it doesn't implement the requested interface
	virtual void* QueryInterface(const char* pInterfaceName) = 0;

	// Init, shutdown
	virtual InitReturnVal_t Init() = 0;
	virtual void Shutdown() = 0;

	// Returns all dependent libraries
	//virtual const AppSystemInfo_t* GetDependencies() { return NULL; }

	// Returns the tier
	virtual AppSystemTier_t GetTier() = 0;

	// Reconnect to a particular interface
	virtual void Reconnect(CreateInterfaceFn factory, const char* pInterfaceName) = 0;
};

#endif // IAPPSYSTEM_H