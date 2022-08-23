//=============================================================================//
//
// Purpose:
//
//=============================================================================//

#include "core/stdafx.h"
#include "tier0/fasttimer.h"
#include "tier1/cvar.h"
#include "tier1/cmd.h"
#include "mathlib/crc32.h"
#include "public/edict.h"
#include "public/utility/utility.h"
#include "filesystem/filesystem.h"
#include "engine/host_state.h"
#include "game/server/ai_node.h"
#include "game/server/ai_network.h"
#include "game/server/ai_networkmanager.h"

constexpr int AINET_SCRIPT_VERSION_NUMBER = 21;
constexpr int AINET_VERSION_NUMBER        = 57;
constexpr int AINETWORK_MIN_SIZE          = 82;

/*
==============================
CAI_NetworkBuilder::BuildFile

  Build AI node graph file from
  in-memory structures and write
  to disk to be loaded
==============================
*/
void CAI_NetworkBuilder::SaveNetworkGraph(CAI_Network* pNetwork)
{
	const string svMeshDir = "maps/navmesh/";
	const string svGraphDir = "maps/graphs/";

	fs::path fsMeshPath(svMeshDir + g_pHostState->m_levelName + "_" + SHULL_SIZE[EHULL_SIZE::LARGE] + ".nm");
	fs::path fsGraphPath(svGraphDir + g_pHostState->m_levelName + ".ain");

	CFastTimer masterTimer;
	CFastTimer timer;

	// Build from memory.
	DevMsg(eDLL_T::SERVER, "++++--------------------------------------------------------------------------------------------------------------------------++++\n");
	DevMsg(eDLL_T::SERVER, ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> AI NETWORK GRAPH FILE CONSTRUCTION STARTED <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
	DevMsg(eDLL_T::SERVER, "++++--------------------------------------------------------------------------------------------------------------------------++++\n");

	masterTimer.Start();
	timer.Start();

	FileHandle_t pAIGraph = FileSystem()->Open(fsGraphPath.relative_path().u8string().c_str(), "wb", "GAME");
	if (!pAIGraph)
	{
		Error(eDLL_T::SERVER, false, "%s - Unable to write to '%s' (read-only?)\n", __FUNCTION__, fsGraphPath.relative_path().u8string().c_str());
		return;
	}

	DevMsg(eDLL_T::SERVER, "+- Writing header...\n");
	DevMsg(eDLL_T::SERVER, " |-- AINet version: '%d'\n", AINET_VERSION_NUMBER);
	FileSystem()->Write(&AINET_VERSION_NUMBER, sizeof(int), pAIGraph);

	DevMsg(eDLL_T::SERVER, " |-- Map version: '%d'\n", g_ServerGlobalVariables->m_nMapVersion);
	FileSystem()->Write(&g_ServerGlobalVariables->m_nMapVersion, sizeof(int), pAIGraph);

	FileHandle_t pNavMesh = FileSystem()->Open(fsMeshPath.relative_path().u8string().c_str(), "rb", "GAME");
	uint32_t nNavMeshHash = NULL;

	if (!pNavMesh)
	{
		Warning(eDLL_T::SERVER, "%s - No %s NavMesh found. Unable to calculate CRC for AI Network.\n", __FUNCTION__, SHULL_SIZE[EHULL_SIZE::LARGE].c_str());
	}
	else
	{
		uint32_t nLen = FileSystem()->Size(pNavMesh);
		uint8_t* pBuf = MemAllocSingleton()->Alloc<uint8_t>(nLen);

		FileSystem()->Read(pBuf, nLen, pNavMesh);
		FileSystem()->Close(pNavMesh);

		nNavMeshHash = crc32::update(NULL, pBuf, nLen);
		MemAllocSingleton()->Free(pBuf);
	}

	// Large NavMesh CRC.
	DevMsg(eDLL_T::SERVER, " |-- NavMesh CRC: '%lx'\n", nNavMeshHash);
	FileSystem()->Write(&nNavMeshHash, sizeof(uint32_t), pAIGraph);

	// Path nodes.
	DevMsg(eDLL_T::SERVER, " |-- Nodecount: '%d'\n", pNetwork->m_iNumNodes);
	FileSystem()->Write(&pNetwork->m_iNumNodes, sizeof(int), pAIGraph);

	timer.End();
	DevMsg(eDLL_T::SERVER, "...done writing header. %lf seconds\n", timer.GetDuration().GetSeconds());

	timer.Start();
	DevMsg(eDLL_T::SERVER, "+- Writing node positions...\n");

	int nCalculatedLinkcount = 0;

	if (pNetwork->m_pAInode)
	{
		for (int i = 0; i < pNetwork->m_iNumNodes; i++)
		{
			// Construct on-disk node struct.
			CAI_NodeDisk diskNode{};
			diskNode.m_vOrigin.x = pNetwork->m_pAInode[i]->m_vOrigin.x;
			diskNode.m_vOrigin.y = pNetwork->m_pAInode[i]->m_vOrigin.y;
			diskNode.m_vOrigin.z = pNetwork->m_pAInode[i]->m_vOrigin.z;
			diskNode.m_flYaw = pNetwork->m_pAInode[i]->m_flYaw;
			memcpy(diskNode.hulls, pNetwork->m_pAInode[i]->m_fHulls, sizeof(diskNode.hulls));
			diskNode.unk0 = static_cast<char>(pNetwork->m_pAInode[i]->unk0);
			diskNode.unk1 = pNetwork->m_pAInode[i]->unk1;

			for (int j = 0; j < MAX_HULLS; j++)
			{
				diskNode.unk2[j] = static_cast<short>(pNetwork->m_pAInode[i]->unk2[j]);
			}

			memcpy(diskNode.unk3, pNetwork->m_pAInode[i]->unk3, sizeof(diskNode.unk3));
			diskNode.unk4 = pNetwork->m_pAInode[i]->unk6;
			diskNode.unk5 = -1; // aiNetwork->nodes[i]->unk8; // This field is wrong, however it's always -1 in original navmeshes anyway.
			memcpy(diskNode.unk6, pNetwork->m_pAInode[i]->unk10, sizeof(diskNode.unk6));


			DevMsg(eDLL_T::SERVER, " |-- Copying node '#%d' from '0x%p' to '0x%zX'\n", pNetwork->m_pAInode[i]->m_nIndex, reinterpret_cast<void*>(pNetwork->m_pAInode[i]), FileSystem()->Tell(pAIGraph));
			FileSystem()->Write(&diskNode, sizeof(CAI_NodeDisk), pAIGraph);

			nCalculatedLinkcount += pNetwork->m_pAInode[i]->m_nNumLinks;
		}
	}
	timer.End();
	DevMsg(eDLL_T::SERVER, "...done writing node positions. %lf seconds\n", timer.GetDuration().GetSeconds());
	
	timer.Start();
	DevMsg(eDLL_T::SERVER, "+- Writing links...\n");

	DevMsg(eDLL_T::SERVER, " |-- Cache linkcount: '%d'\n", pNetwork->m_iNumLinks);
	DevMsg(eDLL_T::SERVER, " |-- Calculated linkcount: '%d'\n", nCalculatedLinkcount);

	nCalculatedLinkcount /= 2;
	if (ai_ainDumpOnLoad->GetBool())
	{
		if (pNetwork->m_iNumLinks != nCalculatedLinkcount)
		{
			Warning(eDLL_T::SERVER, "%s - Calculated linkcount '%d' doesn't match file linkcount '%d' (expected on build!)\n", __FUNCTION__, nCalculatedLinkcount, pNetwork->m_iNumLinks);
		}
	}

	FileSystem()->Write(&nCalculatedLinkcount, sizeof(int), pAIGraph);

	if (pNetwork->m_pAInode)
	{
		for (int i = 0; i < pNetwork->m_iNumNodes; i++)
		{
			for (int j = 0; j < pNetwork->m_pAInode[i]->m_nNumLinks; j++)
			{
				// Skip links that don't originate from current node.
				if (pNetwork->m_pAInode[i]->links[j]->m_iSrcID != pNetwork->m_pAInode[i]->m_nIndex)
				{
					continue;
				}

				CAI_NodeLinkDisk diskLink{};
				diskLink.m_iSrcID = pNetwork->m_pAInode[i]->links[j]->m_iSrcID;
				diskLink.m_iDestID = pNetwork->m_pAInode[i]->links[j]->m_iDestID;
				diskLink.unk0 = pNetwork->m_pAInode[i]->links[j]->unk1;
				memcpy(diskLink.m_bHulls, pNetwork->m_pAInode[i]->links[j]->m_bHulls, sizeof(diskLink.m_bHulls));

				DevMsg(eDLL_T::SERVER, "  |-- Writing link '%h' => '%h' to '0x%zX'\n", diskLink.m_iSrcID, diskLink.m_iDestID, FileSystem()->Tell(pAIGraph));
				FileSystem()->Write(&diskLink, sizeof(CAI_NodeLinkDisk), pAIGraph);
			}
		}
	}

	timer.End();
	DevMsg(eDLL_T::SERVER, "...done writing links. %lf seconds (%d links)\n", timer.GetDuration().GetSeconds(), nCalculatedLinkcount);

	timer.Start();
	DevMsg(eDLL_T::SERVER, "+- Writing hull data...\n");
	// Don't know what this is, it's likely a block from tf1 that got deprecated? should just be 1 int per node.
	DevMsg(eDLL_T::SERVER, " |-- Writing '%d' bytes for node block at '0x%zX'\n", pNetwork->m_iNumNodes * sizeof(uint32_t), FileSystem()->Tell(pAIGraph));

	if (pNetwork->m_iNumNodes > 0)
	{
		uint32_t* unkNodeBlock = MemAllocSingleton()->Alloc<uint32_t>(pNetwork->m_iNumNodes * sizeof(uint32_t));
		memset(&unkNodeBlock, '\0', pNetwork->m_iNumNodes * sizeof(uint32_t));

		FileSystem()->Write(&*unkNodeBlock, pNetwork->m_iNumNodes * sizeof(uint32_t), pAIGraph);
		MemAllocSingleton()->Free(unkNodeBlock);
	}

	// TODO: This is traverse nodes i think? these aren't used in r2 ains so we can get away with just writing count=0 and skipping
	// but ideally should actually dump these.
	DevMsg(eDLL_T::SERVER, " |-- Writing '%d' traversal nodes at '0x%zX'\n", 0, FileSystem()->Tell(pAIGraph));
	short traverseNodeCount = 0; // Only write count since count=0 means we don't have to actually do anything here.
	FileSystem()->Write(&traverseNodeCount, sizeof(short), pAIGraph);

	// TODO: Ideally these should be actually dumped, but they're always 0 in r2 from what i can tell.
	DevMsg(eDLL_T::SERVER, " |-- Writing '%d' bytes for hull data block at '0x%zX'\n", MAX_HULLS * 8, FileSystem()->Tell(pAIGraph));
	for (int i = 0; i < (MAX_HULLS * 8); i++)
	{
		FileSystem()->Write("\0", sizeof(char), pAIGraph);
	}

	timer.End();
	DevMsg(eDLL_T::SERVER, "...done writing hull data. %lf seconds\n", timer.GetDuration().GetSeconds());

	timer.Start();
	DevMsg(eDLL_T::SERVER, "+- Writing clusters...\n");

	FileSystem()->Write(&*g_nAiNodeClusters, sizeof(*g_nAiNodeClusters), pAIGraph);
	for (int i = 0; i < *g_nAiNodeClusters; i++)
	{
		DevMsg(eDLL_T::SERVER, " |-- Writing cluster '#%d' at '0x%zX'\n", i, FileSystem()->Tell(pAIGraph));
		AINodeClusters* nodeClusters = (*g_pppAiNodeClusters)[i];

		FileSystem()->Write(&nodeClusters->m_nIndex, sizeof(nodeClusters->m_nIndex), pAIGraph);
		FileSystem()->Write(&nodeClusters->unk1, sizeof(nodeClusters->unk1), pAIGraph);

		FileSystem()->Write(&nodeClusters->m_vOrigin.x, sizeof(nodeClusters->m_vOrigin.x), pAIGraph);
		FileSystem()->Write(&nodeClusters->m_vOrigin.y, sizeof(nodeClusters->m_vOrigin.y), pAIGraph);
		FileSystem()->Write(&nodeClusters->m_vOrigin.z, sizeof(nodeClusters->m_vOrigin.z), pAIGraph);

		FileSystem()->Write(&nodeClusters->unkcount0, sizeof(nodeClusters->unkcount0), pAIGraph);
		for (int j = 0; j < nodeClusters->unkcount0; j++)
		{
			short unk2Short = static_cast<short>(nodeClusters->unk2[j]);
			FileSystem()->Write(&unk2Short, sizeof(unk2Short), pAIGraph);
		}

		FileSystem()->Write(&nodeClusters->unkcount1, sizeof(nodeClusters->unkcount1), pAIGraph);
		for (int j = 0; j < nodeClusters->unkcount1; j++)
		{
			short unk3Short = static_cast<short>(nodeClusters->unk3[j]);
			FileSystem()->Write(&unk3Short, sizeof(unk3Short), pAIGraph);
		}

		FileSystem()->Write(&nodeClusters->unk5, sizeof(nodeClusters->unk5), pAIGraph);
	}

	timer.End();
	DevMsg(eDLL_T::SERVER, "...done writing clusters. %lf seconds (%d clusters)\n", timer.GetDuration().GetSeconds(), *g_nAiNodeClusters);

	timer.Start();
	DevMsg(eDLL_T::SERVER, "+- Writing cluster links...\n");

	FileSystem()->Write(&*g_nAiNodeClusterLinks, sizeof(*g_nAiNodeClusterLinks), pAIGraph);
	for (int i = 0; i < *g_nAiNodeClusterLinks; i++)
	{
		// Disk and memory structs are literally identical here so just directly write.
		DevMsg(eDLL_T::SERVER, " |-- Writing cluster link '#%d' at '0x%zX'\n", i, FileSystem()->Tell(pAIGraph));
		FileSystem()->Write(&*g_pppAiNodeClusterLinks[i], sizeof(*(*g_pppAiNodeClusterLinks)[i]), pAIGraph);
	}

	timer.End();
	DevMsg(eDLL_T::SERVER, "...done writing cluster links. %lf seconds (%d cluster links)\n", timer.GetDuration().GetSeconds(), *g_nAiNodeClusterLinks);

	// This is always set to '-1'. Likely a field for maintaining compatibility.
	FileSystem()->Write(&pNetwork->unk5, sizeof(pNetwork->unk5), pAIGraph);

	// AIN v57 and above only (not present in r1, static array in r2, pointer to dynamic array in r5).
	timer.Start();
	DevMsg(eDLL_T::SERVER, "+- Writing script nodes...\n");

	FileSystem()->Write(&pNetwork->m_iNumScriptNodes, sizeof(pNetwork->m_iNumScriptNodes), pAIGraph);
	for (int i = 0; i < pNetwork->m_iNumScriptNodes; i++)
	{
		// Disk and memory structs for script nodes are identical.
		DevMsg(eDLL_T::SERVER, " |-- Writing script node '#%d' at '0x%zX'\n", i, FileSystem()->Tell(pAIGraph));
		FileSystem()->Write(&pNetwork->m_ScriptNode[i], sizeof(CAI_ScriptNode), pAIGraph);
	}

	timer.End();
	DevMsg(eDLL_T::SERVER, "...done writing script nodes. %lf seconds (%d nodes)\n", timer.GetDuration().GetSeconds(), pNetwork->m_iNumScriptNodes);

	timer.Start();
	DevMsg(eDLL_T::SERVER, "+- Writing hint data...\n");

	FileSystem()->Write(&pNetwork->m_iNumHints, sizeof(pNetwork->m_iNumHints), pAIGraph);
	for (int i = 0; i < pNetwork->m_iNumHints; i++)
	{
		DevMsg(eDLL_T::SERVER, " |-- Writing hint data '#%d' at '0x%zX'\n", i, FileSystem()->Tell(pAIGraph));
		FileSystem()->Write(&pNetwork->m_Hints[i], sizeof(pNetwork->m_Hints[i]), pAIGraph);
	}

	timer.End();
	DevMsg(eDLL_T::SERVER, "...done writing hint data. %lf seconds (%d hints)\n", timer.GetDuration().GetSeconds(), pNetwork->m_iNumHints);

	FileSystem()->Close(pAIGraph);

	masterTimer.End();
	DevMsg(eDLL_T::SERVER, "...done writing AI node graph. %lf seconds\n", masterTimer.GetDuration().GetSeconds());
	DevMsg(eDLL_T::SERVER, "++++--------------------------------------------------------------------------------------------------------------------------++++\n");
	DevMsg(eDLL_T::SERVER, "++++--------------------------------------------------------------------------------------------------------------------------++++\n");
}

/*
==============================
CAI_NetworkManager::LoadNetworkGraph

  Load network from the disk
  and validate status
==============================
*/
void CAI_NetworkManager::LoadNetworkGraph(CAI_NetworkManager* pAINetworkManager, void* pBuffer, const char* szAIGraphFile)
{
	string svMeshDir = "maps/navmesh/";
	string svGraphDir = "maps/graphs/";

	fs::path fsMeshPath(svMeshDir + g_pHostState->m_levelName + "_" + SHULL_SIZE[EHULL_SIZE::LARGE] + ".nm");
	fs::path fsGraphPath(svGraphDir + g_pHostState->m_levelName + ".ain");

	int nAiNetVersion = NULL;
	int nAiMapVersion = NULL;
	bool bNavMeshAvailable = true;

	uint32_t nAiGraphHash = NULL;
	uint32_t nNavMeshHash = NULL;

	FileHandle_t pNavMesh = FileSystem()->Open(fsMeshPath.relative_path().u8string().c_str(), "rb", "GAME");
	if (!pNavMesh)
	{
		Warning(eDLL_T::SERVER, "%s - No %s NavMesh found. Unable to calculate CRC for AI Network.\n", __FUNCTION__, SHULL_SIZE[EHULL_SIZE::LARGE].c_str());
		bNavMeshAvailable = false;
	}
	else
	{
		uint32_t nLen = FileSystem()->Size(pNavMesh);
		uint8_t* pBuf = MemAllocSingleton()->Alloc<uint8_t>(nLen);

		FileSystem()->Read(pBuf, nLen, pNavMesh);
		FileSystem()->Close(pNavMesh);

		nNavMeshHash = crc32::update(NULL, pBuf, nLen);
		MemAllocSingleton()->Free(pBuf);
	}

	FileHandle_t pAIGraph = FileSystem()->Open(fsGraphPath.relative_path().u8string().c_str(), "rb", "GAME");
	if (!pAIGraph)
	{
		Error(eDLL_T::SERVER, false, "%s - Unable to open '%s' (insufficient rights?)\n", __FUNCTION__, 
			fsGraphPath.relative_path().u8string().c_str());
		LoadNetworkGraphEx(pAINetworkManager, pBuffer, szAIGraphFile);

		return;
	}
	if (FileSystem()->Size(pAIGraph) >= AINETWORK_MIN_SIZE)
	{
		FileSystem()->Read(&nAiNetVersion, sizeof(int), pAIGraph);
		FileSystem()->Read(&nAiMapVersion, sizeof(int), pAIGraph);
		FileSystem()->Read(&nAiGraphHash, sizeof(int), pAIGraph);

		if (nAiNetVersion > AINET_VERSION_NUMBER)
		{
			Warning(eDLL_T::SERVER, "AI node graph '%s' is unsupported (net version: '%d' supported: '%d')\n",
				fsGraphPath.relative_path().u8string().c_str(), nAiNetVersion, AINET_VERSION_NUMBER);
		}
		else if (nAiMapVersion != g_ServerGlobalVariables->m_nMapVersion)
		{
			Warning(eDLL_T::SERVER, "AI node graph '%s' is out of date (map version: '%d' expected: '%d')\n",
				fsGraphPath.relative_path().u8string().c_str(), nAiMapVersion, g_ServerGlobalVariables->m_nMapVersion);
		}
		else
		{
			if (bNavMeshAvailable)
			{
				if (nNavMeshHash != nAiGraphHash)
				{
					Warning(eDLL_T::SERVER, "AI node graph '%s' is out of date (checksum: '0x%X' expected: '0x%X')\n",
						fsGraphPath.relative_path().u8string().c_str(), nNavMeshHash, nAiGraphHash);
				}
			}
		}
	}
	else
	{
		Error(eDLL_T::SERVER, false, "%s - AI node graph '%s' is corrupt (LEN_BYTES < AINETWORK_MIN_SIZE)\n", __FUNCTION__, 
			fsGraphPath.relative_path().u8string().c_str());
	}

	FileSystem()->Close(pAIGraph);
	LoadNetworkGraphEx(pAINetworkManager, pBuffer, szAIGraphFile);
}

/*
==============================
CAI_NetworkManager::LoadNetworkGraphEx

  Load network
  (internal)
==============================
*/
void CAI_NetworkManager::LoadNetworkGraphEx(CAI_NetworkManager* pAINetworkManager, void* pBuffer, const char* szAIGraphFile)
{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	CAI_NetworkManager__LoadNetworkGraph(pAINetworkManager, pBuffer, szAIGraphFile, NULL);
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	CAI_NetworkManager__LoadNetworkGraph(pAINetworkManager, pBuffer, szAIGraphFile);
#endif

	if (ai_ainDumpOnLoad->GetBool())
	{
		DevMsg(eDLL_T::SERVER, "Running BuildAINFile for loaded AI node graph '%s'\n", szAIGraphFile);
		CAI_NetworkBuilder::SaveNetworkGraph(*(CAI_Network**)(reinterpret_cast<char*>(pAINetworkManager) + AINETWORK_OFFSET));
	}
}

/*
==============================
CAI_NetworkBuilder::Build

  builds network in-memory
  during level load
==============================
*/
void CAI_NetworkBuilder::Build(CAI_NetworkBuilder* pBuilder, CAI_Network* pAINetwork, void* a3, int a4)
{
	CAI_NetworkBuilder__Build(pBuilder, pAINetwork, a3, a4);
	CAI_NetworkBuilder::SaveNetworkGraph(pAINetwork);
}

void CAI_NetworkManager_Attach()
{
	DetourAttach((LPVOID*)&CAI_NetworkManager__LoadNetworkGraph, &CAI_NetworkManager::LoadNetworkGraph);
	DetourAttach((LPVOID*)&CAI_NetworkBuilder__Build, &CAI_NetworkBuilder::Build);
}

void CAI_NetworkManager_Detach()
{
	DetourDetach((LPVOID*)&CAI_NetworkManager__LoadNetworkGraph, &CAI_NetworkManager::LoadNetworkGraph);
	DetourDetach((LPVOID*)&CAI_NetworkBuilder__Build, &CAI_NetworkBuilder::Build);
}
