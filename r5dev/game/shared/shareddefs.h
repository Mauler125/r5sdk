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

#define TICK_INTERVAL			((*g_pGlobals)->m_nTickInterval)

#define TIME_TO_TICKS( dt )		( (int)( 0.5f + (float)(dt) / TICK_INTERVAL ) )
#define TICKS_TO_TIME( t )		( TICK_INTERVAL *( t ) )
#define ROUND_TO_TICKS( t )		( TICK_INTERVAL * TIME_TO_TICKS( t ) )
#define TICK_NEVER_THINK		(-1)

#endif // !CLIENT_DLL


#endif // SHAREDDEFS_H