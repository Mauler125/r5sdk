#include "pch.h"
#include "hooks.h"
#include "logsystem.h"

LogSystem g_LogSystem;

void LogSystem::Update()
{
	if (GameGlobals::Cvar->FindVar("cl_drawconsoleoverlay")->m_iValue < 1)
		return;

	if (m_vLogs.empty())
		return;

	static void* pCMatSystemSurface = MemoryAddress(0x14D40B360).RCast<void*(*)()>();
	if (!pCMatSystemSurface)
		return;

	static int fontHeight = 16;

	for (int i = 0; i < m_vLogs.size(); ++i)
	{
		if (m_vLogs[i].Ticks >= 0)
		{
			if (i < GameGlobals::Cvar->FindVar("cl_consoleoverlay_lines")->m_iValue)
			{
				float fadepct = fminf(static_cast<float>(m_vLogs[i].Ticks) / 64.f, 1.0);
				float ptc = static_cast<int>(ceilf( fadepct * 255.f));
				int alpha = static_cast<int>(ptc);
				int y = (10 + (fontHeight * i));

				std::array<int, 3> color = GetLogColorForType(m_vLogs[i].Type);
				addr_CMatSystemSurface_DrawColoredText(pCMatSystemSurface, 0x13, fontHeight, 10, y, color[0], color[1], color[2], alpha, m_vLogs[i].Message.c_str());
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

void LogSystem::AddLog(LogType_t type, std::string message)
{
	if (message.length() > 0)
	{
		m_vLogs.push_back(Log{ message, 1024, type });
	}
}

std::array<int, 3> LogSystem::GetLogColorForType(LogType_t type)
{
	switch (type)
	{
		case LogType_t::NATIVE:
			return { 255, 255, 255 };
		case LogType_t::SCRIPT_SERVER:
			return { 190, 183, 240 };
		case LogType_t::SCRIPT_CLIENT:
			return { 117, 116, 139 };
		case LogType_t::SCRIPT_UI:
			return { 197, 160, 177 };
		default:
			return { 255, 255, 255 };
	}

	return { 255, 255, 255 };
}
