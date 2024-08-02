//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: Hint node utilities and functions.
//
// $NoKeywords: $
//=============================================================================//
#ifndef AI_HINT_H
#define AI_HINT_H

#include "baseentity.h"
#include "recast/Detour/Include/DetourNavMesh.h"
#include "ai_npcstate.h"

//###########################################################
//  >> HintNodeData
//
// This is a chunk of data that's passed to a hint node entity
// when it's created from a CNodeEnt.
//###########################################################
enum HintIgnoreFacing_e
{
	HIF_NO,
	HIF_YES,
	HIF_DEFAULT,
};

//-----------------------------------------------------------------------------
// Hint Data
//-----------------------------------------------------------------------------

struct HintNodeData
{
	string_t           strEntityName;
	Vector3D           vecPosition;
	short              nHintType;
	char gap_16[2];
	int                nNodeID;
	char gap_1c[4];
	string_t           strGroup;
	string_t           iszGenericType;
	string_t           iszActivityName;
	int                nTargetWCNodeID;
	HintIgnoreFacing_e fIgnoreFacing;
	NPC_STATE          minState;
	NPC_STATE          maxState;
	int                nRadius;

	int                nWCNodeID; // Node ID assigned by worldcraft (not same as engine!)

	//DECLARE_SIMPLE_DATADESC();
};

//-----------------------------------------------------------------------------
// CAI_Hint
//-----------------------------------------------------------------------------

class CAI_Hint : public CBaseEntity
{
private:
	void* __vftable;
	HintNodeData m_NodeData;
	int m_hintMovingGroundEnt;
	int m_hintGroundEnt;
	int m_hHintOwner;
	float m_flNextUseTime;
	float m_nodeFOV;
	float m_nodeFOVcos;
	Vector3D m_vecForward;
	float m_npcIdealYawAlign;
	float m_advanceFromCoverScalar;
	bool m_hintDisabled;
	char gap_b8d[1];
	short ainData;
	int shootingCoverType;
	dtPolyRef polyAttachedTo;
	CAI_Hint* hintOnSamePoly_next;
	CAI_Hint* hintOnSamePoly_prev;
	char pad[8];
};
static_assert(sizeof(CAI_Hint) == 0xBB0);

#endif // AI_HINT_H
