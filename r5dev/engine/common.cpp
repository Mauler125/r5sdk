//=====================================================================================//
//
// Purpose: 
//
//=====================================================================================//

#include <core/stdafx.h>
#include <tier1/strtools.h>
#include <localize/localize.h>
#include <engine/common.h>

/*
==============================
COM_FormatSeconds

==============================
*/
const char* COM_FormatSeconds(int seconds)
{
	static char string[64];

	int hours = 0;
	int minutes = seconds / 60;

	if (minutes > 0)
	{
		seconds -= (minutes * 60);
		hours = minutes / 60;

		if (hours > 0)
		{
			minutes -= (hours * 60);
		}
	}

	if (hours > 0)
	{
		Q_snprintf(string, sizeof(string), "%2i:%02i:%02i", hours, minutes, seconds);
	}
	else
	{
		Q_snprintf(string, sizeof(string), "%02i:%02i", minutes, seconds);
	}

	return string;
}

/*
==============================
COM_ExplainDisconnection

==============================
*/
void COM_ExplainDisconnection(bool bPrint, const char* fmt, ...)
{
	char szBuf[1024];
	{/////////////////////////////
		va_list vArgs;
		va_start(vArgs, fmt);

		vsnprintf(szBuf, sizeof(szBuf), fmt, vArgs);

		szBuf[sizeof(szBuf) - 1] = '\0';
		va_end(vArgs);
	}/////////////////////////////

	if (bPrint)
	{
		if (szBuf[0] == '#')
		{
			wchar_t formatStr[1024];
			const wchar_t* wpchReason = (*g_ppVGuiLocalize) ? (*g_ppVGuiLocalize)->Find(szBuf) : nullptr;
			if (wpchReason)
			{
				wcsncpy(formatStr, wpchReason, sizeof(formatStr) / sizeof(wchar_t));

				char conStr[256];
				(*g_ppVGuiLocalize)->ConvertUnicodeToANSI(formatStr, conStr, sizeof(conStr));
				Error(eDLL_T::CLIENT, NO_ERROR, "%s\n", conStr);
			}
			else
				Error(eDLL_T::CLIENT, NO_ERROR, "%s\n", szBuf);
		}
		else
		{
			Error(eDLL_T::CLIENT, NO_ERROR, "%s\n", szBuf);
		}
	}

	v_COM_ExplainDisconnection(bPrint, szBuf);
}

void VCommon::Detour(const bool bAttach) const
{
	DetourSetup(&v_COM_ExplainDisconnection, COM_ExplainDisconnection, bAttach);
}
