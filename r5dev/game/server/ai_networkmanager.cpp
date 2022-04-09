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
#include "public/include/edict.h"
#include "public/include/utility.h"
#include "engine/host_state.h"
#include "engine/sys_utils.h"
#include "game/server/ai_node.h"
#include "game/server/ai_network.h"
#include "game/server/ai_networkmanager.h"

constexpr int AINET_SCRIPT_VERSION_NUMBER = 21;
constexpr int AINET_VERSION_NUMBER        = 57;

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
	std::string svMeshDir = "maps\\navmesh\\";
	std::string svGraphDir = "maps\\graphs\\";

	std::filesystem::path fsMeshPath(svMeshDir + g_pHostState->m_levelName + "_" + HULL_SIZE[3] + ".nm");
	std::filesystem::path fsGraphPath(svGraphDir + g_pHostState->m_levelName + ".ain");

	CFastTimer masterTimer;
	CFastTimer timer;

	int nCalculatedLinkcount = 0;

	// Build from memory.
	DevMsg(eDLL_T::SERVER, "++++--------------------------------------------------------------------------------------------------------------------------++++\n");
	DevMsg(eDLL_T::SERVER, ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> AI NETWORK GRAPH FILE CONSTRUCTION STARTED <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
	DevMsg(eDLL_T::SERVER, "++++--------------------------------------------------------------------------------------------------------------------------++++\n");
	DevMsg(eDLL_T::SERVER, "+- Writing header...\n");

	masterTimer.Start();
	timer.Start();

	CreateDirectories(svGraphDir);

	std::ofstream writeStream(fsGraphPath, std::ofstream::binary);
	DevMsg(eDLL_T::SERVER, " |-- AINet version: '%d'\n", AINET_VERSION_NUMBER);
	writeStream.write(reinterpret_cast<const char*>(&AINET_VERSION_NUMBER), sizeof(int));

	int nMapVersion = g_ServerGlobalVariables->m_nMapVersion;
	DevMsg(eDLL_T::SERVER, " |-- Map version: '%d'\n", nMapVersion);
	writeStream.write(reinterpret_cast<char*>(&nMapVersion), sizeof(int));

	std::ifstream iNavMesh(fsMeshPath, std::fstream::binary);
	std::vector<std::uint8_t> uNavMesh;
	std::uint32_t nNavMeshHash = NULL;
	if (iNavMesh.good())
	{
		iNavMesh.seekg(0, std::fstream::end);
		uNavMesh.resize(iNavMesh.tellg());
		iNavMesh.seekg(0, std::fstream::beg);
		iNavMesh.read((char*)uNavMesh.data(), uNavMesh.size());

		nNavMeshHash = crc32::update(NULL, uNavMesh.data(), uNavMesh.size());
	}
	else
	{
		Warning(eDLL_T::SERVER, "%s - No %s NavMesh found. Unable to calculate CRC for AI Network\n", __FUNCTION__, HULL_SIZE[3].c_str());
	}

	// Large NavMesh CRC.
	DevMsg(eDLL_T::SERVER, " |-- NavMesh CRC: '%lx'\n", nNavMeshHash);
	writeStream.write(reinterpret_cast<const char*>(&nNavMeshHash), sizeof(int));

	// Path nodes.
	DevMsg(eDLL_T::SERVER, " |-- Nodecount: '%d'\n", pNetwork->m_iNumNodes);
	writeStream.write(reinterpret_cast<char*>(&pNetwork->m_iNumNodes), sizeof(int));

	timer.End();
	DevMsg(eDLL_T::SERVER, "...done writing header. %lf seconds\n", timer.GetDuration().GetSeconds());

	timer.Start();
	DevMsg(eDLL_T::SERVER, "+- Writing node positions...\n");

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


			DevMsg(eDLL_T::SERVER, " |-- Copying node '#%d' from '%p' to '0x%llx'\n", pNetwork->m_pAInode[i]->m_nIndex, reinterpret_cast<void*>(pNetwork->m_pAInode[i]), static_cast<size_t>(writeStream.tellp()));
			writeStream.write(reinterpret_cast<char*>(&diskNode), sizeof(CAI_NodeDisk));

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

	writeStream.write(reinterpret_cast<char*>(&nCalculatedLinkcount), sizeof(int));

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

				DevMsg(eDLL_T::SERVER, "  |-- Writing link '%d' => '%d' to '0x%llx'\n", diskLink.m_iSrcID, diskLink.m_iDestID, static_cast<size_t>(writeStream.tellp()));
				writeStream.write(reinterpret_cast<char*>(&diskLink), sizeof(CAI_NodeLinkDisk));
			}
		}
	}

	timer.End();
	DevMsg(eDLL_T::SERVER, "...done writing links. %lf seconds (%d links)\n", timer.GetDuration().GetSeconds(), nCalculatedLinkcount);

	timer.Start();
	DevMsg(eDLL_T::SERVER, "+- Writing hull data...\n");
	// Don't know what this is, it's likely a block from tf1 that got deprecated? should just be 1 int per node.
	DevMsg(eDLL_T::SERVER, " |-- Writing '%d' bytes for unknown block at '0x%llx'\n", pNetwork->m_iNumNodes * sizeof(uint32_t), static_cast<size_t>(writeStream.tellp()));

	if (static_cast<int>(pNetwork->m_iNumNodes) > 0)
	{
		uint32_t* unkNodeBlock = new uint32_t[pNetwork->m_iNumNodes];
		memset(&unkNodeBlock, '\0', pNetwork->m_iNumNodes * sizeof(uint32_t));
		writeStream.write(reinterpret_cast<char*>(*unkNodeBlock), pNetwork->m_iNumNodes * sizeof(uint32_t));
		delete[] unkNodeBlock;
	}

	// TODO: This is traverse nodes i think? these aren't used in r2 ains so we can get away with just writing count=0 and skipping
	// but ideally should actually dump these.
	DevMsg(eDLL_T::SERVER, " |-- Writing '%d' traversal nodes at '0x%llx'\n", 0, static_cast<size_t>(writeStream.tellp()));
	short traverseNodeCount = 0; // Only write count since count=0 means we don't have to actually do anything here.
	writeStream.write(reinterpret_cast<char*>(&traverseNodeCount), sizeof(short));

	// TODO: Ideally these should be actually dumped, but they're always 0 in r2 from what i can tell.
	DevMsg(eDLL_T::SERVER, " |-- Writing '%lld' bytes for unknown hull block at '0x%llx'\n", MAX_HULLS * 8, static_cast<size_t>(writeStream.tellp()));
	char* unkHullBlock = new char[MAX_HULLS * 8];
	memset(unkHullBlock, '\0', MAX_HULLS * 8);
	writeStream.write(unkHullBlock, MAX_HULLS * 8);
	delete[] unkHullBlock;

	timer.End();
	DevMsg(eDLL_T::SERVER, "...done writing hull data. %lf seconds\n", timer.GetDuration().GetSeconds());

	timer.Start();
	DevMsg(eDLL_T::SERVER, "+- Writing clusters...\n");

	writeStream.write(reinterpret_cast<char*>(g_nAiNodeClusters), sizeof(*g_nAiNodeClusters));
	for (int i = 0; i < *g_nAiNodeClusters; i++)
	{
		DevMsg(eDLL_T::SERVER, " |-- Writing cluster '#%d' at '0x%llx'\n", i, static_cast<size_t>(writeStream.tellp()));
		AINodeClusters* nodeClusters = (*g_pppAiNodeClusters)[i];

		writeStream.write(reinterpret_cast<char*>(&nodeClusters->m_nIndex), sizeof(nodeClusters->m_nIndex));
		writeStream.write(reinterpret_cast<char*>(&nodeClusters->unk1), sizeof(nodeClusters->unk1));

		writeStream.write(reinterpret_cast<char*>(&nodeClusters->m_vOrigin.x), sizeof(nodeClusters->m_vOrigin.x));
		writeStream.write(reinterpret_cast<char*>(&nodeClusters->m_vOrigin.y), sizeof(nodeClusters->m_vOrigin.y));
		writeStream.write(reinterpret_cast<char*>(&nodeClusters->m_vOrigin.z), sizeof(nodeClusters->m_vOrigin.z));

		writeStream.write(reinterpret_cast<char*>(&nodeClusters->unkcount0), sizeof(nodeClusters->unkcount0));
		for (int j = 0; j < nodeClusters->unkcount0; j++)
		{
			short unk2Short = static_cast<short>(nodeClusters->unk2[j]);
			writeStream.write(reinterpret_cast<char*>(&unk2Short), sizeof(unk2Short));
		}

		writeStream.write(reinterpret_cast<char*>(&nodeClusters->unkcount1), sizeof(nodeClusters->unkcount1));
		for (int j = 0; j < nodeClusters->unkcount1; j++)
		{
			short unk3Short = static_cast<short>(nodeClusters->unk3[j]);
			writeStream.write(reinterpret_cast<char*>(&unk3Short), sizeof(unk3Short));
		}

		writeStream.write(reinterpret_cast<char*>(&nodeClusters->unk5), sizeof(nodeClusters->unk5));
	}

	timer.End();
	DevMsg(eDLL_T::SERVER, "...done writing clusters. %lf seconds (%d clusters)\n", timer.GetDuration().GetSeconds(), *g_nAiNodeClusters);

	timer.Start();
	DevMsg(eDLL_T::SERVER, "+- Writing cluster links...\n");

	writeStream.write(reinterpret_cast<char*>(g_nAiNodeClusterLinks), sizeof(*g_nAiNodeClusterLinks));
	for (int i = 0; i < *g_nAiNodeClusterLinks; i++)
	{
		// Disk and memory structs are literally identical here so just directly write.
		DevMsg(eDLL_T::SERVER, " |-- Writing cluster link '#%d' at '0x%llx'\n", i, static_cast<size_t>(writeStream.tellp()));
		writeStream.write(reinterpret_cast<char*>((*g_pppAiNodeClusterLinks)[i]), sizeof(*(*g_pppAiNodeClusterLinks)[i]));
	}

	timer.End();
	DevMsg(eDLL_T::SERVER, "...done writing cluster links. %lf seconds (%d cluster links)\n", timer.GetDuration().GetSeconds(), *g_nAiNodeClusterLinks);

	// This is always set to '-1'. Likely a field for maintaining compatibility.
	writeStream.write(reinterpret_cast<char*>(&pNetwork->unk5), sizeof(pNetwork->unk5));

	// AIN v57 and above only (not present in r1, static array in r2, pointer to dynamic array in r5).
	timer.Start();
	DevMsg(eDLL_T::SERVER, "+- Writing script nodes...\n");

	writeStream.write(reinterpret_cast<char*>(&pNetwork->m_iNumScriptNodes), sizeof(pNetwork->m_iNumScriptNodes));
	for (int i = 0; i < pNetwork->m_iNumScriptNodes; i++)
	{
		// Disk and memory structs for script nodes are identical.
		DevMsg(eDLL_T::SERVER, " |-- Writing script node '#%d' at '0x%llx'\n", i, static_cast<size_t>(writeStream.tellp()));
		if (!IsBadReadPtrV2(reinterpret_cast<char*>(&pNetwork->m_ScriptNode[i])))
		{
			writeStream.write(reinterpret_cast<char*>(&pNetwork->m_ScriptNode[i]), sizeof(CAI_ScriptNode));
		}
		else
		{
			Warning(eDLL_T::SERVER, " |-- Unable to write node '#%d' (invalid pointer)\n", i, pNetwork->m_iNumScriptNodes);
		}
	}

	timer.End();
	DevMsg(eDLL_T::SERVER, "...done writing script nodes. %lf seconds (%d nodes)\n", timer.GetDuration().GetSeconds(), pNetwork->m_iNumScriptNodes);

	timer.Start();
	DevMsg(eDLL_T::SERVER, "+- Writing hint data...\n");

	writeStream.write(reinterpret_cast<char*>(&pNetwork->m_iNumHints), sizeof(pNetwork->m_iNumHints));
	for (int i = 0; i < pNetwork->m_iNumHints; i++)
	{
		DevMsg(eDLL_T::SERVER, " |-- Writing hint data '#%d' at '0x%llx'\n", i, static_cast<size_t>(writeStream.tellp()));
		writeStream.write(reinterpret_cast<char*>(&pNetwork->m_Hints[i]), sizeof(pNetwork->m_Hints[i]));
	}

	timer.End();
	DevMsg(eDLL_T::SERVER, "...done writing hint data. %lf seconds (%d hints)\n", timer.GetDuration().GetSeconds(), pNetwork->m_iNumHints);

	masterTimer.End();
	DevMsg(eDLL_T::SERVER, "...done writing AI node graph. %lf seconds\n", masterTimer.GetDuration().GetSeconds());
	DevMsg(eDLL_T::SERVER, "++++--------------------------------------------------------------------------------------------------------------------------++++\n");
	DevMsg(eDLL_T::SERVER, "++++--------------------------------------------------------------------------------------------------------------------------++++\n");

	writeStream.close();
}

void HCAI_NetworkManager__LoadNetworkGraph(void* aimanager, void* buf, const char* filename)
{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	CAI_NetworkManager__LoadNetworkGraph(aimanager, buf, filename, NULL);
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	CAI_NetworkManager__LoadNetworkGraph(aimanager, buf, filename);
#endif

	if (ai_ainDumpOnLoad->GetBool())
	{
		DevMsg(eDLL_T::SERVER, "Running BuildAINFile for loaded file '%s'\n", filename);
		CAI_NetworkBuilder::SaveNetworkGraph(*(CAI_Network**)(reinterpret_cast<char*>(aimanager) + AINETWORK_OFFSET));
	}
}

void HCAI_NetworkBuilder__Build(void* builder, CAI_Network* aiNetwork, void* a3, int a4)
{
	CAI_NetworkBuilder__Build(builder, aiNetwork, a3, a4);
	CAI_NetworkBuilder::SaveNetworkGraph(aiNetwork);
}

void CAI_NetworkManager_Attach()
{
	DetourAttach((LPVOID*)&CAI_NetworkManager__LoadNetworkGraph, &HCAI_NetworkManager__LoadNetworkGraph);
	DetourAttach((LPVOID*)&CAI_NetworkBuilder__Build, &HCAI_NetworkBuilder__Build);
}

void CAI_NetworkManager_Detach()
{
	DetourDetach((LPVOID*)&CAI_NetworkManager__LoadNetworkGraph, &HCAI_NetworkManager__LoadNetworkGraph);
	DetourDetach((LPVOID*)&CAI_NetworkBuilder__Build, &HCAI_NetworkBuilder__Build);
}
