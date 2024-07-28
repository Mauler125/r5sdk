//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: AI Memory
//
// $NoKeywords: $
//=============================================================================//
#ifndef AI_MEMORY_H
#define AI_MEMORY_H

#if defined( _WIN32 )
#pragma once
#endif

#include "tier1/utlmap.h"
#include "baseentity.h"
#include "ai_component.h"

//-----------------------------------------------------------------------------
// AI_EnemyInfo_t
//
// Purpose: Stores relevant tactical information about an enemy
//
//-----------------------------------------------------------------------------
struct AI_EnemyInfo_t
{
	// todo: reverse engineer
};

//-----------------------------------------------------------------------------
// CAI_Enemies
//
// Purpose: Stores a set of AI_EnemyInfo_t's
//
//-----------------------------------------------------------------------------
class CAI_Enemies : public CAI_Component
{
public:
	typedef CUtlMap<CBaseEntity*, AI_EnemyInfo_t*, unsigned char> CMemMap;

private:
	int m_playerEnemyClass;
	char gap_14[4];
	CMemMap m_Map;
	float m_flFreeKnowledgeDuration;
	Vector3D m_vecDefaultLKP;
	Vector3D m_vecDefaultLSP;
	int m_serial;
};
static_assert(sizeof(CAI_Enemies) == 0x68);

//-----------------------------------------------------------------------------

#endif // AI_MEMORY_H