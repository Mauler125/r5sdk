//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//
#ifndef UTIL_SHARED_H
#define UTIL_SHARED_H
#ifndef CLIENT_DLL
#include "game/server/player.h"
#endif

#ifndef CLIENT_DLL
CPlayer* UTIL_PlayerByIndex(int nIndex);
#endif // CLIENT_DLL

#endif // !UTIL_SHARED_H
