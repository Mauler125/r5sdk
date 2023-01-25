//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//
#include "core/stdafx.h"
#include "tier0/commandline.h"
#include "engine/sys_mainwind.h"

//-----------------------------------------------------------------------------
// Purpose: plays the startup video's
//-----------------------------------------------------------------------------
void CGame::PlayStartupVideos(void)
{
	if (!CommandLine()->CheckParm("-novid"))
	{
		v_CGame__PlayStartupVideos();
	}
}

///////////////////////////////////////////////////////////////////////////////
void VGame::Attach() const
{
	DetourAttach(&v_CGame__PlayStartupVideos, &CGame::PlayStartupVideos);
}

void VGame::Detach() const
{
	DetourDetach(&v_CGame__PlayStartupVideos, &CGame::PlayStartupVideos);
}