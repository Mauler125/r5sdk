//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: Parsing of entity network packets.
//
// $NoKeywords: $
//=============================================================================//


#include "core/stdafx.h"
#include "tier0/frametask.h"
#include "public/const.h"
#include "engine/host.h"
#include "engine/client/cl_ents_parse.h"

bool CL_CopyExistingEntity(__int64 a1, unsigned int* a2, char* a3)
{
	int nNewEntity = *reinterpret_cast<int*>(a1 + 40);
	if (nNewEntity >= MAX_EDICTS || nNewEntity < 0)
	{
		v_Host_Error("CL_CopyExistingEntity: m_nNewEntity >= MAX_EDICTS");
		return false;
	}
	return v_CL_CopyExistingEntity(a1, a2, a3);
}

void CL_Ents_Parse_Attach()
{
	DetourAttach((LPVOID*)&v_CL_CopyExistingEntity, &CL_CopyExistingEntity);
}
void CL_Ents_Parse_Detach()
{
	DetourDetach((LPVOID*)&v_CL_CopyExistingEntity, &CL_CopyExistingEntity);
}