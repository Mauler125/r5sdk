//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef TRACE_H
#define TRACE_H

#ifdef _WIN32
#pragma once
#endif
#include "mathlib/mathlib.h"
#include "cmodel.h"

//=============================================================================
// Base Trace Structure
// - shared between engine/game dlls and tools (vrad)
//=============================================================================
class CBaseTrace
{
public:
	Vector3D startpos;
	float unk1;
	Vector3D endpos;
	float unk2;
	cplanetrace_t plane;
	float fraction;
	int contents;
	bool allsolid;
	bool startsolid;
};

#endif // TRACE_H
