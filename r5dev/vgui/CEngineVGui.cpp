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
void CLogSystem::Update(void)
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
void CLogSystem::AddLog(LogType_t type, std::string svMessage)
{
	if (svMessage.length() > 0)
	{
		m_vLogs.push_back(Log{ svMessage, 1024, type });
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogSystem::DrawLog(void)
{
	if (m_vLogs.empty()) { return; }
	for (int i = 0; i < m_vLogs.size(); ++i)
	{
		if (m_vLogs[i].Ticks >= 0)
		{
			if (i < cl_consoleoverlay_lines->GetInt())
			{
				float fadepct = fminf(static_cast<float>(m_vLogs[i].Ticks) / 255.f, 4.f); // TODO [ AMOS ]: register a ConVar for this!
				float ptc = static_cast<int>(ceilf(fadepct * 100.f));
				int alpha = static_cast<int>(ptc);
				int y = (cl_consoleoverlay_offset_y->GetInt() + (fontHeight * i));
				int x = cl_consoleoverlay_offset_x->GetInt();

				Color c = GetLogColorForType(m_vLogs[i].Type);
				CMatSystemSurface_DrawColoredText(g_pMatSystemSurface, 0x13, fontHeight, x, y, c.r(), c.g(), c.b(), alpha, m_vLogs[i].Message.c_str());
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
void CLogSystem::DrawSimStats(void) const
{
	static Color c = { 255, 255, 255, 255 };
	static const char* szLogbuf[4096]{};
	snprintf((char*)szLogbuf, 4096, "Server Frame: (%d) Client Frame: (%d) Render Frame: (%d)\n",
	*sv_m_nTickCount, *cl_host_tickcount, *render_tickcount);

	CMatSystemSurface_DrawColoredText(g_pMatSystemSurface, 0x13, fontHeight, cl_simstats_offset_x->GetInt(), cl_simstats_offset_y->GetInt(), c.r(), c.g(), c.b(), c.a(), (char*)szLogbuf);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogSystem::DrawGPUStats(void) const
{
	static Color c = { 255, 255, 255, 255 };
	static const char* szLogbuf[4096]{};
	snprintf((char*)szLogbuf, 4096, "%8d/%8d/%8dkiB unusable/unfree/total GPU Streaming Texture memory\n", 
	*unusable_streaming_tex_memory / 1024, *unfree_streaming_tex_memory / 1024, *unusable_streaming_tex_memory / 1024);

	CMatSystemSurface_DrawColoredText(g_pMatSystemSurface, 0x13, fontHeight, cl_gpustats_offset_x->GetInt(), cl_gpustats_offset_y->GetInt(), c.r(), c.g(), c.b(), c.a(), (char*)szLogbuf);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Color CLogSystem::GetLogColorForType(LogType_t type)
{
	switch (type)
	{
	case LogType_t::SCRIPT_SERVER:
		return { cl_conoverlay_script_server_clr->GetColor() };
	case LogType_t::SCRIPT_CLIENT:
		return { cl_conoverlay_script_client_clr->GetColor() };
	case LogType_t::SCRIPT_UI:
		return { cl_conoverlay_script_ui_clr->GetColor() };
	case LogType_t::NATIVE_SERVER:
		return { cl_conoverlay_native_server_clr->GetColor() };
	case LogType_t::NATIVE_CLIENT:
		return { cl_conoverlay_native_client_clr->GetColor() };
	case LogType_t::NATIVE_UI:
		return { cl_conoverlay_native_ui_clr->GetColor() };
	case LogType_t::NATIVE_ENGINE:
		return { cl_conoverlay_native_engine_clr->GetColor() };
	case LogType_t::NATIVE_FS:
		return { cl_conoverlay_native_fs_clr->GetColor() };
	case LogType_t::NATIVE_RTECH:
		return { cl_conoverlay_native_rtech_clr->GetColor() };
	case LogType_t::NATIVE_MS:
		return { cl_conoverlay_native_ms_clr->GetColor() };
	case LogType_t::NETCON_S:
		return { cl_conoverlay_netcon_clr->GetColor() };
	case LogType_t::WARNING_C:
		return { cl_conoverlay_warning_clr->GetColor() };
	case LogType_t::ERROR_C:
		return { cl_conoverlay_error_clr->GetColor() };
	default:
		return { cl_conoverlay_native_engine_clr->GetColor() };
	}
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
