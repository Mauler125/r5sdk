#pragma once
#include "server/IVEngineServer.h"

// PLEASE REMOVE AND FULLY MIGRATE TO CBASECLIENT!

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CBaseClient;

///////////////////////////////////////////////////////////////////////////////
extern CBaseClient* g_pClient;

namespace
{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	const uintptr_t g_dwCClientSize    = 0x4A440;
	const uintptr_t g_dwPersistenceVar = 0x5B4;
	const uintptr_t g_dwCClientPadding = 0x49E88;
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	const uintptr_t g_dwCClientSize    = 0x4A4C0;
	const uintptr_t g_dwPersistenceVar = 0x5BC;
	const uintptr_t g_dwCClientPadding = 0x49F00;
#endif
	static ADDRESS g_pClientBuffer = p_IVEngineServer__PersistenceAvailable.FindPatternSelf("48 8D 0D", ADDRESS::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7);
}

///////////////////////////////////////////////////////////////////////////////
class HClient : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| VAR: g_pClient                            : 0x" << std::hex << std::uppercase << g_pClient << std::setw(0) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HClient);
