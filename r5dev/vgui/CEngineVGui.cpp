#include "core/stdafx.h"
#include "tier0/cvar.h"
#include "mathlib/color.h"
#include "vgui/CEngineVGui.h"
#include "vguimatsurface/MatSystemSurface.h"
#include "materialsystem/materialsystem.h"
#include "engine/debugoverlay.h"
#include "engine/baseclientstate.h"
#include "server/server.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int HCEngineVGui_Paint(void* thisptr, int mode)
{
	int result = CEngineVGui_Paint(thisptr, mode);

	static void* pCMatSystemSurface = ADDRESS(0x14D40B3B0).RCast<void* (*)()>();
	static auto fnRenderStart       = ADDRESS(0x14053EFC0).RCast<void(*)(void*)>();
	static auto fnRenderEnd         = ADDRESS(0x14053F1B0).RCast<void* (*)()>();

	if (mode == 1 || mode == 2) // Render in-main menu and in-game.
	{
		fnRenderStart(pCMatSystemSurface);

		g_pLogSystem.Update();

		fnRenderEnd();
	}

	return result;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogSystem::Update()
{
	if (!g_pMatSystemSurface)
	{
		return;
	}
	if (cl_drawconsoleoverlay->GetBool())
	{
		DrawLog();
	}
	if (cl_showsimstats->GetBool())
	{
		DrawSimStats();
	}
	if (cl_showgpustats->GetBool())
	{
		DrawGPUStats();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogSystem::AddLog(LogType_t type, std::string message)
{
	if (message.length() > 0)
	{
		m_vLogs.push_back(Log{ message, 1024, type });
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void  CLogSystem::DrawLog()
{
	if (m_vLogs.empty())
	{
		return;
	}
	for (int i = 0; i < m_vLogs.size(); ++i)
	{
		if (m_vLogs[i].Ticks >= 0)
		{
			if (i < cl_consoleoverlay_lines->GetInt())
			{
				float fadepct = fminf(static_cast<float>(m_vLogs[i].Ticks) / 255.f, 4.0); // TODO [ AMOS ]: register a ConVar for this!
				float ptc = static_cast<int>(ceilf(fadepct * 100.f));                     // TODO [ AMOS ]: register a ConVar for this!
				int alpha = static_cast<int>(ptc);
				int y = (cl_consoleoverlay_offset_y->GetInt() + (fontHeight * i));
				int x = cl_consoleoverlay_offset_x->GetInt();

				Color c = GetLogColorForType(m_vLogs[i].Type);
				CMatSystemSurface_DrawColoredText(g_pMatSystemSurface, 0x13, fontHeight, x, y, c._color[0], c._color[1], c._color[2], alpha, m_vLogs[i].Message.c_str());
			}
			else
			{
				m_vLogs.erase(m_vLogs.begin());
				continue;
			}

			m_vLogs[i].Ticks--;
		}
		else
		{
			m_vLogs.erase(m_vLogs.begin() + i);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogSystem::DrawSimStats()
{
	Color c = { 255, 255, 255, 255 };
	static const char* szLogbuf[4096]{};
	snprintf((char*)szLogbuf, 4096, "Server Frame: (%d) Client Frame: (%d) Render Frame: (%d)\n",
	*sv_m_nTickCount, *cl_host_tickcount, *render_tickcount);

	CMatSystemSurface_DrawColoredText(g_pMatSystemSurface, 0x13, fontHeight, cl_simstats_offset_x->GetInt(), cl_simstats_offset_y->GetInt(), c._color[0], c._color[1], c._color[2], 255, (char*)szLogbuf);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogSystem::DrawGPUStats()
{
	Color c = { 255, 255, 255, 255 };
	static const char* szLogbuf[4096]{};
	snprintf((char*)szLogbuf, 4096, "%8d/%8d/%8dkiB unusable/unfree/total GPU Streaming Texture memory\n", 
	*unusable_streaming_tex_memory / 1024, *unfree_streaming_tex_memory / 1024, *unusable_streaming_tex_memory / 1024);

	CMatSystemSurface_DrawColoredText(g_pMatSystemSurface, 0x13, fontHeight, cl_gpustats_offset_x->GetInt(), cl_gpustats_offset_y->GetInt(), c._color[0], c._color[1], c._color[2], 255, (char*)szLogbuf);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Color CLogSystem::GetLogColorForType(LogType_t type)
{
	switch (type)
	{
		case LogType_t::NATIVE:
			return { 255, 255, 255, 255 };
		case LogType_t::SCRIPT_SERVER:
			return { 190, 183, 240, 255 };
		case LogType_t::SCRIPT_CLIENT:
			return { 117, 116, 139, 255 };
		case LogType_t::SCRIPT_UI:
			return { 197, 160, 177, 255 };
		default:
			return { 255, 255, 255, 255 };
	}

	return { 255, 255, 255, 255 };
}

///////////////////////////////////////////////////////////////////////////////
void CEngineVGui_Attach()
{
	//DetourAttach((LPVOID*)&CEngineVGui_Paint, &HCEngineVGui_Paint);
}

void CEngineVGui_Detach()
{
	//DetourDetach((LPVOID*)&CEngineVGui_Paint, &HCEngineVGui_Paint);
}

///////////////////////////////////////////////////////////////////////////////
CLogSystem g_pLogSystem;
