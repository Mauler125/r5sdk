#pragma once
#include "core/stdafx.h"
#include "mathlib/color.h"

struct CTextNotify
{
	CTextNotify(const eDLL_T type, const float nTime, const string& svMessage)
	{
		this->m_svMessage       = svMessage;
		this->m_flLifeRemaining = nTime;
		this->m_type            = type;
	}
	eDLL_T m_type            = eDLL_T::NONE;
	float  m_flLifeRemaining = 0.0f;
	string m_svMessage       = "";
};

class CTextOverlay
{
public:
	CTextOverlay()
	{
		m_nFontHeight = 16;
		m_nCon_NPrintf_Idx = 0;
		memset(m_szCon_NPrintf_Buf, '\0', sizeof(m_szCon_NPrintf_Buf));
	}

	void Update(void);
	void AddLog(const eDLL_T context, const string& svText);
	void DrawNotify(void);
	void DrawFormat(const int x, const int y, const Color c, const char* pszFormat, ...) const;
	void ShouldDraw(const float flFrameTime);
	void DrawSimStats(void) const;
	void DrawGPUStats(void) const;
	void DrawCrosshairMaterial(void) const;
	void DrawStreamOverlay(void) const;

	void Con_NPrintf(void);

private:
	Color GetLogColorForType(const eDLL_T type) const;
	vector<CTextNotify> m_vNotifyText;
	int m_nFontHeight; // Hardcoded to 16 in this engine.

	mutable std::mutex m_Mutex;

public:
	int m_nCon_NPrintf_Idx;
	char m_szCon_NPrintf_Buf[4096];
};

///////////////////////////////////////////////////////////////////////////////
extern CTextOverlay* g_pOverlay;
