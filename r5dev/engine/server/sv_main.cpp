#include "core/stdafx.h"
#include "engine/server/sv_main.h"
#include "networksystem/r5net.h"
#include "public/include/bansystem.h"

//-----------------------------------------------------------------------------
// Purpose: checks if particular client is banned on the comp server
//-----------------------------------------------------------------------------
void SV_IsClientBanned(R5Net::Client* pR5net, const string svIPAddr, uint64_t nNucleusID)
{
	string svError = string();

	bool bCompBanned = pR5net->GetClientIsBanned(svIPAddr, nNucleusID, svError);
	if (bCompBanned)
	{
		DevMsg(eDLL_T::SERVER, "Connection rejected for '%s' ('%llu' is banned from the master server!)\n", svIPAddr.c_str(), nNucleusID);
		g_pBanSystem->AddConnectionRefuse(svError, nNucleusID); // Add to the vector.
	}
}

///////////////////////////////////////////////////////////////////////////////