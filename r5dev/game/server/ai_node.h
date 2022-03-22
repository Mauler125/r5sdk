//=============================================================================//
//
// Purpose:
//
//=============================================================================//
#pragma once
#include "mathlib/vector.h"
constexpr int MAX_HULLS = 5;

//=============================================================================
//	>> CAI_NodeLink
//=============================================================================
struct CAI_NodeLink
{
	short m_iSrcID;
	short m_iDestID;
	bool m_bHulls[MAX_HULLS];
	char unk0;
	char unk1; // maps => unk0 on disk
	char unk2[5];
	int64_t m_nFlags;
};

//=============================================================================
//	>> CAI_NodeLinkDisk
//=============================================================================
#pragma pack(push, 1)
struct CAI_NodeLinkDisk
{
	short m_iSrcID;
	short m_iDestID;
	char unk0;
	bool m_bHulls[MAX_HULLS];
};
#pragma pack(pop)

//=============================================================================
//	>> CAI_Node
//=============================================================================
struct CAI_Node
{
	int m_nIndex; // Not present on disk
	Vector3 m_vOrigin;
	float m_fHulls[MAX_HULLS];
	float m_flYaw;

	int unk0;            // Always 2 in buildainfile, maps directly to unk0 in disk struct
	int unk1;            // Maps directly to unk1 in disk struct
	int unk2[MAX_HULLS]; // Maps directly to unk2 in disk struct, despite being ints rather than shorts

	// View server.dll+393672 for context
	char unk3[MAX_HULLS];  // Should map to unk3 on disk
	char pad[3];           // Aligns next bytes
	float unk4[MAX_HULLS]; // I have no clue, calculated using some kind float function magic

	CAI_NodeLink** links;
	char unk5[16];
	int m_nNumLinks;
	int unk11;     // Bad name lmao
	short unk6;    // Should match up to unk4 on disk
	char unk7[16]; // Padding until next bit
	short unk8;    // Should match up to unk5 on disk
	char unk9[8];  // Padding until next bit
	char unk10[8]; // Should match up to unk6 on disk
};

//=============================================================================
//	>> CAI_NodeDisk
//=============================================================================
#pragma pack(push, 1)
struct CAI_NodeDisk // The way CAI_Nodes are represented in on-disk ain files
{
	Vector3 m_vOrigin;

	float m_flYaw;
	float hulls[MAX_HULLS];

	char unk0;
	int unk1;
	short unk2[MAX_HULLS];
	char unk3[MAX_HULLS];
	short unk4;
	short unk5;
	char unk6[8];
}; // Total size of 68 bytes
#pragma pack(pop)

//=============================================================================
//	>> CAI_ScriptNode
//=============================================================================
struct CAI_ScriptNode
{
	Vector3 m_vOrigin;
	uint64_t scriptdata;
};

struct AINodeClusters
{
	int m_nIndex;
	char unk0;
	char unk1;	  // Maps to unk1 on disk
	char pad0[2]; // Padding to +8

	Vector3 m_vOrigin;

	char pad5[4];
	int* unk2;     // Maps to unk5 on disk;
	char pad1[16]; // Pad to +48
	int unkcount0; // Maps to unkcount0 on disk

	char pad2[4];  // Pad to +56
	int* unk3;
	char pad3[16]; // Pad to +80
	int unkcount1;

	char pad4[132];
	char unk5;
};

struct AINodeClusterLinks
{
	short unk0;
	short unk1;
	int unk2;
	char unk3;
	char unk4;
	char unk5;
};
