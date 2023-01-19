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

struct CServerNetworkProperty : IServerNetworkable
{
public:
	edict_t GetEdict() const;

private:
	CBaseEntity* m_pOuter;
	ServerClass* m_pServerClass;
	int m_edict;
	int m_hParent;
};

#endif // SERVERNETWORKPROPERTY_H
