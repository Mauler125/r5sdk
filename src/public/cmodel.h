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

struct Ray_t
{
	VectorAligned m_Start;
	VectorAligned m_Delta;
	VectorAligned m_StartOffset;
	int m_nUnk30;
	int m_nUnk34;
	int m_nUnk38;
	int m_nUnk3C;
	int m_nUnk40;
	int m_nUnk44;
	int m_nFlags;
	const matrix3x4_t* m_pWorldAxisTransform;
	int m_nUnk58;
	bool m_IsRay;
	bool m_IsSwept;
	int m_nFlags2; // Unsure what these do yet.
	int m_nUnk68;

	Ray_t() : m_pWorldAxisTransform(nullptr) {};
	Ray_t(Vector3D const& start, Vector3D const& end, int nFlags1 = 0x3f800000, int nFlags2 = NULL) { Init(start, end, nFlags1, nFlags2); };

	void Init(Vector3D const& start, Vector3D const& end, int nFlags1, int nFlags2)
	{
		VectorSubtract(end, start, m_Delta);
		m_IsSwept = (m_Delta.LengthSqr() != 0);

		m_nUnk30 = NULL;
		m_nUnk34 = NULL;
		m_nUnk38 = NULL;
		m_nUnk3C = NULL;
		m_nUnk40 = NULL;
		m_nUnk44 = NULL;

		m_nFlags = nFlags1; // !TODO: Reverse these flags!

		m_pWorldAxisTransform = nullptr;
		m_IsRay = true;

		m_nUnk58 = NULL;
		m_nFlags2 = nFlags2; // !TODO: Reverse these flags!
		m_nUnk68 = NULL;

		VectorClear(m_StartOffset);
		VectorCopy(start, m_Start);
	}
};
static_assert(sizeof(Ray_t) == 0x70);

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

struct dsurfaceproperty_t
{
	__int16 unk;
	byte unk2;
	byte contentMaskOffset;
	int surfaceNameOffset;
};

struct dbvhnode_t
{
	__int16 mins_x[4];
	__int16 mins_y[4];
	__int16 mins_z[4];
	__int16 maxs_x[4];
	__int16 maxs_y[4];
	__int16 maxs_z[4];
	int index_child01_types;
	int index_child23_types;
	int index_cm;
	int index_pad;
};

struct bspmodel_t
{
	dbvhnode_t* bvhnodes;
	dsurfaceproperty_t* surfaceproperties;
	int* bvhleafdata;
	void* vertices;
	int* contentmasks;
	char* texdatastringdata;
	int unk;
	int unk2;
	float unk_f1;
	float unk_f2;
	float unk_f3;
	float unk_f4;
};

#endif // CMODEL_H
