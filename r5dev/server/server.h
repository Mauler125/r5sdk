#pragma once
#include "tier0/basetypes.h"
#include "networksystem/r5net.h"

struct user_creds
{
	std::uint8_t  gap0[16];
	std::uint32_t m_nIpAddr;
	std::uint8_t  gap1[4];
	std::int32_t  m_nProtocolVer;
	std::uint8_t  gap2[12];
	std::int64_t  m_nNucleusID;
	std::int64_t  m_nUserID;
};

namespace
{
	/* ==== CSERVER ========================================================================================================================================================= */
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	ADDRESS p_CServer_Authenticate = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x44\x89\x44\x24\x00\x55\x56\x57\x48\x8D\xAC\x24\x00\x00\x00\x00", "xxxx?xxxxxxx????");
	void* (*CServer_Authenticate)(void* cserver, user_creds* creds) = (void* (*)(void* cserver, user_creds * creds))p_CServer_Authenticate.GetPtr(); /*44 89 44 24 ?? 55 56 57 48 8D AC 24 ?? ?? ?? ??*/
#elif defined (GAMEDLL_S2)
	ADDRESS p_CServer_Authenticate = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x44\x89\x44\x24\x00\x56\x57\x48\x81\xEC\x00\x00\x00\x00", "xxxx?xxxxx????");
	void* (*CServer_Authenticate)(void* cserver, user_creds* creds) = (void* (*)(void* cserver, user_creds * creds))p_CServer_Authenticate.GetPtr(); /*44 89 44 24 ?? 56 57 48 81 EC ?? ?? ?? ??*/
#else
	ADDRESS p_CServer_Authenticate = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x55\x57\x41\x55\x41\x57\x48\x8D\xAC\x24\x00\x00\x00\x00", "xxxxxxxxxxx????");
	void* (*CServer_Authenticate)(void* cserver, user_creds* creds) = (void* (*)(void* cserver, user_creds * creds))p_CServer_Authenticate.GetPtr(); /*40 55 57 41 55 41 57 48 8D AC 24 ?? ?? ?? ??*/
#endif
	ADDRESS p_CServer_RejectConnection = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x4C\x89\x4C\x24\x00\x53\x55\x56\x57\x48\x81\xEC\x00\x00\x00\x00\x49\x8B\xD9", "xxxx?xxxxxxx????xxx");
	void* (*CServer_RejectConnection)(void* pServer, unsigned int a2, user_creds* pCreds, const char* szMessage) = (void* (*)(void*, unsigned int, user_creds*, const char*))p_CServer_RejectConnection.GetPtr(); /*4C 89 4C 24 ?? 53 55 56 57 48 81 EC ?? ?? ?? ?? 49 8B D9*/
}

void CServer_Attach();
void CServer_Detach();

void IsClientBanned(R5Net::Client* r5net, const std::string ipaddr, std::int64_t nucleus_id);
void* HCServer_Authenticate(void* cserver, user_creds* inpacket);

extern bool g_bCheckCompBanDB;

///////////////////////////////////////////////////////////////////////////////
class HServer : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: CServer::Authenticate                : 0x" << std::hex << std::uppercase << p_CServer_Authenticate.GetPtr()     << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: CServer::RejectConnection            : 0x" << std::hex << std::uppercase << p_CServer_RejectConnection.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HServer);
