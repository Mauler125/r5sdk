#pragma once
#include "tier1/NetAdr2.h"
#include "networksystem/r5net.h"

struct user_creds
{
	v_netadr_t m_nAddr;
	int32_t  m_nProtocolVer;
	int32_t  m_nchallenge;
	uint8_t  gap2[8];
	int64_t  m_nNucleusID;
	int64_t  m_nUserID;
};

namespace
{
	/* ==== CSERVER ========================================================================================================================================================= */
	ADDRESS p_CServer_Think = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x81\xEC\x00\x00\x00\x00\x80\x3D\x00\x00\x00\x00\x00"), "xxxx?xxxx?xxxx????xx?????");
	void (*CServer_Think)(bool check_clock_drift, bool bIsSimulating) = (void (*)(bool, bool))p_CServer_Think.GetPtr(); /*48 89 5C 24 ? 48 89 74 24 ? 57 48 81 EC ? ? ? ? 80 3D ? ? ? ? ?*/
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	ADDRESS p_CServer_Authenticate = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x44\x89\x44\x24\x00\x55\x56\x57\x48\x8D\xAC\x24\x00\x00\x00\x00"), "xxxx?xxxxxxx????");
	void* (*CServer_Authenticate)(void* cserver, user_creds* creds) = (void* (*)(void* cserver, user_creds * creds))p_CServer_Authenticate.GetPtr(); /*44 89 44 24 ?? 55 56 57 48 8D AC 24 ?? ?? ?? ??*/
#elif defined (GAMEDLL_S2)
	ADDRESS p_CServer_Authenticate = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x44\x89\x44\x24\x00\x56\x57\x48\x81\xEC\x00\x00\x00\x00"), "xxxx?xxxxx????");
	void* (*CServer_Authenticate)(void* cserver, user_creds* creds) = (void* (*)(void* cserver, user_creds * creds))p_CServer_Authenticate.GetPtr(); /*44 89 44 24 ?? 56 57 48 81 EC ?? ?? ?? ??*/
#else
	ADDRESS p_CServer_Authenticate = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x55\x57\x41\x55\x41\x57\x48\x8D\xAC\x24\x00\x00\x00\x00"), "xxxxxxxxxxx????");
	void* (*CServer_Authenticate)(void* cserver, user_creds* creds) = (void* (*)(void* cserver, user_creds * creds))p_CServer_Authenticate.GetPtr(); /*40 55 57 41 55 41 57 48 8D AC 24 ?? ?? ?? ??*/
#endif
	ADDRESS p_CServer_RejectConnection = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x4C\x89\x4C\x24\x00\x53\x55\x56\x57\x48\x81\xEC\x00\x00\x00\x00\x49\x8B\xD9"), "xxxx?xxxxxxx????xxx");
	void* (*CServer_RejectConnection)(void* pServer, unsigned int a2, user_creds* pCreds, const char* szMessage) = (void* (*)(void*, unsigned int, user_creds*, const char*))p_CServer_RejectConnection.GetPtr(); /*4C 89 4C 24 ?? 53 55 56 57 48 81 EC ?? ?? ?? ?? 49 8B D9*/

	int* sv_m_nTickCount = p_CServer_Think.Offset(0xB0).FindPatternSelf("8B 15", ADDRESS::Direction::DOWN).ResolveRelativeAddressSelf(0x2, 0x6).RCast<int*>();
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
		std::cout << "| FUN: CServer::Think                       : 0x" << std::hex << std::uppercase << p_CServer_Think.GetPtr()            << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: CServer::Authenticate                : 0x" << std::hex << std::uppercase << p_CServer_Authenticate.GetPtr()     << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: CServer::RejectConnection            : 0x" << std::hex << std::uppercase << p_CServer_RejectConnection.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "| VAR: sv_m_nTickCount                      : 0x" << std::hex << std::uppercase << sv_m_nTickCount                     << std::setw(0)    << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HServer);
