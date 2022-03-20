#pragma once
#include "game/server/ai_node.h"

//-----------------------------------------------------------------------------
// CAI_Network
//
// Purpose: Stores a node graph through which an AI may pathfind
//-----------------------------------------------------------------------------
class CAI_Network
{
public:
	void* m_pVTable;

	// this is uninitialised and never set on ain build, fun!
	int m_iNumLinks;                   // +8
	char unk1[124];                    // +12
	int m_iNumZones;                   // +136
	char unk2[16];                     // +140

	// unk8 on disk
	int unk5;                          // +156
	char unk6[4];                      // +160
	int m_iNumHints;                   // +164

	// these probably aren't actually hints, but there's 1 of them per hint so idk
	short m_Hints[2000];               // +168
	int m_iNumScriptNodes;             // +4168
	char pad[28]; // unk
	int64_t m_iNumNodes;               // +4200
	CAI_Node** m_pAInode;              // +4208

	CAI_ScriptNode m_ScriptNode[4000]; // +4172
};