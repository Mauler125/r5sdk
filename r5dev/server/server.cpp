#include "core/stdafx.h"
#include "tier0/cvar.h"
#include "engine/sys_utils.h"
#include "server/server.h"
#include "engine/baseclient.h"
#include "networksystem/r5net.h"
#include "public/include/bansystem.h"

//-----------------------------------------------------------------------------
// Purpose: checks if particular client is banned on the comp server
//-----------------------------------------------------------------------------
void IsClientBanned(R5Net::Client* pR5net, const std::string svIPAddr, std::int64_t nNucleusID)
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

//-----------------------------------------------------------------------------
// Purpose: client to server authentication
//-----------------------------------------------------------------------------
void* HCServer_Authenticate(void* pServer, user_creds* pInpacket)
{
	std::string svIpAddress = pInpacket->m_nAddr.GetAddress();
	if (sv_showconnecting->GetBool())
	{
		DevMsg(eDLL_T::SERVER, "\n");
		DevMsg(eDLL_T::SERVER, "______________________________________________________________\n");
		DevMsg(eDLL_T::SERVER, "] AUTHENTICATION ---------------------------------------------\n");
		DevMsg(eDLL_T::SERVER, "] UID : | '%s'\n", pInpacket->m_nUserID);
		DevMsg(eDLL_T::SERVER, "] OID : | '%lld'\n", pInpacket->m_nNucleusID);
		DevMsg(eDLL_T::SERVER, "] ADR : | '%s'\n", svIpAddress.c_str());
		DevMsg(eDLL_T::SERVER, "--------------------------------------------------------------\n");
	}

	if (g_pBanSystem->IsBanListValid()) // Is the banlist vector valid?
	{
		if (g_pBanSystem->IsBanned(svIpAddress, pInpacket->m_nNucleusID)) // Is the client trying to connect banned?
		{
			CServer_RejectConnection(pServer, *(unsigned int*)((std::uintptr_t)pServer + 0xC), pInpacket, "You have been banned from this Server."); // RejectConnection for the client.

			if (sv_showconnecting->GetBool())
			{
				Warning(eDLL_T::SERVER, "Connection rejected for '%s' ('%lld' is banned from this Server!)\n", svIpAddress.c_str(), pInpacket->m_nNucleusID);
			}
			return nullptr;
		}
	}
	if (sv_showconnecting->GetBool())
	{
		DevMsg(eDLL_T::SERVER, "\n");
	}

	if (g_bCheckCompBanDB)
	{
		if (g_pR5net)
		{
			std::thread th(IsClientBanned, g_pR5net, svIpAddress, pInpacket->m_nNucleusID);
			th.detach();
		}
	}

	return CServer_Authenticate(pServer, pInpacket);
}

///////////////////////////////////////////////////////////////////////////////
void CServer_Attach()
{
	DetourAttach((LPVOID*)&CServer_Authenticate, &HCServer_Authenticate);
}

void CServer_Detach()
{
	DetourDetach((LPVOID*)&CServer_Authenticate, &HCServer_Authenticate);
}

///////////////////////////////////////////////////////////////////////////////
bool g_bCheckCompBanDB = true;