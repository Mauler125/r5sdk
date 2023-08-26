//======= Copyright (c) 1996-2009, Valve Corporation, All rights reserved. ======
//
// Purpose: Definitions that are shared by the game DLL and the client DLL.
//
//===============================================================================


#ifndef SHAREDDEFS_H
#define SHAREDDEFS_H
#ifdef _WIN32
#pragma once
#endif

#ifndef CLIENT_DLL
#include "game/server/gameinterface.h"

#define TICK_INTERVAL			((*g_pGlobals)->m_flTickInterval)

#define TIME_TO_TICKS( dt )		( (int)( 0.5f + (float)(dt) / TICK_INTERVAL ) )
#define TICKS_TO_TIME( t )		( TICK_INTERVAL *( t ) )
#define ROUND_TO_TICKS( t )		( TICK_INTERVAL * TIME_TO_TICKS( t ) )
#define TICK_NEVER_THINK		(-1)

#endif // !CLIENT_DLL


// ---------------------------
//  Hit Group standards
// ---------------------------
#define HITGROUP_INVALID	-1
#define	HITGROUP_GENERIC	0
#define	HITGROUP_HEAD		1
#define	HITGROUP_CHEST		2
#define	HITGROUP_STOMACH	3
#define HITGROUP_LEFTARM	4	
#define HITGROUP_RIGHTARM	5
#define HITGROUP_LEFTLEG	6
#define HITGROUP_RIGHTLEG	7
#define HITGROUP_GEAR		8			// alerts NPC, but doesn't do damage or bleed (1/100th damage)
#define HITGROUP_COUNT		9

typedef enum // !TODO[ AMOS ]: Confirm this!
{
	USE_OFF = 0,
	USE_ON = 1,
	USE_SET = 2,
	USE_TOGGLE = 3
} USE_TYPE;

#endif // SHAREDDEFS_H