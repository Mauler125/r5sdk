//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PLAYERSTATE_H
#define PLAYERSTATE_H
#ifdef _WIN32
#pragma once
#endif
#include "tier1/string_t.h"
#include "mathlib/mathlib.h"

class CPlayerState
{
public:
	// This virtual method is necessary to generate a vtable in all cases
	// (DECLARE_PREDICTABLE will generate a vtable also)!
	virtual ~CPlayerState() {}

	int playerEntityIndex;
	char gap_c[4];
	i64 currentClass;
	i64 requestedClass;
	i64 onDeathClass;
	i64 oldClass;
	string_t netname;
	int fixangle;
	QAngle anglechange;
	int index;
	QAngle forceWorldViewAngle;
	int forceWorldViewAngleUntilTick;
	bool replay;
	char gap_5d[3];
	int lastPlayerView_tickcount;
	Vector3D lastPlayerView_origin;
	QAngle lastPlayerView_angle;
	bool deadflag;
	char gap_7d[3];
	QAngle m_localViewAngles;
};

#endif // PLAYERSTATE_H