#pragma once
#include "core/stdafx.h"
#include "mathlib/color.h"

enum class LogType_t : int
{
	SCRIPT_SERVER,
	SCRIPT_CLIENT,
	SCRIPT_UI,
	NATIVE_SERVER,
	NATIVE_CLIENT,
	NATIVE_UI,
	NATIVE_ENGINE,
	NATIVE_FS,
	NATIVE_RTECH,
	NATIVE_MS,
	NETCON_S,
	WARNING_C,
	ERROR_C,
	NONE
};

struct LogMsg_t
{
	LogMsg_t(const string svMessage, const int nTicks, const LogType_t type)
	{
		this->m_svMessage = svMessage;
		this->m_nTicks    = nTicks;
		this->m_type      = type;
	}
	string      m_svMessage = "";
	int         m_nTicks    = 1024;
	LogType_t   m_type      = LogType_t::NONE;
};

class CLogSystem
{
public:
	void Update(void);
	void AddLog(LogType_t type, string svText);
	void DrawLog(void);
	void DrawHostStats(void) const;
	void DrawSimStats(void) const;
	void DrawGPUStats(void) const;
	void DrawCrosshairMaterial(void) const;
	void DrawStreamOverlay(void) const;

private:
	Color GetLogColorForType(LogType_t type) const;
	vector<LogMsg_t> m_vLogs{};
	int m_nFontHeight = 16;

public:
	char* m_pszCon_NPrintf_Buf[4096]{};
};

///////////////////////////////////////////////////////////////////////////////
void CEngineVGui_Attach();
void CEngineVGui_Detach();

///////////////////////////////////////////////////////////////////////////////
extern CLogSystem g_pLogSystem;
