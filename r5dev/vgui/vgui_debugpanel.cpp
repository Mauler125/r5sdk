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
#include <rtech/rtech_utils.h>
#include <engine/sys_engine.h>

//-----------------------------------------------------------------------------
// Purpose: proceed a log update
//-----------------------------------------------------------------------------
void CLogSystem::Update(void)
{
	if (!g_pMatSystemSurface)
	{
		return;
	}
	if (con_drawnotify->GetBool())
	{
		DrawNotify();
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
	if (stream_overlay->GetBool())
	{
		DrawStreamOverlay();
	}
}

//-----------------------------------------------------------------------------
// Purpose: add a log to the vector.
//-----------------------------------------------------------------------------
void CLogSystem::AddLog(const EGlobalContext_t context, const string& svText)
{
	if (con_drawnotify->GetBool())
	{
		if (svText.length() > 0)
		{
			std::lock_guard<std::mutex> l(m_Mutex);
			m_vNotifyText.push_back(CNotifyText{ context, con_notifytime->GetFloat() , svText });

			while (m_vNotifyText.size() > 0 &&
				(m_vNotifyText.size() >= cl_consoleoverlay_lines->GetInt()))
			{
				m_vNotifyText.erase(m_vNotifyText.begin());
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: draw notify logs on screen.
//-----------------------------------------------------------------------------
void CLogSystem::DrawNotify(void)
{
	int x;
	int y;
	if (cl_consoleoverlay_invert_rect_x->GetBool())
	{
		x = g_nWindowWidth - cl_consoleoverlay_offset_x->GetInt();
	}
	else
	{
		x = cl_consoleoverlay_offset_x->GetInt();
	}
	if (cl_consoleoverlay_invert_rect_y->GetBool())
	{
		y = g_nWindowHeight - cl_consoleoverlay_offset_y->GetInt();
	}
	else
	{
		y = cl_consoleoverlay_offset_y->GetInt();
	}

	std::lock_guard<std::mutex> l(m_Mutex);
	for (int i = 0, j = m_vNotifyText.size(); i < j; i++)
	{
		CNotifyText* pNotify = &m_vNotifyText[i];
		Color c = GetLogColorForType(m_vNotifyText[i].m_type);

		float flTimeleft = pNotify->m_flLifeRemaining;

		if (flTimeleft < .5f)
		{
			float f = clamp(flTimeleft, 0.0f, .5f) / .5f;
			c[3] = (int)(f * 255.0f);

			if (i == 0 && f < 0.2f)
			{
				y -= m_nFontHeight * (1.0f - f / 0.2f);
			}
		}
		else
		{
			c[3] = 255;
		}
		CMatSystemSurface_DrawColoredText(g_pMatSystemSurface, v_Rui_GetFontFace(), m_nFontHeight, x, y, c.r(), c.g(), c.b(), c.a(), m_vNotifyText[i].m_svMessage.c_str());

		if (IsX360())
		{
			// For some reason the fontTall value on 360 is about twice as high as it should be
			y += 12;
		}
		else
		{
			y += m_nFontHeight;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: checks if the notify text is expired
// Input  : flFrameTime - 
//-----------------------------------------------------------------------------
void CLogSystem::ShouldDraw(const float flFrameTime)
{
	if (con_drawnotify->GetBool())
	{
		std::lock_guard<std::mutex> l(m_Mutex);

		int i;
		int c = m_vNotifyText.size();
		for (i = c - 1; i >= 0; i--)
		{
			CNotifyText* notify = &m_vNotifyText[i];
			notify->m_flLifeRemaining -= flFrameTime;

			if (notify->m_flLifeRemaining <= 0.0f)
			{
				m_vNotifyText.erase(m_vNotifyText.begin() + i);
				continue;
			}
		}
	}
	else if (!m_vNotifyText.empty())
	{
		m_vNotifyText.clear();
	}
}

//-----------------------------------------------------------------------------
// Purpose: draw current host stats on screen.
//-----------------------------------------------------------------------------
void CLogSystem::DrawHostStats(void) const
{
	int nWidth  = cl_hoststats_offset_x->GetInt();
	int nHeight = cl_hoststats_offset_y->GetInt();
	const static Color c = { 255, 255, 255, 255 };

	if (cl_hoststats_invert_rect_x->GetBool())
	{
		nWidth  = g_nWindowWidth  - nWidth;
	}
	if (cl_hoststats_invert_rect_y->GetBool())
	{
		nHeight = g_nWindowHeight - nHeight;
	}

	CMatSystemSurface_DrawColoredText(g_pMatSystemSurface, v_Rui_GetFontFace(), m_nFontHeight, nWidth, nHeight, c.r(), c.g(), c.b(), c.a(), m_pszCon_NPrintf_Buf);
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

	snprintf((char*)szLogbuf, 4096, "Server Frame: (%d) Client Frame: (%d) Render Frame: (%d)\n",
		g_pClientState->GetServerTickCount(), g_pClientState->GetClientTickCount(), *g_nRenderTickCount);

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
	*g_nUnusableStreamingTextureMemory / 1024, *g_nUnfreeStreamingTextureMemory / 1024, *g_nUnusableStreamingTextureMemory / 1024);

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
	snprintf((char*)szLogbuf, 4096, "name: %s\nguid: %llx\ndimensions: %d x %d\nsurface: %s/%s\nstc: %i\ntc: %i",
		material->m_pszName,
		material->m_GUID,
		material->m_iWidth, material->m_iHeight,
		material->m_pszSurfaceName1, material->m_pszSurfaceName2,
		material->m_nStreamableTextureCount,
		material->m_pShaderGlue->m_nTextureInputCount);

	CMatSystemSurface_DrawColoredText(g_pMatSystemSurface, v_Rui_GetFontFace(), m_nFontHeight, cl_materialinfo_offset_x->GetInt(), cl_materialinfo_offset_y->GetInt(), c.r(), c.g(), c.b(), c.a(), (char*)szLogbuf);
}

void CLogSystem::DrawStreamOverlay(void) const
{
	char buf[4096];
	
	GetStreamOverlay(stream_overlay_mode->GetString(), buf, sizeof(buf));

	static Color c = { 255, 255, 255, 255 };

	CMatSystemSurface_DrawColoredText(g_pMatSystemSurface, v_Rui_GetFontFace(), m_nFontHeight, 20, 300, c.r(), c.g(), c.b(), c.a(), buf);
}

//-----------------------------------------------------------------------------
// Purpose: get log color for passed type.
//-----------------------------------------------------------------------------
Color CLogSystem::GetLogColorForType(const EGlobalContext_t context) const
{
	switch (context)
	{
	case EGlobalContext_t::SCRIPT_SERVER:
		return { cl_conoverlay_script_server_clr->GetColor() };
	case EGlobalContext_t::SCRIPT_CLIENT:
		return { cl_conoverlay_script_client_clr->GetColor() };
	case EGlobalContext_t::SCRIPT_UI:
		return { cl_conoverlay_script_ui_clr->GetColor() };
	case EGlobalContext_t::NATIVE_SERVER:
		return { cl_conoverlay_native_server_clr->GetColor() };
	case EGlobalContext_t::NATIVE_CLIENT:
		return { cl_conoverlay_native_client_clr->GetColor() };
	case EGlobalContext_t::NATIVE_UI:
		return { cl_conoverlay_native_ui_clr->GetColor() };
	case EGlobalContext_t::NATIVE_ENGINE:
		return { cl_conoverlay_native_engine_clr->GetColor() };
	case EGlobalContext_t::NATIVE_FS:
		return { cl_conoverlay_native_fs_clr->GetColor() };
	case EGlobalContext_t::NATIVE_RTECH:
		return { cl_conoverlay_native_rtech_clr->GetColor() };
	case EGlobalContext_t::NATIVE_MS:
		return { cl_conoverlay_native_ms_clr->GetColor() };
	case EGlobalContext_t::NETCON_S:
		return { cl_conoverlay_netcon_clr->GetColor() };
	case EGlobalContext_t::COMMON_C:
		return { cl_conoverlay_common_clr->GetColor() };
	case EGlobalContext_t::WARNING_C:
		return { cl_conoverlay_warning_clr->GetColor() };
	case EGlobalContext_t::ERROR_C:
		return { cl_conoverlay_error_clr->GetColor() };
	default:
		return { cl_conoverlay_native_engine_clr->GetColor() };
	}
}

///////////////////////////////////////////////////////////////////////////////
CLogSystem g_pLogSystem;
