//
// Purpose: engine/launcher interface
//
// $NoKeywords: $
//=============================================================================//

#include "appframework/iappsystem.h"

//-----------------------------------------------------------------------------
// Return values from the initialization stage of the application framework
//-----------------------------------------------------------------------------
enum
{
	INIT_RESTART = INIT_LAST_VAL,
	RUN_FIRST_VAL,
};


//-----------------------------------------------------------------------------
// Return values from IEngineAPI::Run.
//-----------------------------------------------------------------------------
enum
{
	RUN_OK = RUN_FIRST_VAL,
	RUN_RESTART,
};


//-----------------------------------------------------------------------------
// Main engine interface to launcher + tools
//-----------------------------------------------------------------------------
#define VENGINE_LAUNCHER_API_VERSION "VENGINE_LAUNCHER_API_VERSION004"

// NOTE: _purecall IEngineAPI vtable
