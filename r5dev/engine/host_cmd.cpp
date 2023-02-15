#include "core/stdafx.h"
#include "tier0/commandline.h"
#include "host_cmd.h"
#include "common.h"
#include "client/client.h"

/*
==================
DFS_InitializeFeatureFlagDefinitions

Initialize feature 
flag definitions
==================
*/
bool DFS_InitializeFeatureFlagDefinitions(const char* pszFeatureFlags)
{
	if (CommandLine()->CheckParm("-nodfs"))
		return false;

	return v_DFS_InitializeFeatureFlagDefinitions(pszFeatureFlags);
}

/*
==================
Host_Status_PrintClient

Print client info 
to console
==================
*/
void Host_Status_PrintClient(CClient* client, bool bShowAddress, void (*print) (const char* fmt, ...))
{
	CNetChan* nci = client->GetNetChan();
	const char* state = "challenging";

	if (client->IsActive())
		state = "active";
	else if (client->IsSpawned())
		state = "spawning";
	else if (client->IsConnected())
		state = "connecting";

	if (nci != NULL)
	{
		print("# %i \"%s\" %llu %s %i %i %s %d", client->GetHandle(), client->GetServerName(), client->GetNucleusID(), COM_FormatSeconds(nci->GetTimeConnected()),
			static_cast<int>(1000.0f * nci->GetAvgLatency(FLOW_OUTGOING)), static_cast<int>(100.0f * nci->GetAvgLoss(FLOW_INCOMING)), state, nci->GetDataRate());

		if (bShowAddress)
		{
			print(" %s", nci->GetAddress());
		}
	}
	else
	{
		print("#%2i \"%s\" %s %llu", client->GetUserID() + 1, client->GetServerName(), client->GetNucleusID(), state);
	}

	print("\n");
}

///////////////////////////////////////////////////////////////////////////////
void VHostCmd::Attach() const
{
	DetourAttach(&v_DFS_InitializeFeatureFlagDefinitions, &DFS_InitializeFeatureFlagDefinitions);
	DetourAttach(&v_Host_Status_PrintClient, &Host_Status_PrintClient);
}

void VHostCmd::Detach() const
{
	DetourDetach(&v_DFS_InitializeFeatureFlagDefinitions, &DFS_InitializeFeatureFlagDefinitions);
	DetourDetach(&v_Host_Status_PrintClient, &Host_Status_PrintClient);
}

///////////////////////////////////////////////////////////////////////////////
EngineParms_t* g_pEngineParms = nullptr;