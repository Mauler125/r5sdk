//=============================================================================//
//
// Purpose:
//
//=============================================================================//
#pragma once
#include "mathlib/vector.h"
constexpr int MAX_HULLS = 5;

constexpr int NOT_CACHED = -2;			// Returned if data not in cache
constexpr int NO_NODE    = -1;			// Returned when no node meets the qualification

//=============================================================================
//	>> CAI_NodeLink
//=============================================================================
struct CAI_NodeLink
{
	short m_iSrcID;
	short m_iDestID;
	bool m_bHulls[MAX_HULLS];
	byte m_LinkInfo;
	char unk1; // maps => unk0 on disk
	char unk2[5];
	int64_t m_nFlags;
};

//=============================================================================
//	>> CAI_Node
//=============================================================================
struct CAI_Node
{
	int m_nIndex; // Not present on disk
	Vector3D m_vOrigin;
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
	void* unkBuf0;
	void* unkBuf1;
	int m_nNumLinks;
	int unk11;     // Bad name lmao
	short unk6;    // Should match up to unk4 on disk
	char unk7[16]; // Padding until next bit
	short unk8;    // Should match up to unk5 on disk
	char unk9[8];  // Padding until next bit
	char unk10[8]; // Should match up to unk6 on disk
};

//=============================================================================
//	>> CAI_ScriptNode
//=============================================================================
struct CAI_ScriptNode
{
	Vector3D m_vOrigin;

	// Might be wrong; seems to be used for clamping.
	// See [r5apex_ds + 0xF28A6E]
	int m_nMin;
	int m_nMax;
};

//=============================================================================
//	>> CAI_Cluster
//=============================================================================
struct CAI_Cluster
{
	int m_nIndex;
	char unk0;
	char unk1;   // Maps to unk1 on disk

	Vector3D m_vOrigin;
	char unkC; // idk, might be a 4 bytes type or just padding.

	// These are utlvectors in engine, but its
	// unknown what they do yet.
	CUtlVector<int> unkVec0;
	CUtlVector<int> unkVec1;

	// This is an array of floats that is indexed
	// into by teamNum at [r5apex_ds + EC84DC];
	// Seems to be used along with the cvar:
	// 'ai_path_dangerous_cluster_min_time'.
	float clusterTime[MAX_TEAMS];

	float field_0250;
	float field_0254;
	float field_0258;
	char unk5;
};
static_assert(sizeof(CAI_Cluster) == 608);

//=============================================================================
//	>> CAI_ClusterLink
//=============================================================================
struct CAI_ClusterLink
{
	short prevIndex_MAYBE;
	short nextIndex_MAYBE;
	int unk2;
	char flags;
	char unk4;
	char unk5;
};
static_assert(sizeof(CAI_ClusterLink) == 12);