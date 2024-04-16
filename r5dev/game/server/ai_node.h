//=============================================================================//
//
// Purpose:
//
//=============================================================================//
#pragma once
#include "mathlib/vector.h"
#include "mathlib/bitvec.h"
constexpr int MAX_HULLS = 5;

constexpr int NOT_CACHED = -2;			// Returned if data not in cache
constexpr int NO_NODE    = -1;			// Returned when no node meets the qualification

//=========================================================
//	>> The type of node
//=========================================================
enum NodeType_e // !TODO: unconfirmed for r1/r2/r5.
{
	NODE_ANY,			// Used to specify any type of node (for search)
	NODE_DELETED,		// Used in wc_edit mode to remove nodes during runtime     
	NODE_GROUND,
	NODE_AIR,
	NODE_CLIMB,
	NODE_WATER
};

//=============================================================================
//	>> CAI_NodeLink
//=============================================================================
struct CAI_NodeLink
{
	short m_iSrcID;
	short m_iDestID;
	byte m_iAcceptedMoveTypes[MAX_HULLS];
	byte m_LinkInfo;
	char unk1; // maps => unk0 on disk
	char unk2[5];
	int64_t m_nFlags;
};

//=============================================================================
//	>> CAI_Node
//=============================================================================
class CAI_Node
{
public:
	const Vector3D& GetOrigin() const { return m_vOrigin; }
	Vector3D& AccessOrigin() { return m_vOrigin; }
	float			GetYaw() const { return m_flYaw; }

	int				NumLinks() const { return m_Links.Count(); }
	void			ClearLinks() { m_Links.Purge(); }
	CAI_NodeLink* GetLinkByIndex(int i) const { return m_Links[i]; }

	NodeType_e		SetType(NodeType_e type) { return (m_eNodeType = type); }
	NodeType_e		GetType() const { return m_eNodeType; }

	int				SetInfo(int info) { return m_eNodeInfo = info; }
	int				GetInfo() const { return m_eNodeInfo; }

	int        m_iID;                  // ID for this node
	Vector3D   m_vOrigin;              // location of this node in space
	float      m_flVOffset[MAX_HULLS]; // vertical offset for each hull type, assuming ground node, 0 otherwise
	float      m_flYaw;                // NPC on this node should face this yaw to face the hint, or climb a ladder

	NodeType_e m_eNodeType; // The type of node; always 2 in buildainfile.
	int        m_eNodeInfo; // bits that tell us more about this nodes

	int unk2[MAX_HULLS]; // Maps directly to unk2 in disk struct, despite being ints rather than shorts

	// View server.dll+393672 for context
	char unk3[MAX_HULLS];  // Should map to unk3 on disk
	char pad[3];           // Aligns next bytes
	float unk4[MAX_HULLS]; // I have no clue, calculated using some kind float function magic

	CUtlVector<CAI_NodeLink*> m_Links;
	short unk6;    // Should match up to unk4 on disk
	char unk7[16]; // Padding until next bit
	short unk8;
	short unk9;    // Should match up to unk5 on disk
	char unk10[6]; // Padding until next bit
	char unk11[8]; // Should match up to unk6 on disk
};

//=============================================================================
//	>> CAI_Cluster
//=============================================================================
class CAI_Cluster
{
public:
	const Vector3D& GetOrigin() const { return m_vOrigin; }
	Vector3D& AccessOrigin() { return m_vOrigin; }

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
	short m_iSrcID;
	short m_iDestID;
	int unk2;
	char flags;
	char unkFlags4;
	char unkFlags5;
};
static_assert(sizeof(CAI_ClusterLink) == 12);

//=============================================================================
//	>> CAI_ScriptNode
//=============================================================================
struct CAI_TraverseNode
{
	Quaternion m_Quat;
	int m_Index_MAYBE;
};
static_assert(sizeof(CAI_TraverseNode) == 20);

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
//	>> CAI_HullData
//=============================================================================
struct CAI_HullData
{
	CVarBitVec m_bitVec;

	// Unknown, possible part of CVarBitVec ??? see [r5apex_ds + 1A52B0] if,
	// this is part of CVarBitVec, it seems to be unused in any of the
	// compiled CVarBitVec and CLargeVarBitVec methods so i think it should be
	// just part of this struct.
	char unk3[8];
};
