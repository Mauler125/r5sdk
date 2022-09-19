#include "core/stdafx.h"
#include "tier0/threadtools.h"
#include "tier0/frametask.h"
#include "engine/server/sv_main.h"
#include "networksystem/pylon.h"
#include "networksystem/bansystem.h"

//-----------------------------------------------------------------------------
// Purpose: checks if particular client is banned on the comp server
//-----------------------------------------------------------------------------
void SV_IsClientBanned(const string& svIPAddr, const uint64_t nNucleusID)
{
	string svError;

	bool bCompBanned = g_pMasterServer->CheckForBan(svIPAddr, nNucleusID, svError);
	if (bCompBanned)
	{
		if (!ThreadInMainThread())
		{
			g_TaskScheduler->Dispatch([svError, nNucleusID]
				{
					g_pBanSystem->AddConnectionRefuse(svError, nNucleusID); // Add to the vector.
				}, 0);
		}
		Warning(eDLL_T::SERVER, "Added '%s' to refused list ('%llu' is banned from the master server!)\n", svIPAddr.c_str(), nNucleusID);
	}
}

///////////////////////////////////////////////////////////////////////////////