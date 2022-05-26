#include "core/stdafx.h"
#include "engine/server/sv_main.h"
#include "networksystem/r5net.h"
#include "public/include/bansystem.h"

//-----------------------------------------------------------------------------
// Purpose: checks if particular client is banned on the comp server
//-----------------------------------------------------------------------------
void SV_IsClientBanned(R5Net::Client* pR5net, const std::string svIPAddr, std::int64_t nNucleusID)
{
	std::string svError = std::string();
	bool bCompBanned = pR5net->GetClientIsBanned(svIPAddr, nNucleusID, svError);
	if (bCompBanned)
	{
		DevMsg(eDLL_T::SERVER, "\n");
		DevMsg(eDLL_T::SERVER, "______________________________________________________________\n");
		DevMsg(eDLL_T::SERVER, "] PYLON_NOTICE -----------------------------------------------\n");
		DevMsg(eDLL_T::SERVER, "] OriginID : | '%lld' IS PYLON BANNED.\n", nNucleusID);
		DevMsg(eDLL_T::SERVER, "--------------------------------------------------------------\n");
		DevMsg(eDLL_T::SERVER, "\n");
		g_pBanSystem->AddConnectionRefuse(svError, nNucleusID); // Add to the vector.
	}
}

///////////////////////////////////////////////////////////////////////////////