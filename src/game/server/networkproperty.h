//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef SERVERNETWORKPROPERTY_H
#define SERVERNETWORKPROPERTY_H
#ifdef _WIN32
#pragma once
#endif

#include "public/iservernetworkable.h"
#include "public/server_class.h"
#include "public/edict.h"
#include "game/shared/predictioncopy.h"

class CServerNetworkProperty : IServerNetworkable
{
public:
	inline edict_t GetEdict(void) const { return m_edict; }

private:
	CBaseEntity* m_pOuter;
	ServerClass* m_pServerClass;
	edict_t m_edict;
	EHANDLE m_hParent;
};
static_assert(sizeof(CServerNetworkProperty) == 32);

#endif // SERVERNETWORKPROPERTY_H
