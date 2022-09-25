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
void SysGame_Attach()
{
	DetourAttach((LPVOID*)&v_CGame__PlayStartupVideos, &CGame::PlayStartupVideos);
}

void SysGame_Detach()
{
	DetourDetach((LPVOID*)&v_CGame__PlayStartupVideos, &CGame::PlayStartupVideos);
}