#pragma once
#include "ai_node.h"

//-----------------------------------------------------------------------------
// CAI_Network
//
// Purpose: Stores a node graph through which an AI may pathfind
//-----------------------------------------------------------------------------
class CAI_Network
{
public:
	char unk0[8];                     // +0

	// this is uninitialised and never set on ain build, fun!
	int linkcount;                    // +8
	char unk1[124];                   // +12
	int zonecount;                    // +136
	char unk2[16];                    // +140

	// unk8 on disk
	int unk5;                         // +156
	char unk6[4];                     // +160
	int hintcount;                    // +164

	// these probably aren't actually hints, but there's 1 of them per hint so idk
	short hints[2000];                // +168
	int scriptnodecount;              // +4168
	CAI_ScriptNode scriptnodes[4000]; // +4172
	int nodecount;                    // +84172
	CAI_Node** nodes;                 // +84176

public:
	static void BuildAINFile(CAI_Network* aiNetwork);
};