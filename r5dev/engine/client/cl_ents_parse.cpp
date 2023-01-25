//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: Parsing of entity network packets.
//
// $NoKeywords: $
//=============================================================================//
#include "core/stdafx.h"
#include "public/const.h"
#include "engine/client/cl_ents_parse.h"

bool CL_CopyExistingEntity(__int64 a1, unsigned int* a2, char* a3)
{
	int nNewEntity = *reinterpret_cast<int*>(a1 + 40);
	if (nNewEntity >= MAX_EDICTS || nNewEntity < NULL)
	{
		// Value isn't sanitized in release builds for
		// every game powered by the Source Engine 1
		// causing read/write outside of array bounds.
		// This defect has let to the achievement of a
		// full-chain RCE exploit. We hook and perform
		// sanity checks for the value of m_nNewEntity
		// here to prevent this behavior from happening.
		return false;
	}
	return v_CL_CopyExistingEntity(a1, a2, a3);
}
