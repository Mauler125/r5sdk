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
	LogMsg_t(const std::string svMessage, const int nTicks, const LogType_t type)
	{
		this->m_svMessage = svMessage;
		this->m_nTicks    = nTicks;
		this->m_type      = type;
	}
	std::string m_svMessage = "";
	int         m_nTicks    = 1024;
	LogType_t   m_type      = LogType_t::NONE;
};

class CLogSystem
{
public:
	void Update(void);
	void AddLog(LogType_t type, std::string svText);
	void DrawLog(void);
	void DrawSimStats(void) const;
	void DrawGPUStats(void) const;

private:
	Color GetLogColorForType(LogType_t type) const;
	std::vector<LogMsg_t> m_vLogs{};
	int m_nFontHeight = 16;
};

///////////////////////////////////////////////////////////////////////////////
int HCEngineVGui_Paint(void* thisptr, int nMode);
void CEngineVGui_Attach();
void CEngineVGui_Detach();

///////////////////////////////////////////////////////////////////////////////
extern CLogSystem g_pLogSystem;
