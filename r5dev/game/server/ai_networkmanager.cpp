//=============================================================================//
//
// Purpose:
//
//=============================================================================//

#include "core/stdafx.h"
#include "public/include/edict.h"
#include "tier0/cvar.h"
#include "tier0/cmd.h"
#include "engine/host_state.h"
#include "engine/sys_utils.h"
#include "game/server/ai_node.h"
#include "game/server/ai_network.h"
#include "game/server/ai_networkmanager.h"

const unsigned int PLACEHOLDER_CRC    = 0;
const int AINET_SCRIPT_VERSION_NUMBER = 21;
const int AINET_VERSION_NUMBER        = 57;
const int MAP_VERSION_TEMP            = 30;

/*
==============================
CAI_NetworkBuilder::BuildFile

  Build AI node graph file from
  in-memory structures and write
  to disk to be loaded
==============================
*/
void CAI_NetworkBuilder::BuildFile(CAI_Network* pNetwork)
{
	std::filesystem::path fsWritePath("platform\\maps\\graphs\\");
	fsWritePath /= g_pHostState->m_levelName;
	fsWritePath += ".ain";

	// Dump from memory.
	DevMsg(eDLL_T::SERVER, "++++--------------------------------------------------------------------------------------------------------------------------++++\n");
	DevMsg(eDLL_T::SERVER, ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> AI NETWORK GRAPH FILE CONSTRUCTION STARTED <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
	DevMsg(eDLL_T::SERVER, "++++--------------------------------------------------------------------------------------------------------------------------++++\n");
	DevMsg(eDLL_T::SERVER, "Output file: '%s'\n", fsWritePath.string().c_str());

	std::ofstream writeStream(fsWritePath, std::ofstream::binary);
	DevMsg(eDLL_T::SERVER, "Writing AINet version: '%d'\n", AINET_VERSION_NUMBER);
	writeStream.write((char*)&AINET_VERSION_NUMBER, sizeof(int));

	int nMapVersion = g_ServerGlobalVariables->m_nMapVersion;
	DevMsg(eDLL_T::SERVER, "Writing map version: '%d'\n", nMapVersion);
	writeStream.write((char*)&nMapVersion, sizeof(int));

	DevMsg(eDLL_T::SERVER, "Writing placeholder CRC: '%d'\n", PLACEHOLDER_CRC);
	writeStream.write((char*)&PLACEHOLDER_CRC, sizeof(int));

	int nCalculatedLinkcount = 0;

	// Path nodes
	DevMsg(eDLL_T::SERVER, "Writing nodecount: '%d'\n", pNetwork->nodecount);
	writeStream.write((char*)&pNetwork->nodecount, sizeof(int));

	for (int i = 0; i < pNetwork->nodecount; i++)
	{
		sizeof(CAI_Network);

		// Construct on-disk node struct.
		CAI_NodeDisk diskNode{};
		diskNode.x = pNetwork->nodes[i]->x;
		diskNode.y = pNetwork->nodes[i]->y;
		diskNode.z = pNetwork->nodes[i]->z;
		diskNode.yaw = pNetwork->nodes[i]->yaw;
		memcpy(diskNode.hulls, pNetwork->nodes[i]->hulls, sizeof(diskNode.hulls));
		diskNode.unk0 = (char)pNetwork->nodes[i]->unk0;
		diskNode.unk1 = pNetwork->nodes[i]->unk1;

		for (int j = 0; j < MAX_HULLS; j++)
		{
			diskNode.unk2[j] = (short)pNetwork->nodes[i]->unk2[j];
			spdlog::info((short)pNetwork->nodes[i]->unk2[j]);
		}

		memcpy(diskNode.unk3, pNetwork->nodes[i]->unk3, sizeof(diskNode.unk3));
		diskNode.unk4 = pNetwork->nodes[i]->unk6;
		diskNode.unk5 = -1; // aiNetwork->nodes[i]->unk8; // This field is wrong, however it's always -1 in original navmeshes anyway.
		memcpy(diskNode.unk6, pNetwork->nodes[i]->unk10, sizeof(diskNode.unk6));

		DevMsg(eDLL_T::SERVER, "Writing node '%d' from '%d' to '%x'\n", pNetwork->nodes[i]->index, (void*)pNetwork->nodes[i], writeStream.tellp());
		writeStream.write((char*)&diskNode, sizeof(CAI_NodeDisk));

		nCalculatedLinkcount += pNetwork->nodes[i]->linkcount;
	}

	// links
	DevMsg(eDLL_T::SERVER, "Linkcount: '%d'\n", pNetwork->linkcount);
	DevMsg(eDLL_T::SERVER, "Calculated total linkcount: '%d'\n", nCalculatedLinkcount);

	nCalculatedLinkcount /= 2;
	if (ai_dumpAINfileFromLoad->GetBool())
	{
		if (pNetwork->linkcount == nCalculatedLinkcount)
		{
			DevMsg(eDLL_T::SERVER, "Caculated linkcount is normal!");
		}
		else
		{
			DevMsg(eDLL_T::SERVER, "Calculated linkcount has unexpected value. This is expected on build!");
		}
	}

	DevMsg(eDLL_T::SERVER, "Writing linkcount: '%d'\n", nCalculatedLinkcount);
	writeStream.write((char*)&nCalculatedLinkcount, sizeof(int));

	for (int i = 0; i < pNetwork->nodecount; i++)
	{
		for (int j = 0; j < pNetwork->nodes[i]->linkcount; j++)
		{
			// skip links that don't originate from current node
			if (pNetwork->nodes[i]->links[j]->srcId != pNetwork->nodes[i]->index)
				continue;

			CAI_NodeLinkDisk diskLink{};
			diskLink.srcId = pNetwork->nodes[i]->links[j]->srcId;
			diskLink.destId = pNetwork->nodes[i]->links[j]->destId;
			diskLink.unk0 = pNetwork->nodes[i]->links[j]->unk1;
			memcpy(diskLink.hulls, pNetwork->nodes[i]->links[j]->hulls, sizeof(diskLink.hulls));

			DevMsg(eDLL_T::SERVER, "Writing link '%d' => '%d' to '%x'\n", diskLink.srcId, diskLink.destId, writeStream.tellp());
			writeStream.write((char*)&diskLink, sizeof(CAI_NodeLinkDisk));
		}
	}

	// Don't know what this is, it's likely a block from tf1 that got deprecated? should just be 1 int per node
	DevMsg(eDLL_T::SERVER, "Writing '%x' bytes for unknown block at '%x'\n", pNetwork->nodecount * sizeof(uint32_t), writeStream.tellp());
	uint32_t* unkNodeBlock = new uint32_t[pNetwork->nodecount];
	memset(unkNodeBlock, 0, pNetwork->nodecount * sizeof(uint32_t));
	writeStream.write((char*)unkNodeBlock, pNetwork->nodecount * sizeof(uint32_t));
	delete[] unkNodeBlock;

	// TODO: this is traverse nodes i think? these aren't used in tf2 ains so we can get away with just writing count=0 and skipping
	// but ideally should actually dump these
	DevMsg(eDLL_T::SERVER, "Writing '%d' traversal nodes at '%x'\n", 0, writeStream.tellp());
	short traverseNodeCount = 0;
	writeStream.write((char*)&traverseNodeCount, sizeof(short));
	// Only write count since count=0 means we don't have to actually do anything here

	// TODO: ideally these should be actually dumped, but they're always 0 in tf2 from what i can tell
	DevMsg(eDLL_T::SERVER, "Writing '%d' bytes for unknown hull block at '%x'\n", MAX_HULLS * 8, writeStream.tellp());
	char* unkHullBlock = new char[MAX_HULLS * 8];
	memset(unkHullBlock, 0, MAX_HULLS * 8);
	writeStream.write(unkHullBlock, MAX_HULLS * 8);
	delete[] unkHullBlock;

	DevMsg(eDLL_T::SERVER, "Writing '%d' node clusters at '%x'\n", *g_nAiNodeClusters, writeStream.tellp());
	writeStream.write((char*)g_nAiNodeClusters, sizeof(*g_nAiNodeClusters));
	for (int i = 0; i < *g_nAiNodeClusters; i++)
	{
		DevMsg(eDLL_T::SERVER, "Writing unknown node struct '%d' at '%x'\n", i, writeStream.tellp());
		AINodeClusters* nodeClusters = (*g_pppAiNodeClusters)[i];

		writeStream.write((char*)&nodeClusters->index, sizeof(nodeClusters->index));
		writeStream.write((char*)&nodeClusters->unk1, sizeof(nodeClusters->unk1));

		writeStream.write((char*)&nodeClusters->x, sizeof(nodeClusters->x));
		writeStream.write((char*)&nodeClusters->y, sizeof(nodeClusters->y));
		writeStream.write((char*)&nodeClusters->z, sizeof(nodeClusters->z));

		writeStream.write((char*)&nodeClusters->unkcount0, sizeof(nodeClusters->unkcount0));
		for (int j = 0; j < nodeClusters->unkcount0; j++)
		{
			short unk2Short = (short)nodeClusters->unk2[j];
			writeStream.write((char*)&unk2Short, sizeof(unk2Short));
		}

		writeStream.write((char*)&nodeClusters->unkcount1, sizeof(nodeClusters->unkcount1));
		for (int j = 0; j < nodeClusters->unkcount1; j++)
		{
			short unk3Short = (short)nodeClusters->unk3[j];
			writeStream.write((char*)&unk3Short, sizeof(unk3Short));
		}

		writeStream.write((char*)&nodeClusters->unk5, sizeof(nodeClusters->unk5));
	}

	// Unknown struct that's seemingly link-related
	DevMsg(eDLL_T::SERVER, "Writing '%d' unknown link structs at '%x'\n", *g_nAiNodeClusterLinks, writeStream.tellp());
	writeStream.write((char*)g_nAiNodeClusterLinks, sizeof(*g_nAiNodeClusterLinks));
	for (int i = 0; i < *g_nAiNodeClusterLinks; i++)
	{
		// Disk and memory structs are literally identical here so just directly write.
		DevMsg(eDLL_T::SERVER, "Writing unknown link struct '%d' at '%x'\n", i, writeStream.tellp());
		writeStream.write((char*)(*g_pppAiNodeClusterLinks)[i], sizeof(*(*g_pppAiNodeClusterLinks)[i]));
	}

	// Some weird int idk what this is used for.
	writeStream.write((char*)&pNetwork->unk5, sizeof(pNetwork->unk5));

	// Tf2-exclusive stuff past this point, i.e. ain v57 only.
	DevMsg(eDLL_T::SERVER, "Writing '%d' script nodes at '%x'\n", pNetwork->scriptnodecount, writeStream.tellp());
	writeStream.write((char*)&pNetwork->scriptnodecount, sizeof(pNetwork->scriptnodecount));
	for (int i = 0; i < pNetwork->scriptnodecount; i++)
	{
		// disk and memory structs are literally identical here so just directly write
		//DevMsg(eDLL_T::SERVER, "Writing script node %d at %x\n", i, writeStream.tellp());
		//writeStream.write((char*)&pNetwork->scriptnodes[i], sizeof(pNetwork->scriptnodes[i]));
	}

	DevMsg(eDLL_T::SERVER, "Writing '%d' hints at '%x'\n", pNetwork->hintcount, writeStream.tellp());
	writeStream.write((char*)&pNetwork->hintcount, sizeof(pNetwork->hintcount));
	for (int i = 0; i < pNetwork->hintcount; i++)
	{
		DevMsg(eDLL_T::SERVER, "Writing hint data '%d' at '%x'\n", i, writeStream.tellp());
		writeStream.write((char*)&pNetwork->hints[i], sizeof(pNetwork->hints[i]));
	}

	writeStream.close();
}

void HCAI_NetworkManager__LoadNetworkGraph(void* aimanager, void* buf, const char* filename)
{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	CAI_NetworkManager__LoadNetworkGraph(aimanager, buf, filename, NULL);
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	CAI_NetworkManager__LoadNetworkGraph(aimanager, buf, filename);
#endif

	if (ai_dumpAINfileFromLoad->GetBool())
	{
		DevMsg(eDLL_T::SERVER, "Running BuildAINFile for loaded file '%s'\n", filename);
		CAI_NetworkBuilder::BuildFile(*(CAI_Network**)((char*)aimanager + 2840));
	}
}

void HCAI_NetworkBuilder__Build(void* builder, CAI_Network* aiNetwork, void* a3, int a4)
{
	CAI_NetworkBuilder__Build(builder, aiNetwork, a3, a4);
	CAI_NetworkBuilder::BuildFile(aiNetwork);
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
