//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//
#include "core/stdafx.h"
#include "entitylist.h"

IHandleEntity* LookupEntityByIndex(int iEntity)
{
	Assert(iEntity >= 0 && iEntity < NUM_ENT_ENTRIES); // Programmer error!

	IHandleEntity* pHandle = reinterpret_cast<IHandleEntity*>(*&g_pEntityList[6 * iEntity]);
	return pHandle; // !TODO: implement CBaseEntityList properly.
}

CEntInfo** g_pEntityList = nullptr;