#include "core/stdafx.h"
#include "engine/host_cmd.h"

///////////////////////////////////////////////////////////////////////////////
EngineParms_t* g_pEngineParms = reinterpret_cast<EngineParms_t*>(g_pEngineParmsBuffer.GetPtr());

// TODO: this file is for when dedicated is stable, to move hardcoded patches in Host_Init for a more dynamic solution.
