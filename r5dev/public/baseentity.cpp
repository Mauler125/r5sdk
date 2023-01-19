//======= Copyright (c) 1996-2009, Valve Corporation, All rights reserved. ======
//
// Purpose: The base class from which all game entities are derived.
//
//===============================================================================
#include "core/stdafx.h"
#include "baseentity.h"

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
CCollisionProperty* CBaseEntity::CollisionProp()
{
	return &m_Collision;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
const CCollisionProperty* CBaseEntity::CollisionProp() const
{
	return &m_Collision;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
CServerNetworkProperty* CBaseEntity::NetworkProp()
{
	return &m_Network;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
const CServerNetworkProperty* CBaseEntity::NetworkProp() const
{
	return &m_Network;
}
