#ifndef APPSYSTEMGROUP_H
#define APPSYSTEMGROUP_H

#include "tier1/interface.h"
#include "tier1/utlvector.h"
#include "tier1/utldict.h"
#include "filesystem/filesystem.h"

//-----------------------------------------------------------------------------
// NOTE: The following methods must be implemented in your application
// although they can be empty implementations if you like...
//-----------------------------------------------------------------------------
abstract_class IAppSystemGroup
{
public:
	virtual ~IAppSystemGroup() {}

	// An installed application creation function, you should tell the group
	// the DLLs and the singleton interfaces you want to instantiate.
	// Return false if there's any problems and the app will abort
	virtual bool Create() = 0;

	// Allow the application to do some work after AppSystems are connected but 
	// they are all Initialized.
	// Return false if there's any problems and the app will abort
	virtual bool PreInit() = 0;

	// Allow the application to do some work after AppSystems are initialized but 
	// before main is run
	// Return false if there's any problems and the app will abort
	virtual bool PostInit() = 0;

	// Main loop implemented by the application
	virtual int Main() = 0;

	// Allow the application to do some work after all AppSystems are shut down
	virtual void PreShutdown() = 0;

	// Allow the application to do some work after all AppSystems are shut down
	virtual void PostShutdown() = 0;

	// Call an installed application destroy function, occurring after all modules
	// are unloaded
	virtual void Destroy() = 0;
};

//-----------------------------------------------------------------------------
// This class represents a group of app systems that all have the same lifetime
// that need to be connected/initialized, etc. in a well-defined order
//-----------------------------------------------------------------------------
class CAppSystemGroup : public IAppSystemGroup
{
public:
	// Used to determine where we exited out from the system
	enum AppSystemGroupStage_t
	{
		CREATION = 0,
		DEPENDENCIES,
		CONNECTION,
		PREINITIALIZATION,
		INITIALIZATION,
		POSTINITIALIZATION,
		RUNNING,
		PRESHUTDOWN,
		SHUTDOWN,
		POSTSHUTDOWN,
		DISCONNECTION,
		DESTRUCTION,

		APPSYSTEM_GROUP_STAGE_COUNT,
		NONE,	// This means no error
	};

	// Detour statics.
	static void StaticDestroy(CAppSystemGroup* pAppSystemGroup);

	// Returns the stage at which the app system group ran into an error
	AppSystemGroupStage_t GetCurrentStage() const;

	// Method to look up a particular named system...
	void* FindSystem(const char* pInterfaceName);

private:
	struct Module_t
	{
		CSysModule* m_pModule;
		CreateInterfaceFn m_Factory;
		char* m_pModuleName;
	};

protected:
	CUtlVector<Module_t> m_Modules;
	CUtlVector<IAppSystem*> m_Systems;
	CUtlVector<CreateInterfaceFn> m_NonAppSystemFactories;
	CUtlDict<int, unsigned short> m_SystemDict;
	CAppSystemGroup* m_pParentAppSystem;
	AppSystemGroupStage_t m_nCurrentStage;
};
static_assert(sizeof(CAppSystemGroup) == 0xA8);

inline CMemory p_CAppSystemGroup_Destroy;
inline void(*CAppSystemGroup_Destroy)(CAppSystemGroup* pAppSystemGroup);

///////////////////////////////////////////////////////////////////////////////
class VAppSystemGroup : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CAppSystemGroup::Destroy", p_CAppSystemGroup_Destroy.GetPtr());
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CAppSystemGroup_Destroy = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 41 54 41 55 41 56 41 57 48 83 EC 20 8B 81 ?? ?? ?? ??");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CAppSystemGroup_Destroy = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 8B 81 ?? ?? ?? ?? 48 8B F9");
#endif
		CAppSystemGroup_Destroy = p_CAppSystemGroup_Destroy.RCast<void(*)(CAppSystemGroup*)>();
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};

#endif // APPSYSTEMGROUP_H
