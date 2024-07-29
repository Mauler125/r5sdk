//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: AI task scheduler
//
//=============================================================================//
#ifndef AI_SCHEDULE_H
#define AI_SCHEDULE_H

#ifdef _WIN32
#pragma once
#endif
#include "mathlib/bitvec.h"

#define	MAX_CONDITIONS 12*8
typedef CBitVec<MAX_CONDITIONS> CAI_ScheduleBits;

class CAI_Schedule
{
	// todo: reverse engineer.
};

#endif // AI_SCHEDULE_H
