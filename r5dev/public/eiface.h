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
#include "tier1/bitbuf.h"
#include "vpc/keyvalues.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CRecipientFilter; // TODO: Reverse.

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
	virtual ~IServerGameEnts() = 0;
	// !TODO
};

//-----------------------------------------------------------------------------
// Purpose: Interface the engine exposes to the game DLL
//-----------------------------------------------------------------------------
abstract_class IVEngineServer
{
public:
	virtual int			GetNumGameSlots(void) const = 0;
	// Tell engine to change level ( "changelevel s1\n" )
	virtual void		ChangeLevel( const char *szLevelName ) = 0;
	// Ask engine whether the specified map is a valid map file (exists and has valid version number).
	virtual bool		IsMapValid(const char* szFileName) = 0;
	// Is this a dedicated server?
	virtual bool		IsDedicatedServer(void) = 0;
	// Is this server active?
	virtual bool		IsActive(void) = 0;
	virtual void		NullSub0(void) = 0;
	// get arbitrary launch options
	virtual KeyValues* GetLaunchOptions(void) = 0;

	// Add to the server/client lookup/precache table, the specified string is given a unique index
	// NOTE: The indices for PrecacheModel are 1 based
	//  a 0 returned from those methods indicates the model or sound was not correctly precached
	// However, generic and decal are 0 based
	// If preload is specified, the file is loaded into the server/client's cache memory before level startup, otherwise
	//  it'll only load when actually used (which can cause a disk i/o hitch if it occurs during play of a level).
	virtual int			PrecacheModel(const char* szName) = 0;
	virtual int			PrecacheDecal(const char* szName) = 0;

	virtual int			GetNumEdicts(void) const = 0;

	// !TODO:
	virtual void sub_140313E70(void) = 0;
	virtual void sub_140313EC0(void) = 0;
	virtual void sub_140313F10(void) = 0;
	virtual void sub_140313F70(void) = 0;
	virtual void sub_140313FB0(void) = 0;
	virtual void sub_140314020(void) = 0;
	virtual void sub_140314060(void) = 0;
	virtual void sub_140314080(void) = 0;
	virtual void sub_1403140C0(void) = 0;
	virtual void sub_140314140(void) = 0;
	virtual void sub_140314150(void) = 0;


	virtual bool		EmptyEdictSlotsAvailable(void) const = 0;

	// Fade out the client's volume level toward silence (or fadePercent)
	virtual void        FadeClientVolume(const edict_t* pEdict, float flFadePercent, float flFadeOutSeconds, float flHoldTime, float flFadeInSeconds) = 0;

	// Issue a command to the command parser as if it was typed at the server console.	
	virtual void		ServerCommand(const char* szCommand) = 0;
	virtual void		ServerCommandTokenized(const char* szCommand) = 0;
	// Execute any commands currently in the command parser immediately (instead of once per frame)
	virtual void		ServerExecute(void) = 0;
	// Issue the specified command to the specified client (mimics that client typing the command at the console).
	virtual void		ClientCommand(edict_t* pEdict, PRINTF_FORMAT_STRING const char* szFmt, ...) FMTFUNCTION(3, 4) = 0;

	// Set the lightstyle to the specified value and network the change to any connected clients.  Note that val must not 
	//  change place in memory (use MAKE_STRING) for anything that's not compiled into your mod.
	virtual void		LightStyle( int nStyle, PRINTF_FORMAT_STRING const char *szVal ) = 0;

	virtual bf_write*	UserMessageBegin(CRecipientFilter* filter, int a3, char* szMessageName, int nMsgIdx) = 0;
	virtual void		MessageEnd(void) = 0;
	virtual void		MessageCancel(void) = 0;

	// Print szMsg to the client console.
	virtual void		ClientPrintf(edict_t nEdict, const char* szMsg) = 0;

	// SINGLE PLAYER/LISTEN SERVER ONLY (just matching the client .dll api for this)
	// Prints the formatted string to the notification area of the screen ( down the right hand edge
	//  numbered lines starting at position 0
	virtual void		Con_NPrintf( int nPos, const char *szFmt, ... ) = 0; // Might not work, this vtable pointer points to a implementation that has a signature similar to 'Con_NXPrintf'.
	// SINGLE PLAYER/LISTEN SERVER ONLY(just matching the client .dll api for this)
	// Similar to Con_NPrintf, but allows specifying custom text color and duration information
	virtual void		Con_NXPrintf( const struct con_nprint_s *pInfo, const char *szFmt, ... ) = 0;

	// Change a specified player's "view entity" (i.e., use the view entity position/orientation for rendering the client view)
	virtual void		SetView(const edict_t nClient, const edict_t nViewEnt) = 0;

	// returns 'pViewent'.
	virtual void		Unk0(const edict_t nClient, const edict_t nViewEnt) = 0;
	// TODO: similar to 'SetView'
	virtual void		Unk1(const edict_t nClient, const edict_t nViewEnt) = 0;
	virtual void		Unk2(const edict_t nClient, const edict_t nViewEnt) = 0;

	// Set the player's crosshair angle
	virtual void		CrosshairAngle(const edict_t nClient, float flPitch, float flYaw) = 0;
	virtual bool		GrantClientSidePickup(const edict_t nClient, int a3, int a4, int* a5, int a6, int a7, int a8) = 0;

	// Get the current game directory (hl2, tf2, hl1, cstrike, etc.)
	virtual void        GetGameDir(char* szGetGameDir, int nMaxlength) = 0;
	// Used by AI node graph code to determine if .bsp and .ain files are out of date
	virtual int 		CompareFileTime(const char* szFileName1, const char* szFileName2, int* iCompare) = 0;

	// Locks/unlocks the network string tables (.e.g, when adding bots to server, this needs to happen).
	// Be sure to reset the lock after executing your code!!!
	virtual bool		LockNetworkStringTables( bool bLock ) = 0;

	virtual int GetNumConnectedPlayers(void) const = 0;
	virtual int GetNumTotalPlayers(void) const = 0;
	virtual int GetNumFakePlayers(void) const = 0;
	virtual int GetNumHumanPlayers(void) const = 0;

	// Create a bot with the given name.  Returns -1 if fake client can't be created
	virtual edict_t CreateFakeClient(const char* szName, int nTeam) = 0;

	// Get a convar keyvalue for specified client
	virtual const char* GetClientConVarValue(int nClientIndex, const char* szConVarName) = 0;


#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	virtual void NullSub1(void) = 0; // Additional nullsub only present in s0 and s1 gamedll's
#endif // GAMEDLL_S0 || GAMEDLL_S1

	// Returns the name as represented on the server of specified client
	virtual const char* GetClientServerName(int nClientIndex) = 0;
	// Returns the network address of specified client
	virtual const char* GetClientNetworkAddress(int nClientIndex) = 0;
	// !TODO: Returns float field in CCLient from specified client, needs to be reversed still
	virtual float Unk3(int nClientIndex) = 0;

	virtual bool ReplayEnabled(void) const = 0;

	// !TODO: the rest..
};

#endif // EIFACE_H
