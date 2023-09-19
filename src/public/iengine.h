//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//===========================================================================//
#if !defined( IENGINE_H )
#define IENGINE_H
#ifdef _WIN32
#pragma once
#endif

//#include "tier1/interface.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
abstract_class IEngine
{
public:
	enum QuitState_t
	{
		QUIT_NOTQUITTING = 0,
		QUIT_TODESKTOP,
		QUIT_RESTART
	};

	// Engine State Flags
	enum EngineState_t
	{
		DLL_INACTIVE = 0,		// no dll
		DLL_ACTIVE,				// engine is focused
		DLL_CLOSE,				// closing down dll
		DLL_RESTART,			// engine is shutting down but will restart right away
		DLL_PAUSED,				// engine is paused, can become active from this state
	};


	virtual			~IEngine(void) { }

	virtual	bool	Load(bool dedicated, const char* rootdir) = 0;
	virtual void	Unload(void) = 0;
	virtual void	SetNextState(EngineState_t iNextState) = 0;
	virtual EngineState_t GetState(void) = 0;

	virtual bool	Frame(void) = 0; // Returns true if an engine frame is being ran.
	virtual float	GetFrameTime(void) = 0;
	virtual float GetPreviousTime(void) = 0;

	virtual __m128	GetCurTime(void) = 0;
	virtual void	SetQuitting(int quittype) = 0;

	virtual int GetPlaylistCount(void) = 0;

	virtual const char* sub_1401FE2B0(int a2) = 0; // Playlists KeyValues stuff.
	virtual bool sub_1401FE3B0(__int64 a2) = 0; // Playlists KeyValues stuff.
};

#endif // IENGINE_H
