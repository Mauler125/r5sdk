//===== Copyright � 1996-2008, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "clientstate.h"
#ifndef CL_SPLITSCREEN_H
#define CL_SPLITSCREEN_H
#ifdef _WIN32
#pragma once
#endif

class CNetChan;
class ISplitScreen
{
public:

	virtual bool			Init() = 0;
	virtual void			Shutdown() = 0;

	virtual bool			AddSplitScreenUser( int nSlot, int nPlayerIndex ) = 0;
	virtual bool			AddBaseUser( int nSlot, int nPlayerIndex ) = 0;
	virtual bool			RemoveSplitScreenUser( int nSlot, int nPlayerIndex ) = 0;
	virtual int				GetActiveSplitScreenPlayerSlot() = 0;
	virtual int				SetActiveSplitScreenPlayerSlot( int nSlot ) = 0;

	virtual bool			IsValidSplitScreenSlot( int nSlot ) = 0;
	virtual int				FirstValidSplitScreenSlot() = 0; // -1 == invalid
	virtual int				NextValidSplitScreenSlot( int nPreviousSlot ) = 0; // -1 == invalid

	virtual int				GetNumSplitScreenPlayers() = 0;
	virtual int				GetSplitScreenPlayerEntity( int nSlot ) = 0;
	virtual CNetChan		*GetSplitScreenPlayerNetChan( int nSlot ) = 0;

	virtual bool			IsDisconnecting( int nSlot ) = 0;
	virtual void			SetDisconnecting( int nSlot, bool bState ) = 0;

	virtual bool			SetLocalPlayerIsResolvable( char const *pchContext, int nLine, bool bResolvable ) = 0;
	virtual bool			IsLocalPlayerResolvable() = 0;
};

class CSplitScreen : public ISplitScreen
{
	// Commented as 'CClientState' uses virtual functions,
	// which are pure in the SDK since its implemented in
	// shipped engine code, but this should be the struct:
//public:
//	struct SplitPlayer_t
//	{
//		bool m_bActive;
//		CClientState m_Client;
//	};
//
//	SplitPlayer_t m_SplitScreenPlayers[MAX_SPLITSCREEN_CLIENTS];
//	int m_nActiveSplitScreenUserCount;
};

#ifndef DEDICATED
extern CSplitScreen* g_pSplitScreenMgr;
#endif

class CSetActiveSplitScreenPlayerGuard
{
public:
	CSetActiveSplitScreenPlayerGuard( char const *pchContext, int nLine, int slot );
	~CSetActiveSplitScreenPlayerGuard();
private:
	char const *m_pchContext;
	int m_nLine;
	int	 m_nSaveSlot;
	bool m_bResolvable;
};

// If this is defined, all of the scopeguard objects are NULL'd out to reduce overhead
#ifdef DEDICATED
#define SPLIT_SCREEN_STUBS
#endif

#if defined( CSTRIKE15 ) // && !defined( _GAMECONSOLE ) // Split screen removed from console.
#define SPLIT_SCREEN_STUBS
#endif

#if defined( SPLIT_SCREEN_STUBS )
#define IS_LOCAL_PLAYER_RESOLVABLE true
#define SET_LOCAL_PLAYER_RESOLVABLE( a, b, c ) true
#define SET_ACTIVE_SPLIT_SCREEN_PLAYER_SLOT( x )

#define ACTIVE_SPLITSCREEN_PLAYER_GUARD( slot )
#define FORCE_DEFAULT_SPLITSCREEN_PLAYER_GUARD

#define FOR_EACH_VALID_SPLITSCREEN_PLAYER( iteratorName ) for ( int iteratorName = 0; iteratorName == 0; ++iteratorName )
#define FOR_EACH_SPLITSCREEN_PLAYER FOR_EACH_VALID_SPLITSCREEN_PLAYER

#define ASSERT_LOCAL_PLAYER_RESOLVABLE()
#define GET_ACTIVE_SPLITSCREEN_SLOT() ( 0 )
#define GET_NUM_SPLIT_SCREEN_PLAYERS() 1
#define IS_VALID_SPLIT_SCREEN_SLOT( i ) ( ( i ) == 0 )

#else
#define IS_LOCAL_PLAYER_RESOLVABLE ( g_pSplitScreenMgr->IsLocalPlayerResolvable() )

#define SET_LOCAL_PLAYER_RESOLVABLE( a, b, c ) ( g_pSplitScreenMgr->SetLocalPlayerIsResolvable( a, b, c ) )

#define ACTIVE_SPLITSCREEN_PLAYER_GUARD( slot )	CSetActiveSplitScreenPlayerGuard g_SSGuard( __FILE__, __LINE__, slot );
#define FORCE_DEFAULT_SPLITSCREEN_PLAYER_GUARD	CSetActiveSplitScreenPlayerGuard g_SSGuard( __FILE__, __LINE__, 0 );

#define FOR_EACH_VALID_SPLITSCREEN_PLAYER( iteratorName )						\
    for ( int iteratorName = g_pSplitScreenMgr->FirstValidSplitScreenSlot();			\
		 iteratorName != -1;													\
		iteratorName = g_pSplitScreenMgr->NextValidSplitScreenSlot( iteratorName ) )	
#define FOR_EACH_SPLITSCREEN_PLAYER( iteratorName ) for ( int iteratorName = 0; iteratorName < MAX_SPLITSCREEN_CLIENTS; ++iteratorName )

#define ASSERT_LOCAL_PLAYER_RESOLVABLE() Assert( g_pSplitScreenMgr->IsLocalPlayerResolvable() );
#define GET_ACTIVE_SPLITSCREEN_SLOT() g_pSplitScreenMgr->GetActiveSplitScreenPlayerSlot()
#define SET_ACTIVE_SPLIT_SCREEN_PLAYER_SLOT( x ) g_pSplitScreenMgr->SetActiveSplitScreenPlayerSlot( x )
#define GET_NUM_SPLIT_SCREEN_PLAYERS() ( g_pSplitScreenMgr->GetNumSplitScreenPlayers() )
#define IS_VALID_SPLIT_SCREEN_SLOT( i ) ( g_pSplitScreenMgr->IsValidSplitScreenSlot( i ) )
#endif

inline CClientState* GetBaseLocalClient()
{
	return g_pClientState;
}

class VSplitScreen : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogVarAdr("g_SplitScreenMgr", g_pSplitScreenMgr);
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
		const char* const pszPattern = "40 53 48 83 EC 20 48 8D 1D ?? ?? ?? ?? 83 FA FF 75 12 48 8B 05 ?? ?? ?? ?? 48 8B CB FF 50 28 48 63 C8 EB 03 48 63 CA 48 69 C1 ?? ?? ?? ?? 66 C7 84 18 ?? ?? ?? ?? ?? ??";
		const char* const pszInstruction = "48 8D";

		g_pSplitScreenMgr = g_GameDll.FindPatternSIMD(pszPattern).FindPatternSelf(pszInstruction).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CSplitScreen*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { };
};

#endif // CL_SPLITSCREEN_H
