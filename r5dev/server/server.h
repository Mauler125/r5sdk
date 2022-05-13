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

/* ==== CSERVER ========================================================================================================================================================= */
inline CMemory p_CServer_Think;
inline auto CServer_Think = p_CServer_Think.RCast<void (*)(bool bCheckClockDrift, bool bIsSimulating)>();

inline CMemory p_CServer_Authenticate;
inline auto CServer_Authenticate = p_CServer_Authenticate.RCast<void* (*)(void* pServer, user_creds* pCreds)>();

inline CMemory p_CServer_RejectConnection;
inline auto CServer_RejectConnection = p_CServer_RejectConnection.RCast<void* (*)(void* pServer, unsigned int a2, user_creds* pCreds, const char* szMessage)>();

inline int* sv_m_nTickCount = nullptr;

void CServer_Attach();
void CServer_Detach();

void IsClientBanned(R5Net::Client* r5net, const std::string ipaddr, std::int64_t nucleus_id);
void* HCServer_Authenticate(void* cserver, user_creds* inpacket);

extern bool g_bCheckCompBanDB;

///////////////////////////////////////////////////////////////////////////////
class VServer : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: CServer::Think                       : {:#18x} |\n", p_CServer_Think.GetPtr());
		spdlog::debug("| FUN: CServer::Authenticate                : {:#18x} |\n", p_CServer_Authenticate.GetPtr());
		spdlog::debug("| FUN: CServer::RejectConnection            : {:#18x} |\n", p_CServer_RejectConnection.GetPtr());
		spdlog::debug("| VAR: sv_m_nTickCount                      : {:#18x} |\n", reinterpret_cast<uintptr_t>(sv_m_nTickCount));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_CServer_Think            = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x81\xEC\x00\x00\x00\x00\x80\x3D\x00\x00\x00\x00\x00"), "xxxx?xxxx?xxxx????xx?????");
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CServer_Authenticate     = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x44\x89\x44\x24\x00\x55\x56\x57\x48\x8D\xAC\x24\x00\x00\x00\x00"), "xxxx?xxxxxxx????");
#elif defined (GAMEDLL_S2)
		p_CServer_Authenticate     = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x44\x89\x44\x24\x00\x56\x57\x48\x81\xEC\x00\x00\x00\x00"), "xxxx?xxxxx????");
#else
		p_CServer_Authenticate     = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x55\x57\x41\x55\x41\x57\x48\x8D\xAC\x24\x00\x00\x00\x00"), "xxxxxxxxxxx????");
#endif
		p_CServer_RejectConnection = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x4C\x89\x4C\x24\x00\x53\x55\x56\x57\x48\x81\xEC\x00\x00\x00\x00\x49\x8B\xD9"), "xxxx?xxxxxxx????xxx");

		CServer_Think            = p_CServer_Think.RCast<void (*)(bool bCheckClockDrift, bool bIsSimulating)>(); /*48 89 5C 24 ?? 48 89 74 24 ?? 57 48 81 EC ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ??*/
		CServer_Authenticate     = p_CServer_Authenticate.RCast<void* (*)(void* pServer, user_creds* pCreds)>(); /*40 55 57 41 55 41 57 48 8D AC 24 ?? ?? ?? ??*/
		CServer_RejectConnection = p_CServer_RejectConnection.RCast<void* (*)(void* pServer, unsigned int a2, user_creds* pCreds, const char* szMessage)>(); /*4C 89 4C 24 ?? 53 55 56 57 48 81 EC ?? ?? ?? ?? 49 8B D9*/
	}
	virtual void GetVar(void) const
	{
		sv_m_nTickCount = p_CServer_Think.Offset(0xB0).FindPatternSelf("8B 15", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x2, 0x6).RCast<int*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VServer);
