//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//
#ifndef EIFACE_H
#define EIFACE_H
#include "edict.h"

//-----------------------------------------------------------------------------
// Purpose: Interface to get at server clients
//-----------------------------------------------------------------------------
abstract_class IServerGameClients
{
public:
	// Get server max players and lower bound for same
	virtual void GetPlayerLimits(int& nMinPlayers, int& nMaxPlayers, int& nDefaultMaxPlayers) const = 0;

	// Client is connecting to server ( return false to reject the connection )
	//	You can specify a rejection message by writing it into pszReject
	virtual bool ClientConnect(edict_t nEntity, char const* pszName, char const* pszAddress, char* pszReject, int nMaxRejectLen) = 0;

	// Client is going active
	// If bLoadGame is true, don't spawn the player because its state is already setup.
	virtual void ClientActive(edict_t nEntity, bool bLoadGame) = 0;
	virtual void ClientFullyConnect(edict_t nEntity, bool bRestore) = 0;
};

//-----------------------------------------------------------------------------
// Purpose: Interface to get at server entities
//-----------------------------------------------------------------------------
abstract_class IServerGameEnts
{
public:
	// !TODO
};

#endif // EIFACE_H
