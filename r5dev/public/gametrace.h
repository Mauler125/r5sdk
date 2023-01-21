//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
//=============================================================================//

#ifndef GAMETRACE_H
#define GAMETRACE_H
#ifdef _WIN32
#pragma once
#endif
#include "trace.h"
#include "cmodel.h"

// !TODO: Remove these and include properly.
#if defined( CLIENT_DLL )
class C_BaseEntity;
#else
class CBaseEntity;
#endif
//-----------------------------------------------------------------------------
// Purpose: A trace is returned when a box is swept through the world
//-----------------------------------------------------------------------------
class CGameTrace : public CBaseTrace
{
public:
	char gap3A[0x4];
	csurface_t surface;
	float fractionleftsolid;
	int hitgroup;
	short physicsBone;
	char gap5A[0x6];
#if defined( CLIENT_DLL )
	C_BaseEntity* hit_entity;
#else
	CBaseEntity* hit_entity;
#endif
	int hitbox;
	char gap6C[0x114];
};
static_assert(sizeof(CGameTrace) == 0x180);
typedef CGameTrace trace_t;

#endif // GAMETRACE_H
