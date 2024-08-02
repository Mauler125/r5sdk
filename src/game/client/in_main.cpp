//========= Copyright (c) 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: builds an intended movement command to send to the server
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=======================================================================================//
#include "kbutton.h"
#include "engine/client/cl_splitscreen.h"

kbutton_t::Split_t& kbutton_t::GetPerUser( int nSlot /*=-1*/ )
{
	if ( nSlot == -1 )
	{
		ASSERT_LOCAL_PLAYER_RESOLVABLE();
		nSlot = GET_ACTIVE_SPLITSCREEN_SLOT();
	}
	return m_PerUser[ nSlot ];
}
