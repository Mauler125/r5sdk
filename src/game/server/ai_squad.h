//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: Squad classes
//
//=============================================================================//
#ifndef AI_SQUAD_H
#define AI_SQUAD_H

#include "mathlib/bitvec.h"
#include "tier1/string_t.h"
#include "game/shared/predictioncopy.h"

struct SquadSlotTimer
{
	int slot;
	float endTime;
};

struct AISquadEnemyInfo_t
{
	// todo: reverse engineer and verify this!
	//EHANDLE 						hEnemy;
	//CBitVec<MAX_SQUADSLOTS>	slots;									// What squad slots are filled?

	//DECLARE_SIMPLE_DATADESC();
};

class CAI_Squad
{
private:
	CAI_Squad* m_pNextSquad;
	string_t m_Name;
	char m_SquadMembers[72];
	char m_membersSorted[72];
	Vector3D m_memberSortPos;
	float m_memberSortTime;
	float m_useShortestPathThresholdSqr;
	float m_useShortestPathPercent;
	float m_flSquadSoundWaitTime;
	int m_nSquadSoundPriority;
	int m_hSquadInflictor;
	int m_squadEnemy;
	float m_squadEnemyUpdateTime;
	float m_squadEnemySeeTime;
	Vector3D m_investigatePos;
	Vector3D m_centroid;
	float m_centroidTime;
	Vector3D m_attackPathAssignPos;
	int m_attackPathAssignCluster;
	float m_nextThinkTime;
	SquadSlotTimer m_slotTimers[2];
	char gap_108[8];
	float m_squadHealthInteruptThreshold;
	int m_squadModifierFlags;
	char m_mutex[40];
	AISquadEnemyInfo_t* m_pLastFoundEnemyInfo;
	char m_EnemyInfos[32];
	float m_flEnemyInfoCleanupTime;
	float m_squadMemberDeathTime;
	char m_squadPathData[16];
	int m_squadMode;
};
static_assert(sizeof(CAI_Squad) == 0x190);

#endif // AI_SQUAD_H
