//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "core/stdafx.h"
#include "tier0/frametask.h"
#include "engine/host.h"
#ifndef DEDICATED
#include "vgui/vgui_debugpanel.h"
#endif // !DEDICATED

/*
==================
_Host_RunFrame

Runs all active servers
==================
*/
void _Host_RunFrame(void* unused, float time)
{
	for (IFrameTask* const& task : g_FrameTasks)
	{
		task->RunFrame();
	}

	g_FrameTasks.erase(std::remove_if(g_FrameTasks.begin(), g_FrameTasks.end(), [](const IFrameTask* task)
		{
			return task->IsFinished();
		}), g_FrameTasks.end());

#ifndef DEDICATED
	g_pLogSystem.ShouldDraw(time);
#endif // !DEDICATED

	return v_Host_RunFrame(unused, time);
}

///////////////////////////////////////////////////////////////////////////////
void Host_Attach()
{
	DetourAttach((LPVOID*)&v_Host_RunFrame, &_Host_RunFrame);
}

void Host_Detach()
{
	DetourDetach((LPVOID*)&v_Host_RunFrame, &_Host_RunFrame);
}