//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef CMODEL_H
#define CMODEL_H
#ifdef _WIN32
#pragma once
#endif
#include "mathlib/mathlib.h"

// EVERYTHING IN HERE STILL NEEDS TESTING!!!!

struct Ray_t
{
	VectorAligned m_Start;
	VectorAligned m_Delta;
	VectorAligned m_StartOffset;
	VectorAligned m_Extents;
	char gap2C[0x10];
	void* m_pWorldAxisTransform;
	bool m_IsRay;
	bool m_IsSwept;

	void Init(Vector3D const& start, Vector3D const& end)
	{
		m_Delta = end - start;

		m_IsSwept = (m_Delta.LengthSqr() != 0);

		m_Extents.Init();

		m_pWorldAxisTransform = NULL;
		m_IsRay = true;

		m_StartOffset.Init();
		m_Start = start;
	}
};

struct csurface_t
{
	const char* name;
	short surfaceProp;
	uint16_t flags;
};

struct cplanetrace_t
{
	Vector3D normal;
	float dist;
};

#endif // CMODEL_H