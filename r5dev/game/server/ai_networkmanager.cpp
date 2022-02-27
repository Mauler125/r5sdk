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
	//std::filesystem::path fsWritePath("platform/maps/graphs");
	//fsWritePath /= g_pHostState->m_levelName;
	//fsWritePath += ".ain";

	//// Dump from memory.
	//DevMsg(eDLL_T::SERVER, "++++--------------------------------------------------------------------------------------------------------------------------++++\n");
	//DevMsg(eDLL_T::SERVER, ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> AI NODE GRAPH FILE CONSTRUCTION STARTED <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
	//DevMsg(eDLL_T::SERVER, "++++--------------------------------------------------------------------------------------------------------------------------++++\n");
	//DevMsg(eDLL_T::SERVER, "Output file: '%s'\n", fsWritePath.string());

	//std::ofstream writeStream(fsWritePath, std::ofstream::binary);
	//DevMsg(eDLL_T::SERVER, "Writing AINet version: %d\n", AINET_VERSION_NUMBER);
	//writeStream.write((char*)&AINET_VERSION_NUMBER, sizeof(int));

	//// Could probably be cleaner but whatever
	////int nMapVersion = *(int*)(*pUnkServerMapversionGlobal + 104); // TODO: Find in apex
	////DevMsg(eDLL_T::SERVER, "Writing map version: %d\n", nMapVersion); // temp
	////writeStream.write((char*)&nMapVersion, sizeof(int));

	//DevMsg(eDLL_T::SERVER, "Writing placeholder crc: %d\n", PLACEHOLDER_CRC);
	//writeStream.write((char*)&PLACEHOLDER_CRC, sizeof(int));

	//int nCalculatedLinkcount = 0;

	//// Path nodes
	//DevMsg(eDLL_T::SERVER, "Writing nodecount: %d\n", pNetwork->nodecount);
	//writeStream.write((char*)&pNetwork->nodecount, sizeof(int));

	//for (int i = 0; i < pNetwork->nodecount; i++)
	//{
	//	// construct on-disk node struct
	//	CAI_NodeDisk diskNode{};
	//	diskNode.x = pNetwork->nodes[i]->x;
	//	diskNode.y = pNetwork->nodes[i]->y;
	//	diskNode.z = pNetwork->nodes[i]->z;
	//	diskNode.yaw = pNetwork->nodes[i]->yaw;
	//	memcpy(diskNode.hulls, pNetwork->nodes[i]->hulls, sizeof(diskNode.hulls));
	//	diskNode.unk0 = (char)pNetwork->nodes[i]->unk0;
	//	diskNode.unk1 = pNetwork->nodes[i]->unk1;

	//	for (int j = 0; j < MAX_HULLS; j++)
	//	{
	//		diskNode.unk2[j] = (short)pNetwork->nodes[i]->unk2[j];
	//		spdlog::info((short)pNetwork->nodes[i]->unk2[j]);
	//	}

	//	memcpy(diskNode.unk3, pNetwork->nodes[i]->unk3, sizeof(diskNode.unk3));
	//	diskNode.unk4 = pNetwork->nodes[i]->unk6;
	//	diskNode.unk5 =
	//		-1; // aiNetwork->nodes[i]->unk8; // This field is wrong, however it's always -1 in original navmeshes anyway.
	//	memcpy(diskNode.unk6, pNetwork->nodes[i]->unk10, sizeof(diskNode.unk6));

	//	DevMsg(eDLL_T::SERVER, "Writing node %d from %d to %x\n", pNetwork->nodes[i]->index, (void*)pNetwork->nodes[i], writeStream.tellp());
	//	writeStream.write((char*)&diskNode, sizeof(CAI_NodeDisk));

	//	nCalculatedLinkcount += pNetwork->nodes[i]->linkcount;
	//}

	//// links
	//DevMsg(eDLL_T::SERVER, "Linkcount: %d\n", pNetwork->linkcount);
	//DevMsg(eDLL_T::SERVER, "Calculated total linkcount: %d\n", nCalculatedLinkcount);

	//nCalculatedLinkcount /= 2;
	//if (ai_dumpAINfileFromLoad->GetBool())
	//{
	//	if (pNetwork->linkcount == nCalculatedLinkcount)
	//		DevMsg(eDLL_T::SERVER, "Caculated linkcount is normal!");
	//	else
	//		DevMsg(eDLL_T::SERVER, "Calculated linkcount has weird value! this is expected on build!");
	//}

	//spdlog::info("Writing linkcount: %d\n", nCalculatedLinkcount);
	//writeStream.write((char*)&nCalculatedLinkcount, sizeof(int));

	//for (int i = 0; i < pNetwork->nodecount; i++)
	//{
	//	for (int j = 0; j < pNetwork->nodes[i]->linkcount; j++)
	//	{
	//		// skip links that don't originate from current node
	//		if (pNetwork->nodes[i]->links[j]->srcId != pNetwork->nodes[i]->index)
	//			continue;

	//		CAI_NodeLinkDisk diskLink{};
	//		diskLink.srcId = pNetwork->nodes[i]->links[j]->srcId;
	//		diskLink.destId = pNetwork->nodes[i]->links[j]->destId;
	//		diskLink.unk0 = pNetwork->nodes[i]->links[j]->unk1;
	//		memcpy(diskLink.hulls, pNetwork->nodes[i]->links[j]->hulls, sizeof(diskLink.hulls));

	//		DevMsg(eDLL_T::SERVER, "Writing link %d => %d to %x\n", diskLink.srcId, diskLink.destId, writeStream.tellp());
	//		writeStream.write((char*)&diskLink, sizeof(CAI_NodeLinkDisk));
	//	}
	//}

	//// Don't know what this is, it's likely a block from tf1 that got deprecated? should just be 1 int per node
	//DevMsg(eDLL_T::SERVER, "Writing %x bytes for unknown block at %x\n", pNetwork->nodecount * sizeof(uint32_t), writeStream.tellp());
	//uint32_t* unkNodeBlock = new uint32_t[pNetwork->nodecount];
	//memset(unkNodeBlock, 0, pNetwork->nodecount * sizeof(uint32_t));
	//writeStream.write((char*)unkNodeBlock, pNetwork->nodecount * sizeof(uint32_t));
	//delete[] unkNodeBlock;

	//// TODO: this is traverse nodes i think? these aren't used in tf2 ains so we can get away with just writing count=0 and skipping
	//// but ideally should actually dump these
	//DevMsg(eDLL_T::SERVER, "Writing %d traversal nodes at %x\n", 0, writeStream.tellp());
	//short traverseNodeCount = 0;
	//writeStream.write((char*)&traverseNodeCount, sizeof(short));
	//// Only write count since count=0 means we don't have to actually do anything here

	//// TODO: ideally these should be actually dumped, but they're always 0 in tf2 from what i can tell
	//DevMsg(eDLL_T::SERVER, "Writing %d bytes for unknown hull block at %x\n", MAX_HULLS * 8, writeStream.tellp());
	//char* unkHullBlock = new char[MAX_HULLS * 8];
	//memset(unkHullBlock, 0, MAX_HULLS * 8);
	//writeStream.write(unkHullBlock, MAX_HULLS * 8);
	//delete[] unkHullBlock;

	//// Snknown struct that's seemingly node-related
	//DevMsg(eDLL_T::SERVER, "Writing %d unknown node structs at %x\n", *pUnkStruct0Count, writeStream.tellp());
	//writeStream.write((char*)pUnkStruct0Count, sizeof(*pUnkStruct0Count));
	//for (int i = 0; i < *pUnkStruct0Count; i++)
	//{
	//	DevMsg(eDLL_T::SERVER, "Writing unknown node struct %d at %x\n", i, writeStream.tellp());
	//	UnkNodeStruct0* nodeStruct = (*pppUnkNodeStruct0s)[i];

	//	writeStream.write((char*)&nodeStruct->index, sizeof(nodeStruct->index));
	//	writeStream.write((char*)&nodeStruct->unk1, sizeof(nodeStruct->unk1));

	//	writeStream.write((char*)&nodeStruct->x, sizeof(nodeStruct->x));
	//	writeStream.write((char*)&nodeStruct->y, sizeof(nodeStruct->y));
	//	writeStream.write((char*)&nodeStruct->z, sizeof(nodeStruct->z));

	//	writeStream.write((char*)&nodeStruct->unkcount0, sizeof(nodeStruct->unkcount0));
	//	for (int j = 0; j < nodeStruct->unkcount0; j++)
	//	{
	//		short unk2Short = (short)nodeStruct->unk2[j];
	//		writeStream.write((char*)&unk2Short, sizeof(unk2Short));
	//	}

	//	writeStream.write((char*)&nodeStruct->unkcount1, sizeof(nodeStruct->unkcount1));
	//	for (int j = 0; j < nodeStruct->unkcount1; j++)
	//	{
	//		short unk3Short = (short)nodeStruct->unk3[j];
	//		writeStream.write((char*)&unk3Short, sizeof(unk3Short));
	//	}

	//	writeStream.write((char*)&nodeStruct->unk5, sizeof(nodeStruct->unk5));
	//}

	//// Unknown struct that's seemingly link-related
	//DevMsg(eDLL_T::SERVER, "Writing %d unknown link structs at %x\n", *pUnkLinkStruct1Count, writeStream.tellp());
	//writeStream.write((char*)pUnkLinkStruct1Count, sizeof(*pUnkLinkStruct1Count));
	//for (int i = 0; i < *pUnkLinkStruct1Count; i++)
	//{
	//	// disk and memory structs are literally identical here so just directly write
	//	DevMsg(eDLL_T::SERVER, "Writing unknown link struct %d at %x\n", i, writeStream.tellp());
	//	writeStream.write((char*)(*pppUnkStruct1s)[i], sizeof(*(*pppUnkStruct1s)[i]));
	//}

	//// Some weird int idk what this is used for
	//writeStream.write((char*)&pNetwork->unk5, sizeof(pNetwork->unk5));

	//// Tf2-exclusive stuff past this point, i.e. ain v57 only
	//DevMsg(eDLL_T::SERVER, "Writing %d script nodes at %x\n", pNetwork->scriptnodecount, writeStream.tellp());
	//writeStream.write((char*)&pNetwork->scriptnodecount, sizeof(pNetwork->scriptnodecount));
	//for (int i = 0; i < pNetwork->scriptnodecount; i++)
	//{
	//	// disk and memory structs are literally identical here so just directly write
	//	DevMsg(eDLL_T::SERVER, "Writing script node %d at %x\n", i, writeStream.tellp());
	//	writeStream.write((char*)&pNetwork->scriptnodes[i], sizeof(pNetwork->scriptnodes[i]));
	//}

	//DevMsg(eDLL_T::SERVER, "Writing %d hints at %x\n", pNetwork->hintcount, writeStream.tellp());
	//writeStream.write((char*)&pNetwork->hintcount, sizeof(pNetwork->hintcount));
	//for (int i = 0; i < pNetwork->hintcount; i++)
	//{
	//	DevMsg(eDLL_T::SERVER, "Writing hint data %d at %x\n", i, writeStream.tellp());
	//	writeStream.write((char*)&pNetwork->hints[i], sizeof(pNetwork->hints[i]));
	//}

	//writeStream.close();
}

void HCAI_NetworkManager__LoadNetworkGraph(void* aimanager, void* buf, const char* filename)
{
	CAI_NetworkManager__LoadNetworkGraph(aimanager, buf, filename);

	if (ai_dumpAINfileFromLoad->GetBool())
	{
		DevMsg(eDLL_T::SERVER, "Running BuildAINFile for loaded file %s\n", filename);
		CAI_NetworkBuilder::BuildFile(*(CAI_Network**)((char*)aimanager + 2536)); // TODO: Verify in r5apex.exe.
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
