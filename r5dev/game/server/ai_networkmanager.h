//=============================================================================//
//
// Purpose:
//
//=============================================================================//
#pragma once
#include "game/server/ai_network.h"

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
const int AINETWORK_OFFSET = 2808;
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
const int AINETWORK_OFFSET = 2840;
#endif

const string HULL_SIZE[5] = 
{
	"small",
	"med_short",
	"medium",
	"large",
	"extra_large"
};

/* ==== CAI_NETWORKMANAGER ============================================================================================================================================== */
inline CMemory p_CAI_NetworkManager__ShouldRebuild = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\x48\x8B\xD9\x48\x8B\x0D\x00\x00\x00\x00\x8B\x41\x6C"), "xxxxxxxxxxxx????xxx");
inline auto CAI_NetworkManager__ShouldRebuild = p_CAI_NetworkManager__ShouldRebuild.RCast<void* (*)(void* thisptr, CAI_Network* pNetwork, void* a3, int a4)>(); /*40 53 48 83 EC 20 48 8B D9 48 8B 0D ? ? ? ? 8B 41 6C*/
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
inline CMemory p_CAI_NetworkManager__LoadNetworkGraph = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x4C\x89\x44\x24\x00\x48\x89\x4C\x24\x00\x55\x53\x57\x41\x54\x41\x55\x41\x56"), "xxxx?xxxx?xxxxxxxxx");
inline auto CAI_NetworkManager__LoadNetworkGraph = p_CAI_NetworkManager__LoadNetworkGraph.RCast<void* (*)(void* thisptr, void* pBuffer, const char* pszFileName, int a4)>(); /*4C 89 44 24 ? 48 89 4C 24 ? 55 53 57 41 54 41 55 41 56*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
inline CMemory p_CAI_NetworkManager__LoadNetworkGraph = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x4C\x89\x44\x24\x00\x48\x89\x4C\x24\x00\x55\x53\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\x6C\x24\x00\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xFA"), "xxxx?xxxx?xxxxxxxxxxxxxxxx?xxx????xxx");
inline auto CAI_NetworkManager__LoadNetworkGraph = p_CAI_NetworkManager__LoadNetworkGraph.RCast<void* (*)(void* thisptr, void* pBuffer, const char* pszFileName)>(); /*4C 89 44 24 ? 48 89 4C 24 ? 55 53 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B FA*/

#endif
/* ==== CAI_NETWORKBUILDER ============================================================================================================================================== */
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
inline CMemory p_CAI_NetworkBuilder__Build = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x4C\x24\x00\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x83\xEC\x30\x48\x63\xBA\x00\x00\x00\x00"), "xxxx?xxxx?xxxx?xxxx?xxxxxxxxxxxxxxxx????");
inline auto CAI_NetworkBuilder__Build = p_CAI_NetworkBuilder__Build.RCast<void* (*)(void* thisptr, CAI_Network* pNetwork, void* a3, int a4)>(); /*48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 4C 24 ? 57 41 54 41 55 41 56 41 57 48 83 EC 30 48 63 BA ? ? ? ?*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
inline CMemory p_CAI_NetworkBuilder__Build = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x54\x24\x00\x48\x89\x4C\x24\x00\x53\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x83\xEC\x38\x8B\xB2\x00\x00\x00\x00"), "xxxx?xxxx?xxxxxxxxxxxxxxxxxx????");
inline auto CAI_NetworkBuilder__Build = p_CAI_NetworkBuilder__Build.RCast<void* (*)(void* thisptr, CAI_Network* pNetwork, void* a3, int a4)>(); /*48 89 54 24 ? 48 89 4C 24 ? 53 55 56 57 41 54 41 55 41 56 41 57 48 83 EC 38 8B B2 ? ? ? ?*/
#endif

void CAI_NetworkManager_Attach();
void CAI_NetworkManager_Detach();

namespace // !TODO: [AMOS] don't hardocde.
{
	int* g_nAiNodeClusters = CMemory(0x165DAD808).RCast<int*>();
	AINodeClusters*** g_pppAiNodeClusters = CMemory(0x165DAD7F0).RCast<AINodeClusters***>();
	int* g_nAiNodeClusterLinks = CMemory(0x165DB18E8).RCast<int*>();
	AINodeClusterLinks*** g_pppAiNodeClusterLinks = CMemory(0x165DB18D0).RCast<AINodeClusterLinks***>();
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
	static void SaveNetworkGraph(CAI_Network* pNetwork);
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
