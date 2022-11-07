#include "core/stdafx.h"
#include "tier0/commandline.h"
#include "engine/host_cmd.h"

bool DFS_InitializeFeatureFlagDefinitions(const char* pszFeatureFlags)
{
	if (CommandLine()->CheckParm("-nodfs"))
		return false;

	return v_DFS_InitializeFeatureFlagDefinitions(pszFeatureFlags);
}

///////////////////////////////////////////////////////////////////////////////
void HostCmd_Attach()
{
	DetourAttach(&v_DFS_InitializeFeatureFlagDefinitions, &DFS_InitializeFeatureFlagDefinitions);
}

void HostCmd_Detach()
{
	DetourDetach(&v_DFS_InitializeFeatureFlagDefinitions, &DFS_InitializeFeatureFlagDefinitions);
}

///////////////////////////////////////////////////////////////////////////////
EngineParms_t* g_pEngineParms = nullptr;