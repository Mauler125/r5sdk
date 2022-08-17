#pragma once
#include "core/stdafx.h"
#include "mathlib/color.h"

struct LogMsg_t
{
	LogMsg_t(const string svMessage, const int nTicks, const EGlobalContext_t type)
	{
		this->m_svMessage = svMessage;
		this->m_nTicks    = nTicks;
		this->m_type      = type;
	}
	string           m_svMessage = "";
	int              m_nTicks    = 1024;
	EGlobalContext_t m_type      = EGlobalContext_t::NONE;
};

class CLogSystem
{
public:
	CLogSystem()
	{
		m_nFontHeight = 16;
		memset(m_pszCon_NPrintf_Buf, '\0', sizeof(m_pszCon_NPrintf_Buf));
	}

	void Update(void);
	void AddLog(const EGlobalContext_t context, const string& svText);
	void DrawLog(void);
	void DrawHostStats(void) const;
	void DrawSimStats(void) const;
	void DrawGPUStats(void) const;
	void DrawCrosshairMaterial(void) const;
	void DrawStreamOverlay(void) const;

private:
	Color GetLogColorForType(const EGlobalContext_t type) const;
	vector<LogMsg_t> m_vLogs;
	int m_nFontHeight;

public:
	char m_pszCon_NPrintf_Buf[4096]{};
};

///////////////////////////////////////////////////////////////////////////////
void CEngineVGui_Attach();
void CEngineVGui_Detach();

///////////////////////////////////////////////////////////////////////////////
extern CLogSystem g_pLogSystem;
