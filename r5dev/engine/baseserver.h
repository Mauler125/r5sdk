#pragma once
#include "engine/baseclient.h"

namespace
{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	int64_t* g_dwMaxClients = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x8B\x05\x00\x00\x00\x00\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48\x83\xEC\x28\x45\x33\xC0",
		"xx????xxxxxxxxxxxxxxxxx").ResolveRelativeAddressSelf(0x2, 0x6).RCast<int64_t*>(); /*8B 05 ? ? ? ? C3 CC CC CC CC CC CC CC CC CC 48 83 EC 28 45 33 C0*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	int64_t* g_dwMaxClients = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x8B\x05\x00\x00\x00\x00\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48\x83\xEC\x28\x48\x8B\x05\x00\x00\x00\x00",
		"xx????xxxxxxxxxxxxxxxxx????").ResolveRelativeAddressSelf(0x2, 0x6).RCast<int64_t*>(); /*8B 05 ? ? ? ? C3 CC CC CC CC CC CC CC CC CC 48 83 EC 28 48 8B 05 ? ? ? ?*/
#endif
	int64_t* g_dwMaxFakeClients = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x8B\x15\x00\x00\x00\x00\x33\xC0\x85\xD2\x7E\x37",
		"xx????xxxxxx").ResolveRelativeAddressSelf(0x2, 0x6).RCast<int64_t*>(); /*8B 15 ? ? ? ? 33 C0 85 D2 7E 37*/

		// This is a CUtlVector
	CBaseClient* m_Clients = p_IVEngineServer__GetNumHumanPlayers.Offset(0x0).FindPatternSelf("48 8D", ADDRESS::Direction::DOWN).ResolveRelativeAddress(0x3, 0x7).RCast<CBaseClient*>();
}

class CBaseServer
{
public:
	int64_t GetNumHumanPlayers(void) const;
	int64_t GetNumFakeClients(void) const;
};
extern CBaseServer* g_pServer;

///////////////////////////////////////////////////////////////////////////////
class HBaseServer : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| VAR: g_dwMaxClients                       : 0x" << std::hex << std::uppercase << g_dwMaxClients     << std::setw(0) << " |" << std::endl;
		std::cout << "| VAR: g_dwMaxFakeClients                   : 0x" << std::hex << std::uppercase << g_dwMaxFakeClients << std::setw(0) << " |" << std::endl;
		std::cout << "| VAR: m_Clients                            : 0x" << std::hex << std::uppercase << m_Clients          << std::setw(0) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HBaseServer);
