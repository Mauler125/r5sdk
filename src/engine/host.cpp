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

CCommonHostState* g_pCommonHostState = nullptr;

void CCommonHostState::SetWorldModel(model_t* pModel)
{
	if (worldmodel == pModel)
		return;

	worldmodel = pModel;
	if (pModel)
	{
		worldbrush = pModel->brush.pShared;
	}
	else
	{
		worldbrush = NULL;
	}
}

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
	g_pOverlay->ShouldDraw(time);
#endif // !DEDICATED

	return v_Host_RunFrame(unused, time);
}

void _Host_Error(const char* error, ...)
{
	char buf[1024];
	{/////////////////////////////
		va_list args{};
		va_start(args, error);

		vsnprintf(buf, sizeof(buf), error, args);

		buf[sizeof(buf) - 1] = '\0';
		va_end(args);
	}/////////////////////////////

	Error(eDLL_T::ENGINE, NO_ERROR, "Host_Error: %s", buf);
	v_Host_Error(buf);
}

///////////////////////////////////////////////////////////////////////////////
void VHost::Detour(const bool bAttach) const
{
	DetourSetup(&v_Host_RunFrame, &_Host_RunFrame, bAttach);

#ifndef DEDICATED // Dedicated already logs this!
	DetourSetup(&v_Host_Error, &_Host_Error, bAttach);
#endif // !DEDICATED
}
