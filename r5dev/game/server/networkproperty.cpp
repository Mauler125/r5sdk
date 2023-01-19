//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "core/stdafx.h"
#include "public/baseentity.h"
#include "public/basehandle.h"
#include "networkproperty.h"

edict_t CServerNetworkProperty::GetEdict(void) const
{
	return m_edict;
}
