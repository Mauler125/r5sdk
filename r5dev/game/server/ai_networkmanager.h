//=============================================================================//
//
// Purpose:
//
//=============================================================================//
#pragma once
#include "game/server/ai_network.h"

namespace
{
	/* ==== CAI_NETWORKMANAGER ============================================================================================================================================== */
	ADDRESS p_CAI_NetworkManager__ShouldRebuild = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x53\x48\x83\xEC\x20\x48\x8B\xD9\x48\x8B\x0D\x00\x00\x00\x00\x8B\x41\x6C", "xxxxxxxxxxxx????xxx");
	void* (*CAI_NetworkManager__ShouldRebuild)(void* thisptr, CAI_Network* pNetWork, void* a3, int a4) = (void* (*)(void*, CAI_Network*, void*, int))p_CAI_NetworkManager__ShouldRebuild.GetPtr(); /*40 53 48 83 EC 20 48 8B D9 48 8B 0D ? ? ? ? 8B 41 6C*/
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	ADDRESS p_CAI_NetworkManager__LoadNetworkGraph = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x4C\x89\x44\x24\x00\x48\x89\x4C\x24\x00\x55\x53\x57\x41\x54\x41\x55\x41\x56", "xxxx?xxxx?xxxxxxxxx");
	void* (*CAI_NetworkManager__LoadNetworkGraph)(void* thisptr, void* pBuffer, const char* pszFileName, int a4) = (void* (*)(void*, void*, const char*, int))p_CAI_NetworkManager__LoadNetworkGraph.GetPtr(); /*4C 89 44 24 ? 48 89 4C 24 ? 55 53 57 41 54 41 55 41 56*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	ADDRESS p_CAI_NetworkManager__LoadNetworkGraph = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x4C\x89\x44\x24\x00\x48\x89\x4C\x24\x00\x55\x53\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\x6C\x24\x00\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xFA", "xxxx?xxxx?xxxxxxxxxxxxxxxx?xxx????xxx");
	void* (*CAI_NetworkManager__LoadNetworkGraph)(void* thisptr, void* pBuffer, const char* pszFileName) = (void* (*)(void*, void*, const char*))p_CAI_NetworkManager__LoadNetworkGraph.GetPtr(); /*4C 89 44 24 ? 48 89 4C 24 ? 55 53 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B FA*/
#endif
	/* ==== CAI_NETWORKBUILDER ============================================================================================================================================== */
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	ADDRESS p_CAI_NetworkBuilder__Build = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x4C\x24\x00\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x83\xEC\x30\x48\x63\xBA\x00\x00\x00\x00", "xxxx?xxxx?xxxx?xxxx?xxxxxxxxxxxxxxxx????");
	void* (*CAI_NetworkBuilder__Build)(void* thisptr, CAI_Network* pNetWork, void* a3, int a4) = (void* (*)(void*, CAI_Network*, void*, int))p_CAI_NetworkBuilder__Build.GetPtr(); /*48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 4C 24 ? 57 41 54 41 55 41 56 41 57 48 83 EC 30 48 63 BA ? ? ? ?*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	ADDRESS p_CAI_NetworkBuilder__Build = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x54\x24\x00\x48\x89\x4C\x24\x00\x53\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x83\xEC\x38\x8B\xB2\x00\x00\x00\x00", "xxxx?xxxx?xxxxxxxxxxxxxxxxxx????");
	void* (*CAI_NetworkBuilder__Build)(void* thisptr, CAI_Network* pNetWork, void* a3, int a4) = (void* (*)(void*, CAI_Network*, void*, int))p_CAI_NetworkBuilder__Build.GetPtr(); /*48 89 54 24 ? 48 89 4C 24 ? 53 55 56 57 41 54 41 55 41 56 41 57 48 83 EC 38 8B B2 ? ? ? ?*/
#endif
}

void CAI_NetworkManager_Attach();
void CAI_NetworkManager_Detach();

// dword_165DAD808 = g_nAiNodeClusters
// qword_165DAD7F0 = pppUnkNodeStruct0s
// dword_165DB18E8 = g_nAiNodeClusterLinks
// qword_165DB18D0 = pppUnkStruct1s

namespace // !TODO: [AMOS] don't hardocde.
{
	int* g_nAiNodeClusters = ADDRESS(0x165DAD808).RCast<int*>();
	AINodeClusters*** g_pppAiNodeClusters = ADDRESS(0x165DAD7F0).RCast<AINodeClusters***>();
	int* g_nAiNodeClusterLinks = ADDRESS(0x165DB18E8).RCast<int*>();
	AINodeClusterLinks*** g_pppAiNodeClusterLinks = ADDRESS(0x165DB18D0).RCast<AINodeClusterLinks***>();
}

//-----------------------------------------------------------------------------
// CAI_NetworkBuilder
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class CAI_NetworkBuilder
{
public:
	static void BuildFile(CAI_Network* pNetwork);
};

///////////////////////////////////////////////////////////////////////////////
class HCAI_NetworkManager : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: CAI_NetworkManager::LoadNetworkGraph : 0x" << std::hex << std::uppercase << p_CAI_NetworkManager__ShouldRebuild.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: CAI_NetworkManager::ShouldRebuild    : 0x" << std::hex << std::uppercase << p_CAI_NetworkManager__ShouldRebuild.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: CAI_NetworkBuilder::Build            : 0x" << std::hex << std::uppercase << p_CAI_NetworkBuilder__Build.GetPtr()         << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HCAI_NetworkManager);
