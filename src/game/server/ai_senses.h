//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: Base AI component
//
//=============================================================================//

#ifndef AI_SENSES_H
#define AI_SENSES_H

#include "tier0/threadtools.h"
#include "tier1/utlvector.h"
#include "tier1/utlmap.h"
//#include "simtimer.h"
#include "ai_component.h"
#include "soundent.h"

#include "game/shared/predictioncopy.h"

#if defined( _WIN32 )
#pragma once
#endif


//-----------------------------------------------------------------------------
// class CAI_Senses
//
// Purpose: 
//-----------------------------------------------------------------------------

class CAI_Senses : public CAI_Component
{
	int                  m_seenEnemiesCount;
	int                  m_ClosestHighPriority;
	int                  m_ClosestEnemyNPC;
	float                m_lookDistOverride;
	float                m_LastLookDist;
	float                m_TimeLastLook;
	float                m_hearingSensitivity;
	int                  m_iAudibleList; // first index of a linked list of sounds that the npc can hear.
	char                 m_NextAudibleSound[1024];
	CSound               m_reactToSound;
	CUtlVector<EHANDLE>  m_sightProgress;
	CUtlVector<EHANDLE>  m_SeenHighPriority;
	CUtlVector<EHANDLE>  m_SeenNPCs;
	CUtlVector<EHANDLE>  m_SeenMisc;
	CUtlVector<EHANDLE>  m_TeamSpotted;
	int                  m_SeenCorpseIndex;
	bool                 m_bGatheringSeenEntities;
	char gap_4fd[3];
	int                  m_currentSeenArray;
	char gap_504[4];
	CUtlVector<EHANDLE>* m_SeenArrays[3];
	float                m_TimeLastLookHighPriority;
	float                m_TimeLastLookNPCs;
	float                m_TimeLastLookMisc;
	float                m_TimeLastLookCorpse;
	int                  m_iSensingFlags;
	char gap_534[4];
	CThreadMutex         m_mutex;
	char gap560[1560];
};
static_assert(sizeof(CAI_Senses) == 0xB78);

#endif // AI_SENSES_H
