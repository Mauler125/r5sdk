//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
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
#include "mathlib/mathlib.h"

class CPlayerState
{
public:
	// This virtual method is necessary to generate a vtable in all cases
	// (DECLARE_PREDICTABLE will generate a vtable also)!
	virtual ~CPlayerState() {}

	int playerEntityIndex;
	char gap_c[4];
	__int64 currentClass;
	__int64 requestedClass;
	__int64 onDeathClass;
	__int64 oldClass;
	__int64 netname;
	int fixangle;
	Vector3D anglechange;
	int index;
	Vector3D forceWorldViewAngle;
	int forceWorldViewAngleUntilTick;
	bool replay;
	char gap_5d[3];
	int lastPlayerView_tickcount;
	Vector3D lastPlayerView_origin;
	Vector3D lastPlayerView_angle;
	bool deadflag;
	char gap_7d[3];
	Vector3D m_localViewAngles;
};

#endif // PLAYERSTATE_H