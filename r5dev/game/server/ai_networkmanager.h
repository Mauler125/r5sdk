//=============================================================================//
//
// Purpose:
//
//=============================================================================//
#pragma once
#include "game/server/ai_network.h"
#include "game/server/detour_impl.h"
#include "baseentity.h"

/* ==== CAI_NETWORKMANAGER ============================================================================================================================================== */
inline CMemory p_CAI_NetworkManager__DelayedInit = nullptr;
inline void*(*CAI_NetworkManager__DelayedInit)(void* thisptr, CAI_Network* pNetwork);
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
inline CMemory p_CAI_NetworkManager__LoadNetworkGraph = nullptr;
inline void*(*CAI_NetworkManager__LoadNetworkGraph)(void* thisptr, void* pBuffer, const char* pszFileName);
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
inline CMemory p_CAI_NetworkManager__LoadNetworkGraph = nullptr;
inline void*(*CAI_NetworkManager__LoadNetworkGraph)(void* thisptr, void* pBuffer, const char* pszFileName);
#endif
/* ==== CAI_NETWORKBUILDER ============================================================================================================================================== */
inline CMemory p_CAI_NetworkBuilder__Build;
inline void*(*CAI_NetworkBuilder__Build)(void* thisptr, CAI_Network* pNetwork);

inline CUtlVector<CAI_Cluster*>* g_pAIPathClusters = nullptr;
inline CUtlVector<CAI_ClusterLink*>* g_pAIClusterLinks = nullptr;

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CAI_NetworkManager;

//-----------------------------------------------------------------------------
// CAI_NetworkEditTools
//
// Purpose: Bridge class to Hammer node editing functionality
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class CAI_NetworkEditTools
{
public:
	//-----------------
	// WC Editing
	//-----------------
	int m_nNextWCIndex;
	Vector3D* m_pWCPosition;

	//-----------------
	// Debugging Tools
	//-----------------
	int m_debugNetOverlays;
	CAI_Node** m_pNodes; // either nodes or node links.

	//-----------------
	// Network pointers
	//-----------------
	CAI_NetworkManager* m_pManager;
	CAI_Network* m_pNetwork;
};

//-----------------------------------------------------------------------------
// CAI_NetworkBuilder
// 
// Purpose: Wrapper class for building and saving network graphs
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class CAI_NetworkBuilder
{
public:
	static void Build(CAI_NetworkBuilder* pBuilder, CAI_Network* pAINetwork);
	static void SaveNetworkGraph(CAI_Network* pNetwork);
};

//-----------------------------------------------------------------------------
// CAI_NetworkManager
// 
// Purpose: The entity in the level responsible for building the network if it
//          isn't there, saving & loading of the network, and holding the 
//          CAI_Network instance.
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class CAI_NetworkManager : public CBaseEntity
{
public:
	static void LoadNetworkGraph(CAI_NetworkManager* pAINetworkManager, void* pBuffer, const char* szAIGraphFile);
	static void LoadNetworkGraphEx(CAI_NetworkManager* pAINetworkManager, void* pBuffer, const char* szAIGraphFile);

private:
	// !TODO[ AMOS ]: If found, change to ptr and hook up to engine!
	//static bool				gm_fNetworksLoaded;							// Have AINetworks been loaded

	void* _vftable;
	CAI_NetworkEditTools* m_pEditOps;
	CAI_Network* m_pNetwork;
	bool m_fInitalized;
	bool m_bDontSaveGraph;
	char gap_b22[2];
	int m_ainVersion;
	int m_ainMapFilesCRC;
	int m_runtimeCreatedAINMapFilesCRC;
	bool m_calculatedRuntimeAINMapFilesCRC;
	char gap_b31[7];
	/*ThreadedGraphBuildData*/ char m_ThreadedBuild[72];
};

///////////////////////////////////////////////////////////////////////////////
class VAI_NetworkManager : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CAI_NetworkManager::LoadNetworkGraph", p_CAI_NetworkManager__LoadNetworkGraph.GetPtr());
		LogFunAdr("CAI_NetworkManager::DelayedInit", p_CAI_NetworkManager__DelayedInit.GetPtr());
		LogFunAdr("CAI_NetworkBuilder::Build", p_CAI_NetworkBuilder__Build.GetPtr());
		LogVarAdr("g_AIPathClusters< CAI_Cluster* >", reinterpret_cast<uintptr_t>(g_pAIPathClusters));
		LogVarAdr("g_AIClusterLinks< CAI_ClusterLink* >", reinterpret_cast<uintptr_t>(g_pAIClusterLinks));
	}
	virtual void GetFun(void) const
	{
		p_CAI_NetworkManager__DelayedInit = g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 48 8B D9 48 8B 0D ?? ?? ?? ?? 8B 41 6C");
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CAI_NetworkManager__LoadNetworkGraph = g_GameDll.FindPatternSIMD("4C 89 44 24 ?? 48 89 4C 24 ?? 55 53 57 41 54 41 55 41 56");
		CAI_NetworkManager__LoadNetworkGraph = p_CAI_NetworkManager__LoadNetworkGraph.RCast<void* (*)(void*, void*, const char*)>();
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CAI_NetworkManager__LoadNetworkGraph = g_GameDll.FindPatternSIMD("4C 89 44 24 ?? 48 89 4C 24 ?? 55 53 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ?? 48 81 EC ?? ?? ?? ?? 48 8B FA");
		CAI_NetworkManager__LoadNetworkGraph = p_CAI_NetworkManager__LoadNetworkGraph.RCast<void* (*)(void*, void*, const char*)>();
#endif
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CAI_NetworkBuilder__Build = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 4C 24 ?? 57 41 54 41 55 41 56 41 57 48 83 EC 30 48 63 BA ?? ?? ?? ??");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CAI_NetworkBuilder__Build = g_GameDll.FindPatternSIMD("48 89 54 24 ?? 48 89 4C 24 ?? 53 55 56 57 41 54 41 55 41 56 41 57 48 83 EC 38 8B B2 ?? ?? ?? ??");
#endif
		CAI_NetworkManager__DelayedInit = p_CAI_NetworkManager__DelayedInit.RCast<void* (*)(void*, CAI_Network*)>();
		CAI_NetworkBuilder__Build       = p_CAI_NetworkBuilder__Build.RCast<void* (*)(void*, CAI_Network*)>();
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
