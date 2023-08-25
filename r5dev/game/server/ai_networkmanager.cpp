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
#include "filesystem/filesystem.h"
#include "game/server/ai_node.h"
#include "game/server/ai_network.h"
#include "game/server/ai_networkmanager.h"
#include <public/worldsize.h>

constexpr int AINET_SCRIPT_VERSION_NUMBER = 21;
constexpr int AINET_VERSION_NUMBER        = 57;
constexpr int AINET_MIN_FILE_SIZE         = 82;
constexpr const char* AINETWORK_EXT       = ".ain";
constexpr const char* AINETWORK_PATH      = "maps/graphs/";

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
	//int test = 0;

	//while (pNetwork->m_iNumNodes < 8000)
	//{
	//	Vector3D origin;

	//	origin.Init(
	//		RandomFloat(0.0f, MAX_COORD_FLOAT),
	//		RandomFloat(0.0f, MAX_COORD_FLOAT),
	//		RandomFloat(0.0f, MAX_COORD_FLOAT));

	//	/*CAI_Node* pNode = */pNetwork->AddPathNode(&origin, RandomFloat(-180, 180));

	//	if (test > 0)
	//		pNetwork->CreateNodeLink(test-1, test);

	//	CAI_NodeCluster* cluster = new CAI_NodeCluster();
	//	memset(cluster, '\0', sizeof(CAI_NodeCluster));

	//	origin.Init(
	//		RandomFloat(0.0f, MAX_COORD_FLOAT),
	//		RandomFloat(0.0f, MAX_COORD_FLOAT),
	//		RandomFloat(0.0f, MAX_COORD_FLOAT));

	//	cluster->m_nIndex = test;
	//	cluster->m_vOrigin = origin;

	//	g_pAINodeClusters->AddToTail(cluster);

	//	CAI_NodeClusterLink* clusterLink = new CAI_NodeClusterLink();
	//	memset(clusterLink, '\0', sizeof(CAI_NodeClusterLink));

	//	clusterLink->prevIndex_MAYBE = (short)test;
	//	clusterLink->nextIndex_MAYBE = (short)test + 1;

	//	clusterLink->flags = 4;

	//	g_pAINodeClusterLinks->AddToTail(clusterLink);
	//	++test;
	//}

	char szMeshPath[MAX_PATH];
	char szGraphPath[MAX_PATH];

	V_snprintf(szMeshPath,  sizeof(szMeshPath), "%s%s_%s%s", NAVMESH_PATH, g_ServerGlobalVariables->m_pszMapName, S_HULL_TYPE[E_HULL_TYPE::LARGE], NAVMESH_EXT);
	V_snprintf(szGraphPath, sizeof(szGraphPath), "%s%s%s", AINETWORK_PATH, g_ServerGlobalVariables->m_pszMapName, AINETWORK_EXT);

	CFastTimer masterTimer;
	CFastTimer timer;

	// Build from memory.
	Msg(eDLL_T::SERVER, "++++--------------------------------------------------------------------------------------------------------------------------++++\n");
	Msg(eDLL_T::SERVER, ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> AI NETWORK GRAPH FILE CONSTRUCTION STARTED <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
	Msg(eDLL_T::SERVER, "++++--------------------------------------------------------------------------------------------------------------------------++++\n");

	masterTimer.Start();
	timer.Start();

	FileSystem()->CreateDirHierarchy(AINETWORK_PATH, "GAME");
	FileHandle_t pAIGraph = FileSystem()->Open(szGraphPath, "wb", "GAME");
	if (!pAIGraph)
	{
		Error(eDLL_T::SERVER, NO_ERROR, "%s - Unable to write to '%s' (read-only?)\n", __FUNCTION__, szGraphPath);
		return;
	}

	DevMsg(eDLL_T::SERVER, "+- Writing header...\n");
	DevMsg(eDLL_T::SERVER, " |-- AINet version: '%d'\n", AINET_VERSION_NUMBER);
	FileSystem()->Write(&AINET_VERSION_NUMBER, sizeof(int), pAIGraph);

	DevMsg(eDLL_T::SERVER, " |-- Map version: '%d'\n", g_ServerGlobalVariables->m_nMapVersion);
	FileSystem()->Write(&g_ServerGlobalVariables->m_nMapVersion, sizeof(int), pAIGraph);

	FileHandle_t pNavMesh = FileSystem()->Open(szMeshPath, "rb", "GAME");
	uint32_t nNavMeshCRC = NULL;

	if (!pNavMesh)
	{
		Warning(eDLL_T::SERVER, "%s - No %s NavMesh found. Unable to calculate CRC for AI Network\n",
			__FUNCTION__, S_HULL_TYPE[E_HULL_TYPE::LARGE]);
	}
	else
	{
		const ssize_t nLen = FileSystem()->Size(pNavMesh);
		std::unique_ptr<uint8_t[]> pBuf(new uint8_t[nLen]);

		FileSystem()->Read(pBuf.get(), nLen, pNavMesh);
		FileSystem()->Close(pNavMesh);

		nNavMeshCRC = crc32::update(NULL, pBuf.get(), nLen);
	}

	// Large NavMesh CRC.
	DevMsg(eDLL_T::SERVER, " |-- NavMesh CRC: '0x%lX'\n", nNavMeshCRC);
	FileSystem()->Write(&nNavMeshCRC, sizeof(uint32_t), pAIGraph);

	// Path nodes.
	DevMsg(eDLL_T::SERVER, " |-- Node count: '%d'\n", pNetwork->m_iNumNodes);
	FileSystem()->Write(&pNetwork->m_iNumNodes, sizeof(int), pAIGraph);

	timer.End();
	Msg(eDLL_T::SERVER, "...done writing header. %lf seconds\n", timer.GetDuration().GetSeconds());

	timer.Start();
	DevMsg(eDLL_T::SERVER, "+- Writing node positions...\n");

	if (pNetwork->m_pAInode)
	{
		int totalLinkCount = 0;

		for (int i = 0; i < pNetwork->m_iNumNodes; i++)
		{
			const CAI_Node* aiNode = pNetwork->m_pAInode[i];

			// Construct on-disk node struct.
			CAI_NodeDisk diskNode;

			diskNode.m_vOrigin = aiNode->m_vOrigin;
			diskNode.m_flYaw = aiNode->m_flYaw;

			diskNode.unk0 = static_cast<char>(aiNode->unk0);
			diskNode.unk1 = aiNode->unk1;

			for (int j = 0; j < MAX_HULLS; j++)
			{
				diskNode.unk2[j] = static_cast<short>(aiNode->unk2[j]);
				diskNode.unk3[j] = aiNode->unk3[j];
				diskNode.hulls[j] = aiNode->m_fHulls[j];
			}

			diskNode.unk4 = aiNode->unk6;
			diskNode.unk5 = -1; // aiNetwork->nodes[i]->unk8; // This field is wrong, however it's always -1 in original navmeshes anyway.
			memcpy(diskNode.unk6, aiNode->unk10, sizeof(diskNode.unk6));

			DevMsg(eDLL_T::SERVER, " |-- Copying node '#%d' from '0x%p' to '0x%zX'\n", aiNode->m_nIndex, aiNode, FileSystem()->Tell(pAIGraph));
			FileSystem()->Write(&diskNode, sizeof(CAI_NodeDisk), pAIGraph);

			totalLinkCount += aiNode->m_nNumLinks;
		}

		pNetwork->m_iNumLinks = totalLinkCount;
	}
	else
	{
		// No links, this has to be initialized as the engine doesn't do it
		// during build.
		pNetwork->m_iNumLinks = 0;
	}

	timer.End();
	Msg(eDLL_T::SERVER, "...done writing node positions. %lf seconds\n", timer.GetDuration().GetSeconds());
	
	timer.Start();
	DevMsg(eDLL_T::SERVER, "+- Writing links...\n");
	DevMsg(eDLL_T::SERVER, " |-- Cached link count: '%d'\n", pNetwork->m_iNumLinks);

	int packedLinks = pNetwork->m_iNumLinks / 2;
	FileSystem()->Write(&packedLinks, sizeof(int), pAIGraph);

	if (pNetwork->m_pAInode)
	{
		for (int i = 0; i < pNetwork->m_iNumNodes; i++)
		{
			const CAI_Node* aiNode = pNetwork->m_pAInode[i];

			for (int j = 0; j < aiNode->m_nNumLinks; j++)
			{
				const CAI_NodeLink* nodeLink = aiNode->links[j];

				// Skip links that don't originate from current node.
				if (nodeLink->m_iSrcID != aiNode->m_nIndex)
				{
					continue;
				}

				CAI_NodeLinkDisk diskLink;

				diskLink.m_iSrcID = nodeLink->m_iSrcID;
				diskLink.m_iDestID = nodeLink->m_iDestID;
				diskLink.unk0 = nodeLink->unk1;

				for (int k = 0; k < MAX_HULLS; k++)
				{
					diskLink.m_bHulls[k] = nodeLink->m_bHulls[k];
				}

				DevMsg(eDLL_T::SERVER, "  |-- Writing link '%hd' => '%hd' to '0x%zX'\n", diskLink.m_iSrcID, diskLink.m_iDestID, FileSystem()->Tell(pAIGraph));
				FileSystem()->Write(&diskLink, sizeof(CAI_NodeLinkDisk), pAIGraph);
			}
		}
	}

	timer.End();
	Msg(eDLL_T::SERVER, "...done writing links. %lf seconds (%d links)\n", timer.GetDuration().GetSeconds(), pNetwork->m_iNumLinks);

	timer.Start();
	DevMsg(eDLL_T::SERVER, "+- Writing hull data...\n");
	// Don't know what this is, it's likely a block from tf1 that got deprecated? should just be 1 int per node.
	DevMsg(eDLL_T::SERVER, " |-- Writing '%d' bytes for node block at '0x%zX'\n", pNetwork->m_iNumNodes * sizeof(uint32_t), FileSystem()->Tell(pAIGraph));

	if (pNetwork->m_iNumNodes > 0)
	{
		std::unique_ptr<uint32[]> unkNodeBlock(new uint32_t[pNetwork->m_iNumNodes * sizeof(uint32_t)]);
		memset(unkNodeBlock.get(), '\0', pNetwork->m_iNumNodes * sizeof(uint32_t));

		FileSystem()->Write(unkNodeBlock.get(), pNetwork->m_iNumNodes * sizeof(uint32_t), pAIGraph);
	}

	// TODO: This is traverse nodes i think? these aren't used in r2 ains so we can get away with just writing count=0 and skipping
	// but ideally should actually dump these.
	DevMsg(eDLL_T::SERVER, " |-- Writing '%d' traversal nodes at '0x%zX'\n", 0, FileSystem()->Tell(pAIGraph));

	short traverseNodeCount = 0; // Only write count since count=0 means we don't have to actually do anything here.
	FileSystem()->Write(&traverseNodeCount, sizeof(short), pAIGraph);

	// TODO: Ideally these should be actually dumped, but they're always 0 in r2 from what i can tell.
	DevMsg(eDLL_T::SERVER, " |-- Writing '%d' bytes for hull data block at '0x%zX'\n", (MAX_HULLS * 8), FileSystem()->Tell(pAIGraph));
	for (int i = 0; i < (MAX_HULLS * 8); i++)
	{
		FileSystem()->Write("\0", sizeof(char), pAIGraph);
	}

	timer.End();
	Msg(eDLL_T::SERVER, "...done writing hull data. %lf seconds\n", timer.GetDuration().GetSeconds());

	timer.Start();
	DevMsg(eDLL_T::SERVER, "+- Writing clusters...\n");

	const int numClusters = g_pAIPathClusters->Count();
	FileSystem()->Write(&numClusters, sizeof(int), pAIGraph);

	FOR_EACH_VEC(*g_pAIPathClusters, i)
	{
		DevMsg(eDLL_T::SERVER, " |-- Writing cluster '#%d' at '0x%zX'\n", i, FileSystem()->Tell(pAIGraph));

		const CAI_Cluster* pathClusters = (*g_pAIPathClusters)[i];

		FileSystem()->Write(&pathClusters->m_nIndex, sizeof(int), pAIGraph);
		FileSystem()->Write(&pathClusters->unk1, sizeof(char), pAIGraph);

		FileSystem()->Write(&pathClusters->m_vOrigin.x, sizeof(vec_t), pAIGraph);
		FileSystem()->Write(&pathClusters->m_vOrigin.y, sizeof(vec_t), pAIGraph);
		FileSystem()->Write(&pathClusters->m_vOrigin.z, sizeof(vec_t), pAIGraph);

		const int unkVec0Size = pathClusters->unkVec0.Count();
		FileSystem()->Write(&unkVec0Size, sizeof(int), pAIGraph);

		FOR_EACH_VEC(pathClusters->unkVec0, j)
		{
			short unkShort = static_cast<short>(pathClusters->unkVec0[j]);
			FileSystem()->Write(&unkShort, sizeof(short), pAIGraph);
		}

		const int unkVec1Size = pathClusters->unkVec1.Count();
		FileSystem()->Write(&unkVec1Size, sizeof(int), pAIGraph);

		FOR_EACH_VEC(pathClusters->unkVec1, j)
		{
			short unkShort = static_cast<short>(pathClusters->unkVec0[j]);
			FileSystem()->Write(&unkShort, sizeof(short), pAIGraph);
		}

		FileSystem()->Write(&pathClusters->unk5, sizeof(char), pAIGraph);
	}

	timer.End();
	Msg(eDLL_T::SERVER, "...done writing clusters. %lf seconds (%d clusters)\n", timer.GetDuration().GetSeconds(), numClusters);

	timer.Start();
	DevMsg(eDLL_T::SERVER, "+- Writing cluster links...\n");

	const int numClusterLinks = g_pAIClusterLinks->Count();
	FileSystem()->Write(&numClusterLinks, sizeof(int), pAIGraph);

	FOR_EACH_VEC(*g_pAIClusterLinks, i)
	{
		DevMsg(eDLL_T::SERVER, " |-- Writing cluster link '#%d' at '0x%zX'\n", i, FileSystem()->Tell(pAIGraph));

		// Disk and memory structs are literally identical here so just directly write.
		const CAI_ClusterLink* clusterLink = (*g_pAIClusterLinks)[i];

		FileSystem()->Write(&clusterLink->prevIndex_MAYBE, sizeof(short), pAIGraph);
		FileSystem()->Write(&clusterLink->nextIndex_MAYBE, sizeof(short), pAIGraph);

		FileSystem()->Write(&clusterLink->unk2, sizeof(int), pAIGraph);
		FileSystem()->Write(&clusterLink->flags, sizeof(char), pAIGraph);

		FileSystem()->Write(&clusterLink->unk4, sizeof(char), pAIGraph);
		FileSystem()->Write(&clusterLink->unk5, sizeof(char), pAIGraph);
	}

	timer.End();
	Msg(eDLL_T::SERVER, "...done writing cluster links. %lf seconds (%d cluster links)\n", timer.GetDuration().GetSeconds(), numClusterLinks);

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
	Msg(eDLL_T::SERVER, "...done writing script nodes. %lf seconds (%d nodes)\n", timer.GetDuration().GetSeconds(), pNetwork->m_iNumScriptNodes);

	timer.Start();
	DevMsg(eDLL_T::SERVER, "+- Writing hint data...\n");

	FileSystem()->Write(&pNetwork->m_iNumHints, sizeof(pNetwork->m_iNumHints), pAIGraph);
	for (int i = 0; i < pNetwork->m_iNumHints; i++)
	{
		DevMsg(eDLL_T::SERVER, " |-- Writing hint data '#%d' at '0x%zX'\n", i, FileSystem()->Tell(pAIGraph));
		FileSystem()->Write(&pNetwork->m_Hints[i], sizeof(pNetwork->m_Hints[i]), pAIGraph);
	}

	timer.End();
	Msg(eDLL_T::SERVER, "...done writing hint data. %lf seconds (%d hints)\n", timer.GetDuration().GetSeconds(), pNetwork->m_iNumHints);

	FileSystem()->Close(pAIGraph);

	masterTimer.End();
	Msg(eDLL_T::SERVER, "...done writing AI node graph. %lf seconds\n", masterTimer.GetDuration().GetSeconds());
	Msg(eDLL_T::SERVER, "++++--------------------------------------------------------------------------------------------------------------------------++++\n");
	Msg(eDLL_T::SERVER, "++++--------------------------------------------------------------------------------------------------------------------------++++\n");
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
	bool bNavMeshAvailable = true;

	char szMeshPath[MAX_PATH];
	char szGraphPath[MAX_PATH];

	V_snprintf(szMeshPath, sizeof(szMeshPath), "%s%s_%s%s", NAVMESH_PATH, g_ServerGlobalVariables->m_pszMapName, S_HULL_TYPE[E_HULL_TYPE::LARGE], NAVMESH_EXT);
	V_snprintf(szGraphPath, sizeof(szGraphPath), "%s%s%s", AINETWORK_PATH, g_ServerGlobalVariables->m_pszMapName, AINETWORK_EXT);

	int nAiNetVersion = NULL;
	int nAiMapVersion = NULL;

	uint32_t nAiGraphHash = NULL;
	uint32_t nNavMeshHash = NULL;

	FileHandle_t pNavMesh = FileSystem()->Open(szMeshPath, "rb", "GAME");
	if (!pNavMesh)
	{
		Warning(eDLL_T::SERVER, "%s - No %s NavMesh found. Unable to calculate CRC for AI Network\n", __FUNCTION__, S_HULL_TYPE[E_HULL_TYPE::LARGE]);
		bNavMeshAvailable = false;
	}
	else
	{
		const ssize_t nLen = FileSystem()->Size(pNavMesh);
		std::unique_ptr<uint8_t[]> pBuf(new uint8_t[nLen]);

		FileSystem()->Read(pBuf.get(), nLen, pNavMesh);
		FileSystem()->Close(pNavMesh);

		nNavMeshHash = crc32::update(NULL, pBuf.get(), nLen);
	}

	FileHandle_t pAIGraph = FileSystem()->Open(szGraphPath, "rb", "GAME");
	if (!pAIGraph)
	{
		Error(eDLL_T::SERVER, NO_ERROR, "%s - Unable to open '%s' (insufficient rights?)\n", __FUNCTION__, szGraphPath);
		LoadNetworkGraphEx(pAINetworkManager, pBuffer, szAIGraphFile);

		return;
	}

	if (FileSystem()->Size(pAIGraph) >= AINET_MIN_FILE_SIZE)
	{
		FileSystem()->Read(&nAiNetVersion, sizeof(int), pAIGraph);
		FileSystem()->Read(&nAiMapVersion, sizeof(int), pAIGraph);
		FileSystem()->Read(&nAiGraphHash, sizeof(int), pAIGraph);

		if (nAiNetVersion > AINET_VERSION_NUMBER)
		{
			Warning(eDLL_T::SERVER, "AI node graph '%s' is unsupported (net version: '%d' expected: '%d')\n", 
				szGraphPath, nAiNetVersion, AINET_VERSION_NUMBER);
		}
		else if (nAiMapVersion != g_ServerGlobalVariables->m_nMapVersion)
		{
			Warning(eDLL_T::SERVER, "AI node graph '%s' is out of date (map version: '%d' expected: '%d')\n", 
				szGraphPath, nAiMapVersion, g_ServerGlobalVariables->m_nMapVersion);
		}
		else if (bNavMeshAvailable && nAiGraphHash != nNavMeshHash)
		{
			Warning(eDLL_T::SERVER, "AI node graph '%s' is out of date (checksum: '%x' expected: '%x')\n", 
				szGraphPath, nAiGraphHash, nNavMeshHash);
		}
	}
	else
	{
		Error(eDLL_T::SERVER, NO_ERROR, "%s - AI node graph '%s' is corrupt\n", __FUNCTION__, szGraphPath);
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
		Msg(eDLL_T::SERVER, "Reparsing AI Network '%s'\n", szAIGraphFile);
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

void VAI_NetworkManager::Attach() const
{
	DetourAttach((LPVOID*)&CAI_NetworkManager__LoadNetworkGraph, &CAI_NetworkManager::LoadNetworkGraph);
	DetourAttach((LPVOID*)&CAI_NetworkBuilder__Build, &CAI_NetworkBuilder::Build);
}

void VAI_NetworkManager::Detach() const
{
	DetourDetach((LPVOID*)&CAI_NetworkManager__LoadNetworkGraph, &CAI_NetworkManager::LoadNetworkGraph);
	DetourDetach((LPVOID*)&CAI_NetworkBuilder__Build, &CAI_NetworkBuilder::Build);
}
