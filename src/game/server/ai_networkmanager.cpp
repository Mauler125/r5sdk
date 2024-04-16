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
constexpr int AINET_MINIMUM_SIZE          = 82; // The file is at least this large when all required fields are written
constexpr const char* AINETWORK_EXT       = ".ain";
constexpr const char* AINETWORK_PATH      = "maps/graphs/";


static ConVar ai_ainDumpOnLoad("ai_ainDumpOnLoad", "0", FCVAR_DEVELOPMENTONLY, "Dumps AIN data from node graphs loaded from the disk on load");

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

	// Must be computed at this point.
	Assert((*g_ppAINetworkManager)->IsRuntimeCRCCalculated());

	// Large NavMesh CRC.
	DevMsg(eDLL_T::SERVER, " |-- AINet version: '%d'\n", AINET_VERSION_NUMBER);
	DevMsg(eDLL_T::SERVER, " |-- Map version: '%d'\n", g_ServerGlobalVariables->m_nMapVersion);
	DevMsg(eDLL_T::SERVER, " |-- Runtime CRC: '0x%lX'\n", (*g_ppAINetworkManager)->GetRuntimeCRC());

	CUtlBuffer buf;

	// ---------------------------
	// Save the version numbers
	// ---------------------------
	buf.PutInt(AINET_VERSION_NUMBER);
	buf.PutInt(g_ServerGlobalVariables->m_nMapVersion);
	buf.PutInt((*g_ppAINetworkManager)->GetRuntimeCRC());

	timer.End();
	Msg(eDLL_T::SERVER, "...done writing header. %lf seconds\n", timer.GetDuration().GetSeconds());

	timer.Start();
	DevMsg(eDLL_T::SERVER, "+- Writing path nodes...\n");

	// -------------------------------
	// Dump all the nodes to the file
	// -------------------------------
	buf.PutInt(pNetwork->NumPathNodes());
	int totalNumLinks = 0;

	for (int node = 0; node < pNetwork->NumPathNodes(); node++)
	{
		const CAI_Node* aiNode = pNetwork->GetPathNode(node);

		DevMsg(eDLL_T::SERVER, " |-- Writing node '#%d' at '0x%zX'\n", aiNode->m_iID, buf.TellPut());

		buf.Put(&aiNode->m_vOrigin, sizeof(Vector3D));
		buf.PutFloat(aiNode->GetYaw());
		buf.Put(&aiNode->m_flVOffset, sizeof(aiNode->m_flVOffset));
		buf.PutChar((char)aiNode->GetType());
		buf.PutInt(aiNode->GetInfo());

		for (int j = 0; j < MAX_HULLS; j++)
		{
			buf.PutShort((short)aiNode->unk2[j]);
		}

		buf.Put(&aiNode->unk3, sizeof(aiNode->unk3));
		buf.PutShort(aiNode->unk6);
		buf.PutShort(aiNode->unk9); // Always -1;
		buf.Put(&aiNode->unk11, sizeof(aiNode->unk11));

		totalNumLinks += aiNode->NumLinks();
	}

	pNetwork->m_iNumLinks = totalNumLinks;

	timer.End();
	Msg(eDLL_T::SERVER, "...done writing path nodes. %lf seconds (%d nodes)\n", timer.GetDuration().GetSeconds(), pNetwork->m_iNumNodes);
	
	timer.Start();
	DevMsg(eDLL_T::SERVER, "+- Writing node links...\n");
	DevMsg(eDLL_T::SERVER, " |-- Cached node link count: '%d'\n", totalNumLinks);

	// -------------------------------
	// Dump all the links to the file
	// -------------------------------
	int packedLinks = totalNumLinks / 2;
	buf.PutInt(packedLinks);

	for (int node = 0; node < pNetwork->NumPathNodes(); node++)
	{
		const CAI_Node* aiNode = pNetwork->GetPathNode(node);

		for (int link = 0; link < aiNode->NumLinks(); link++)
		{
			const CAI_NodeLink* nodeLink = aiNode->GetLinkByIndex(link);

			// Skip links that don't originate from current node.
			if (nodeLink->m_iSrcID == aiNode->m_iID)
			{
				DevMsg(eDLL_T::SERVER, "  |-- Writing link (%hd <--> %hd) at '0x%zX'\n", nodeLink->m_iSrcID, nodeLink->m_iDestID, buf.TellPut());

				buf.PutShort(nodeLink->m_iSrcID);
				buf.PutShort(nodeLink->m_iDestID);

				buf.PutChar(nodeLink->unk1);
				buf.Put(nodeLink->m_iAcceptedMoveTypes, sizeof(nodeLink->m_iAcceptedMoveTypes));
			}
		}
	}

	timer.End();
	Msg(eDLL_T::SERVER, "...done writing node links. %lf seconds (%d links)\n", timer.GetDuration().GetSeconds(), pNetwork->m_iNumLinks);

	timer.Start();
	DevMsg(eDLL_T::SERVER, "+- Writing wc lookup table...\n");

	// -------------------------------
	// Dump the WC lookup table
	// -------------------------------
	CUtlMap<int, int> wcIDs;
	SetDefLessFunc(wcIDs);
	bool bCheckForProblems = false;

	const CAI_NetworkEditTools* const pEditOps = (*g_ppAINetworkManager)->GetEditOps();

	for (int node = 0; node < pNetwork->m_iNumNodes; node++)
	{
		const int nIndex = pEditOps->m_pNodeIndexTable[node];
		const int iPreviousNodeBinding = wcIDs.Find(nIndex);

		if (iPreviousNodeBinding != wcIDs.InvalidIndex())
		{
			if (!bCheckForProblems)
			{
				DevWarning(eDLL_T::SERVER, "******* MAP CONTAINS DUPLICATE HAMMER NODE IDS! CHECK FOR PROBLEMS IN HAMMER TO CORRECT *******\n");
				bCheckForProblems = true;
			}
			DevWarning(eDLL_T::SERVER, "   AI node %d is associated with Hammer node %d, but %d is already bound to node %d\n",
				node, nIndex, nIndex, wcIDs[(unsigned short)nIndex]);
		}
		else
		{
			wcIDs.Insert(nIndex, node);
		}

		DevMsg(eDLL_T::SERVER, " |-- Writing Hammer node (%d <--> %d) at '0x%zX'\n", nIndex, wcIDs.Element((unsigned short)nIndex), buf.TellPut());
		buf.PutInt(nIndex);
	}

	timer.End();
	Msg(eDLL_T::SERVER, "...done writing wc lookup table. %lf seconds (%d indices)\n", timer.GetDuration().GetSeconds(), wcIDs.Count());

	timer.Start();
	DevMsg(eDLL_T::SERVER, "+- Writing traverse ex nodes...\n");

	// -------------------------------
	// Dump the traverse ex nodes
	// -------------------------------
	const int traverseExNodeCount = g_pAITraverseNodes->Count();
	buf.PutShort((short)traverseExNodeCount);

	FOR_EACH_VEC(*g_pAITraverseNodes, i)
	{
		DevMsg(eDLL_T::SERVER, " |-- Writing traverse ex node '%d' at '0x%zX'\n", i, buf.TellPut());

		const CAI_TraverseNode& traverseExNode = (*g_pAITraverseNodes)[i];
		buf.Put(&traverseExNode.m_Quat, sizeof(Quaternion));
		buf.PutInt(traverseExNode.m_Index_MAYBE);
	}

	timer.End();
	Msg(eDLL_T::SERVER, "...done writing traverse ex nodes. %lf seconds (%d nodes)\n", timer.GetDuration().GetSeconds(), traverseExNodeCount);


	timer.Start();
	DevMsg(eDLL_T::SERVER, "+- Writing hull data blocks...\n");

	// -------------------------------
	// Dump the hull data blocks
	// -------------------------------

	for (int i = 0; i < MAX_HULLS; i++)
	{
		const CAI_HullData& hullData = pNetwork->m_HullData[i];
		const int numHullZones = pNetwork->m_iNumZones[i];

		const unsigned short numHullBits = (unsigned short)hullData.m_bitVec.GetNumBits();
		const unsigned short numHullInts = (unsigned short)hullData.m_bitVec.GetNumDWords();

		buf.PutInt(numHullZones);
		buf.PutUnsignedShort(numHullBits);
		buf.PutUnsignedShort(numHullInts);
		buf.Put(hullData.m_bitVec.Base(), numHullInts * sizeof(int));
	}

	timer.End();
	Msg(eDLL_T::SERVER, "...done writing hull data blocks. %lf seconds (%d blocks)\n", timer.GetDuration().GetSeconds(), MAX_HULLS);


	timer.Start();
	DevMsg(eDLL_T::SERVER, "+- Writing path clusters...\n");

	// -------------------------------
	// Dump the path clusters
	// -------------------------------
	const int numClusters = g_pAIPathClusters->Count();
	buf.PutInt(numClusters);

	FOR_EACH_VEC(*g_pAIPathClusters, i)
	{
		DevMsg(eDLL_T::SERVER, " |-- Writing cluster '#%d' at '0x%zX'\n", i, buf.TellPut());

		const CAI_Cluster* pathClusters = (*g_pAIPathClusters)[i];

		buf.PutInt(pathClusters->m_nIndex);
		buf.PutChar(pathClusters->unk1);

		buf.PutFloat(pathClusters->GetOrigin().x);
		buf.PutFloat(pathClusters->GetOrigin().y);
		buf.PutFloat(pathClusters->GetOrigin().z);

		const int unkVec0Size = pathClusters->unkVec0.Count();
		buf.PutInt(unkVec0Size);

		FOR_EACH_VEC(pathClusters->unkVec0, j)
		{
			short unkShort = static_cast<short>(pathClusters->unkVec0[j]);
			buf.PutShort(unkShort);
		}

		const int unkVec1Size = pathClusters->unkVec1.Count();
		buf.PutInt(unkVec1Size);

		FOR_EACH_VEC(pathClusters->unkVec1, j)
		{
			short unkShort = static_cast<short>(pathClusters->unkVec1[j]);
			buf.PutShort(unkShort);
		}

		buf.PutChar(pathClusters->unk5);
	}

	timer.End();
	Msg(eDLL_T::SERVER, "...done writing path clusters. %lf seconds (%d clusters)\n", timer.GetDuration().GetSeconds(), numClusters);

	timer.Start();
	DevMsg(eDLL_T::SERVER, "+- Writing cluster links...\n");

	// -------------------------------
	// Dump the cluster links
	// -------------------------------
	const int numClusterLinks = g_pAIClusterLinks->Count();
	buf.PutInt(numClusterLinks);

	FOR_EACH_VEC(*g_pAIClusterLinks, i)
	{
		// Disk and memory structs are literally identical here so just directly write.
		const CAI_ClusterLink* clusterLink = (*g_pAIClusterLinks)[i];

		DevMsg(eDLL_T::SERVER, "  |-- Writing link (%hd <--> %hd) at '0x%zX'\n", clusterLink->m_iSrcID, clusterLink->m_iDestID, buf.TellPut());

		buf.PutShort(clusterLink->m_iSrcID);
		buf.PutShort(clusterLink->m_iDestID);

		buf.PutInt(clusterLink->unk2);
		buf.PutChar(clusterLink->flags);

		buf.PutChar(clusterLink->unkFlags4);
		buf.PutChar(clusterLink->unkFlags5);
	}

	timer.End();
	Msg(eDLL_T::SERVER, "...done writing cluster links. %lf seconds (%d links)\n", timer.GetDuration().GetSeconds(), numClusterLinks);

	// This is always set to '-1'. Likely a field for maintaining compatibility.
	buf.PutInt(pNetwork->unk5);

	// AIN v57 and above only (not present in r1, static array in r2, pointer to dynamic array in r5).
	timer.Start();
	DevMsg(eDLL_T::SERVER, "+- Writing script nodes...\n");

	// -------------------------------
	// Dump all the script nodes
	// -------------------------------
	const int numScriptNodes = pNetwork->m_iNumScriptNodes;
	buf.PutInt(numScriptNodes);

	for (int node = 0; node < numScriptNodes; node++)
	{
		// Disk and memory structs for script nodes are identical.
		DevMsg(eDLL_T::SERVER, " |-- Writing script node '#%d' at '0x%zX'\n", node, buf.TellPut());
		buf.Put(&pNetwork->m_ScriptNode[node], sizeof(CAI_ScriptNode));
	}

	timer.End();
	Msg(eDLL_T::SERVER, "...done writing script nodes. %lf seconds (%d nodes)\n", timer.GetDuration().GetSeconds(), numScriptNodes);

	timer.Start();
	DevMsg(eDLL_T::SERVER, "+- Writing hint data...\n");

	// -------------------------------
	// Dump the hint data
	// -------------------------------
	const int numHinst = pNetwork->m_iNumHints;
	buf.PutInt(numHinst);

	for (int hint = 0; hint < numHinst; hint++)
	{
		DevMsg(eDLL_T::SERVER, " |-- Writing hint data '#%d' at '0x%zX'\n", hint, buf.TellPut());
		buf.PutShort(pNetwork->m_Hints[hint]);
	}

	timer.End();
	Msg(eDLL_T::SERVER, "...done writing hint data. %lf seconds (%d hints)\n", timer.GetDuration().GetSeconds(), numHinst);

	timer.Start();
	DevMsg(eDLL_T::SERVER, "+- Calculating navmesh crc...\n");

	// -------------------------------
	// Dump NavMesh CRC
	// -------------------------------
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

	// Note: the NavMesh checksum is written at the END of the file
	// to maintain compatibility with r1 and r2 AIN's.
	DevMsg(eDLL_T::SERVER, " |-- Writing navmesh crc '%x' at '0x%zX'\n", nNavMeshCRC, buf.TellPut());
	buf.PutInt(nNavMeshCRC);

	timer.End();
	Msg(eDLL_T::SERVER, "...done calculating navmesh crc. %lf seconds (%x)\n", timer.GetDuration().GetSeconds(), nNavMeshCRC);

	// Write the entire buffer to the disk.
	FileSystem()->Write(buf.Base(), buf.TellPut(), pAIGraph);
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
void CAI_NetworkManager::LoadNetworkGraph(CAI_NetworkManager* pManager, CUtlBuffer* pBuffer, const char* szAIGraphFile)
{
	bool bNavMeshAvailable = true;

	char szMeshPath[MAX_PATH];
	char szGraphPath[MAX_PATH];

	V_snprintf(szMeshPath, sizeof(szMeshPath), "%s%s_%s%s", NAVMESH_PATH, g_ServerGlobalVariables->m_pszMapName, S_HULL_TYPE[E_HULL_TYPE::LARGE], NAVMESH_EXT);
	V_snprintf(szGraphPath, sizeof(szGraphPath), "%s%s%s", AINETWORK_PATH, g_ServerGlobalVariables->m_pszMapName, AINETWORK_EXT);

	int nAiNetVersion = NULL;
	int nAiMapVersion = NULL;

	uint32_t nAiGraphCRC = NULL;   // AIN CRC from AIN file.
	uint32_t nAiNavMeshCRC = NULL; // NavMesh CRC from AIN file.
	uint32_t nNavMeshCRC = NULL;   // NavMesh CRC from local NM file.
	uint32_t nAiRuntimeCRC = pManager->GetRuntimeCRC();

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

		nNavMeshCRC = crc32::update(NULL, pBuf.get(), nLen);
	}

	const ssize_t nFileSize = pBuffer->TellPut();
	const ssize_t nOldOffset = pBuffer->TellGet();

	// Seek to the start of the buffer so we can validate the header.
	pBuffer->SeekGet(CUtlBuffer::SEEK_HEAD, 0);

	// If we have a NavMesh, then the minimum size is
	// 'AINET_MINIMUM_SIZE' + CRC32 as the AIN needs
	// a NavMesh checksum field for validation.
	const int nMinimumFileSize = bNavMeshAvailable
		? AINET_MINIMUM_SIZE + sizeof(nNavMeshCRC)
		: AINET_MINIMUM_SIZE;

	if (nFileSize >= nMinimumFileSize)
	{
		nAiNetVersion = pBuffer->GetInt();
		nAiMapVersion = pBuffer->GetInt();
		nAiGraphCRC = pBuffer->GetInt();

		// Too old; build with a different game???
		if (nAiNetVersion > AINET_VERSION_NUMBER)
		{
			Warning(eDLL_T::SERVER, "AI node graph '%s' is unsupported (net version: '%d' expected: '%d')\n", 
				szGraphPath, nAiNetVersion, AINET_VERSION_NUMBER);
		}
		// AIN file was build with a different version of the map, therefore,
		// the path node positions might be invalid.
		else if (nAiMapVersion != g_ServerGlobalVariables->m_nMapVersion)
		{
			Warning(eDLL_T::SERVER, "AI node graph '%s' is out of date (map version: '%d' expected: '%d')\n", 
				szGraphPath, nAiMapVersion, g_ServerGlobalVariables->m_nMapVersion);
		}
		// Data checksum is now what the runtime expects.
		else if (nAiGraphCRC != nAiRuntimeCRC)
		{
			Warning(eDLL_T::SERVER, "AI node graph '%s' is out of date (ain checksum: '%x' expected: '%x')\n", 
				szGraphPath, nAiGraphCRC, nAiRuntimeCRC);
		}
		else if (bNavMeshAvailable)
		{
			// Seek to the end of the file, minus the size of the CRC field.
			// The NavMesh CRC is written at the end of the file to maintain
			// compatibility with r1 and r2 AIN files.
			pBuffer->SeekGet(CUtlBuffer::SEEK_HEAD, nFileSize - sizeof(nAiNavMeshCRC));
			nAiNavMeshCRC = pBuffer->GetInt();

			// The AIN file was build with a different NavMesh, therefore,
			// the script node positions might be incorrect.
			if (nAiNavMeshCRC != nNavMeshCRC)
			{
				Warning(eDLL_T::SERVER, "AI node graph '%s' is out of date (nav checksum: '%x' expected: '%x')\n",
					szGraphPath, nAiGraphCRC, nNavMeshCRC);
			}
		}
	}
	else
	{
		Error(eDLL_T::SERVER, NO_ERROR, "%s - AI node graph '%s' appears truncated\n", __FUNCTION__, szGraphPath);
	}

	// Recover old buffer position before we call LoadNetworkGraph.
	pBuffer->SeekGet(CUtlBuffer::SEEK_HEAD, nOldOffset);
	LoadNetworkGraphEx(pManager, pBuffer, szAIGraphFile);
}

/*
==============================
CAI_NetworkManager::LoadNetworkGraphEx

  Load network
  (internal)
==============================
*/
void CAI_NetworkManager::LoadNetworkGraphEx(CAI_NetworkManager* pManager, CUtlBuffer* pBuffer, const char* szAIGraphFile)
{
	CAI_NetworkManager__LoadNetworkGraph(pManager, pBuffer, szAIGraphFile);

	if (ai_ainDumpOnLoad.GetBool())
	{
		Msg(eDLL_T::SERVER, "Dumping AI Network '%s'\n", szAIGraphFile);
		CAI_NetworkBuilder::SaveNetworkGraph(pManager->m_pNetwork);
	}
}

/*
==============================
CAI_NetworkBuilder::Build

  builds network in-memory
  during level load
==============================
*/
void CAI_NetworkBuilder::Build(CAI_NetworkBuilder* pBuilder, CAI_Network* pAINetwork)
{
	CAI_NetworkBuilder__Build(pBuilder, pAINetwork);
	CAI_NetworkBuilder::SaveNetworkGraph(pAINetwork);
}

void VAI_NetworkManager::Detour(const bool bAttach) const
{
	DetourSetup(&CAI_NetworkManager__LoadNetworkGraph, &CAI_NetworkManager::LoadNetworkGraph, bAttach);
	DetourSetup(&CAI_NetworkBuilder__Build, &CAI_NetworkBuilder::Build, bAttach);
}
