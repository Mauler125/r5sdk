//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
//=============================================================================//

#include "core/stdafx.h"
#include "entitylist_base.h"
#include "public/ihandleentity.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

enum
{
	SERIAL_MASK = 0x7fff // the max value of a serial number, rolls back to 0 when it hits this limit
};

void CEntInfo::ClearLinks()
{
	m_pPrev = m_pNext = this;
}

// !TODO: entity list.
