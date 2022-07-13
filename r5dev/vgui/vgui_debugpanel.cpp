//===========================================================================//
//
// Purpose: Implements the debug panels.
//
// $NoKeywords: $
//===========================================================================//

#include <core/stdafx.h>
#include <tier1/cvar.h>
#include <windows/id3dx.h>
#include <vpc/keyvalues.h>
#include <mathlib/color.h>
#include <rtech/rui/rui.h>
#include <vgui/vgui_debugpanel.h>
#include <vguimatsurface/MatSystemSurface.h>
#include <materialsystem/cmaterialsystem.h>
#include <engine/debugoverlay.h>
#include <engine/client/clientstate.h>
#include <materialsystem/cmaterialglue.h>

#ifndef CLIENT_DLL
#include <engine/server/server.h>
#endif

//-----------------------------------------------------------------------------
// Purpose: proceed a log update
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
	if (cl_showhoststats->GetBool())
	{
		DrawHostStats();
	}
	if (cl_showmaterialinfo->GetBool())
	{
		DrawCrosshairMaterial();
	}
}

//-----------------------------------------------------------------------------
// Purpose: add a log to the vector.
//-----------------------------------------------------------------------------
void CLogSystem::AddLog(LogType_t type, string svMessage)
{
	if (svMessage.length() > 0)
	{
		m_vLogs.push_back(LogMsg_t{ svMessage, 1024, type });
	}
}

//-----------------------------------------------------------------------------
// Purpose: draw log on screen.
//-----------------------------------------------------------------------------
void CLogSystem::DrawLog(void)
{
	if (!m_vLogs.empty())
	{
		for (size_t i = 0; i < m_vLogs.size(); ++i)
		{
			if (m_vLogs[i].m_nTicks >= 0)
			{
				if (i < cl_consoleoverlay_lines->GetSizeT())
				{
					float fadepct = fminf(static_cast<float>(m_vLogs[i].m_nTicks) / 255.f, 4.f); // TODO [ AMOS ]: register a ConVar for this!
					float ptc = static_cast<int>(ceilf(fadepct * 100.f));
					int alpha = static_cast<int>(ptc);
					int x = cl_consoleoverlay_offset_x->GetInt();
					int y = cl_consoleoverlay_offset_y->GetInt() + (m_nFontHeight * i);
					Color c = GetLogColorForType(m_vLogs[i].m_type);

					if (cl_consoleoverlay_invert_rect_x->GetBool())
					{
						x = g_nWindowWidth - cl_consoleoverlay_offset_x->GetInt();
					}
					if (cl_consoleoverlay_invert_rect_y->GetBool())
					{
						y = g_nWindowHeight - cl_consoleoverlay_offset_y->GetInt();
						y += m_nFontHeight * i;
					}

					CMatSystemSurface_DrawColoredText(g_pMatSystemSurface, v_Rui_GetFontFace(), m_nFontHeight, x, y, c.r(), c.g(), c.b(), alpha, m_vLogs[i].m_svMessage.c_str());
				}
				else
				{
					m_vLogs.erase(m_vLogs.begin());
					continue;
				}

				m_vLogs[i].m_nTicks--;
			}
			else
			{
				m_vLogs.erase(m_vLogs.begin() + i);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: draw current host stats on screen.
//-----------------------------------------------------------------------------
void CLogSystem::DrawHostStats(void) const
{
	int nWidth  = cl_hoststats_offset_x->GetInt();
	int nHeight = cl_hoststats_offset_y->GetInt();
	static Color c = { 255, 255, 255, 255 };

	if (cl_hoststats_invert_rect_x->GetBool())
	{
		nWidth  = g_nWindowWidth  - nWidth;
	}
	if (cl_hoststats_invert_rect_y->GetBool())
	{
		nHeight = g_nWindowHeight - nHeight;
	}

	CMatSystemSurface_DrawColoredText(g_pMatSystemSurface, v_Rui_GetFontFace(), m_nFontHeight, nWidth, nHeight, c.r(), c.g(), c.b(), c.a(), (char*)m_pszCon_NPrintf_Buf);
}

//-----------------------------------------------------------------------------
// Purpose: draw current simulation stats on screen.
//-----------------------------------------------------------------------------
void CLogSystem::DrawSimStats(void) const
{
	int nWidth  = cl_simstats_offset_x->GetInt();
	int nHeight = cl_simstats_offset_y->GetInt();

	static Color c = { 255, 255, 255, 255 };
	static const char* szLogbuf[4096]{};

#ifdef CLIENT_DLL
	snprintf((char*)szLogbuf, 4096, "Client Frame: (%d) Render Frame: (%d)\n",
		g_pClientState->GetTick(), *render_tickcount);
#else
	snprintf((char*)szLogbuf, 4096, "Server Frame: (%d) Client Frame: (%d) Render Frame: (%d)\n",
		g_pServer->GetTick(), g_pClientState->GetTick(), *render_tickcount);
#endif



	if (cl_simstats_invert_rect_x->GetBool())
	{
		nWidth  = g_nWindowWidth  - nWidth;
	}
	if (cl_simstats_invert_rect_y->GetBool())
	{
		nHeight = g_nWindowHeight - nHeight;
	}

	CMatSystemSurface_DrawColoredText(g_pMatSystemSurface, v_Rui_GetFontFace(), m_nFontHeight, nWidth, nHeight, c.r(), c.g(), c.b(), c.a(), (char*)szLogbuf);
}

//-----------------------------------------------------------------------------
// Purpose: draw current gpu stats on screen.
//-----------------------------------------------------------------------------
void CLogSystem::DrawGPUStats(void) const
{
	int nWidth  = cl_gpustats_offset_x->GetInt();
	int nHeight = cl_gpustats_offset_y->GetInt();

	static Color c = { 255, 255, 255, 255 };
	static const char* szLogbuf[4096]{};
	snprintf((char*)szLogbuf, 4096, "%8d/%8d/%8dkiB unusable/unfree/total GPU Streaming Texture memory\n", 
	*unusable_streaming_tex_memory / 1024, *unfree_streaming_tex_memory / 1024, *unusable_streaming_tex_memory / 1024);

	if (cl_gpustats_invert_rect_x->GetBool())
	{
		nWidth  = g_nWindowWidth  - nWidth;
	}
	if (cl_gpustats_invert_rect_y->GetBool())
	{
		nHeight = g_nWindowHeight - nHeight;
	}

	CMatSystemSurface_DrawColoredText(g_pMatSystemSurface, v_Rui_GetFontFace(), m_nFontHeight, nWidth, nHeight, c.r(), c.g(), c.b(), c.a(), (char*)szLogbuf);
}

//-----------------------------------------------------------------------------
// Purpose: draw currently traced material info on screen.
//-----------------------------------------------------------------------------
void CLogSystem::DrawCrosshairMaterial(void) const
{
	CMaterialGlue* material = GetMaterialAtCrossHair();
	if (!material)
		return;

	static Color c = { 255, 255, 255, 255 };
	static const char* szLogbuf[4096]{};
	snprintf((char*)szLogbuf, 4096, "name: %s\nguid: %llx\ndimensions: %d x %d\nsurface: %s/%s\nsig: %i",
		material->m_pszName,
		material->m_GUID,
		material->m_iWidth, material->m_iHeight,
		material->m_pszSurfaceName1, material->m_pszSurfaceName2,
		material->m_UnknownSignature);

	CMatSystemSurface_DrawColoredText(g_pMatSystemSurface, v_Rui_GetFontFace(), m_nFontHeight, cl_materialinfo_offset_x->GetInt(), cl_materialinfo_offset_y->GetInt(), c.r(), c.g(), c.b(), c.a(), (char*)szLogbuf);
}

//-----------------------------------------------------------------------------
// Purpose: get log color for passed type.
//-----------------------------------------------------------------------------
Color CLogSystem::GetLogColorForType(LogType_t type) const
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
CLogSystem g_pLogSystem;
