//=============================================================================//
//
// Purpose:
//
//=============================================================================//
#pragma once
#include "game/server/ai_network.h"
#include "game/server/detour_impl.h"

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
const int AINETWORK_OFFSET = 2808;
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
const int AINETWORK_OFFSET = 2840;
#endif

/* ==== CAI_NETWORKMANAGER ============================================================================================================================================== */
inline CMemory p_CAI_NetworkManager__ShouldRebuild = nullptr;
inline void*(*CAI_NetworkManager__ShouldRebuild)(void* thisptr, CAI_Network* pNetwork, void* a3, int a4);
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
inline CMemory p_CAI_NetworkManager__LoadNetworkGraph = nullptr;
inline void*(*CAI_NetworkManager__LoadNetworkGraph)(void* thisptr, void* pBuffer, const char* pszFileName, int a4);
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
inline CMemory p_CAI_NetworkManager__LoadNetworkGraph = nullptr;
inline void*(*CAI_NetworkManager__LoadNetworkGraph)(void* thisptr, void* pBuffer, const char* pszFileName);
#endif
/* ==== CAI_NETWORKBUILDER ============================================================================================================================================== */
inline CMemory p_CAI_NetworkBuilder__Build;
inline void*(*CAI_NetworkBuilder__Build)(void* thisptr, CAI_Network* pNetwork, void* a3, int a4);

inline CUtlVector<CAI_Cluster*>* g_pAIPathClusters = nullptr;
inline CUtlVector<CAI_ClusterLink*>* g_pAIClusterLinks = nullptr;

//-----------------------------------------------------------------------------
// CAI_NetworkBuilder
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class CAI_NetworkBuilder
{
public:
	static void Build(CAI_NetworkBuilder* pBuilder, CAI_Network* pAINetwork, void* a3, int a4);
	static void SaveNetworkGraph(CAI_Network* pNetwork);
};

//-----------------------------------------------------------------------------
// CAI_NetworkManager
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class CAI_NetworkManager
{
public:
	static void LoadNetworkGraph(CAI_NetworkManager* pAINetworkManager, void* pBuffer, const char* szAIGraphFile);
	static void LoadNetworkGraphEx(CAI_NetworkManager* pAINetworkManager, void* pBuffer, const char* szAIGraphFile);
};

///////////////////////////////////////////////////////////////////////////////
class VAI_NetworkManager : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CAI_NetworkManager::LoadNetworkGraph", p_CAI_NetworkManager__LoadNetworkGraph.GetPtr());
		LogFunAdr("CAI_NetworkManager::DelayedInit", p_CAI_NetworkManager__ShouldRebuild.GetPtr());
		LogFunAdr("CAI_NetworkBuilder::Build", p_CAI_NetworkBuilder__Build.GetPtr());
		LogVarAdr("g_AIPathClusters< CAI_Cluster* >", reinterpret_cast<uintptr_t>(g_pAIPathClusters));
		LogVarAdr("g_AIClusterLinks< CAI_ClusterLink* >", reinterpret_cast<uintptr_t>(g_pAIClusterLinks));
	}
	virtual void GetFun(void) const
	{
		p_CAI_NetworkManager__ShouldRebuild = g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 48 8B D9 48 8B 0D ?? ?? ?? ?? 8B 41 6C");
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CAI_NetworkManager__LoadNetworkGraph = g_GameDll.FindPatternSIMD("4C 89 44 24 ?? 48 89 4C 24 ?? 55 53 57 41 54 41 55 41 56");
		CAI_NetworkManager__LoadNetworkGraph = p_CAI_NetworkManager__LoadNetworkGraph.RCast<void* (*)(void*, void*, const char*, int)>(); /*4C 89 44 24 ?? 48 89 4C 24 ?? 55 53 57 41 54 41 55 41 56*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CAI_NetworkManager__LoadNetworkGraph = g_GameDll.FindPatternSIMD("4C 89 44 24 ?? 48 89 4C 24 ?? 55 53 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ?? 48 81 EC ?? ?? ?? ?? 48 8B FA");
		CAI_NetworkManager__LoadNetworkGraph = p_CAI_NetworkManager__LoadNetworkGraph.RCast<void* (*)(void*, void*, const char*)>(); /*4C 89 44 24 ?? 48 89 4C 24 ?? 55 53 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ?? 48 81 EC ?? ?? ?? ?? 48 8B FA*/
#endif
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CAI_NetworkBuilder__Build = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 4C 24 ?? 57 41 54 41 55 41 56 41 57 48 83 EC 30 48 63 BA ?? ?? ?? ??");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CAI_NetworkBuilder__Build = g_GameDll.FindPatternSIMD("48 89 54 24 ?? 48 89 4C 24 ?? 53 55 56 57 41 54 41 55 41 56 41 57 48 83 EC 38 8B B2 ?? ?? ?? ??");
#endif
		CAI_NetworkManager__ShouldRebuild = p_CAI_NetworkManager__ShouldRebuild.RCast<void* (*)(void*, CAI_Network*, void*, int)>(); /*40 53 48 83 EC 20 48 8B D9 48 8B 0D ?? ?? ?? ?? 8B 41 6C*/
		CAI_NetworkBuilder__Build         = p_CAI_NetworkBuilder__Build.RCast<void* (*)(void*, CAI_Network*, void*, int)>();         /*48 89 54 24 ?? 48 89 4C 24 ?? 53 55 56 57 41 54 41 55 41 56 41 57 48 83 EC 38 8B B2 ?? ?? ?? ??*/
	}
	virtual void GetVar(void) const
	{
		g_pAIPathClusters = g_GameDll.FindPatternSIMD("F3 0F 10 52 ?? 4C 8B CA")
			.FindPatternSelf("48 8B 35", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CUtlVector<CAI_Cluster*>*>();
		g_pAIClusterLinks = g_GameDll.FindPatternSIMD("F3 0F 10 52 ?? 4C 8B CA")
			.FindPatternSelf("4C 8B 1D", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CUtlVector<CAI_ClusterLink*>*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const;
	virtual void Detach(void) const;
};
///////////////////////////////////////////////////////////////////////////////
