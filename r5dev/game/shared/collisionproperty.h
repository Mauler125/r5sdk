//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef COLLISIONPROPERTY_H
#define COLLISIONPROPERTY_H

#include "mathlib/vector.h"
#include "engine/ICollideable.h"
#include "game/server/baseentity.h"

class CCollisionProperty : public ICollideable
{
	CBaseEntity* m_pOuter;
	Vector3D m_vecMins;
	Vector3D m_vecMaxs;
	int m_usSolidFlags;
	char m_nSolidType;
	char m_triggerBloat;
	char m_collisionDetailLevel;
	char m_isInDirtyList;
	char m_hasDirtyBounds;
	char m_hiddenFromSpatialQueries;
	__int16 m_dirtyListIndex;
	char m_nSurroundType;
	char gap_35[1];
	__int16 m_spatialAccelHandle;
	__int16 m_partitionMask;
	char gap_3a[2];
	float m_flRadius;
	Vector3D m_vecSpecifiedSurroundingMins;
	Vector3D m_vecSpecifiedSurroundingMaxs;
	Vector3D m_vecSurroundingMins;
	Vector3D m_vecSurroundingMaxs;
	float m_hitboxTestRadius;
};

#endif // COLLISIONPROPERTY_H