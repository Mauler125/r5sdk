#pragma once

/* ==== COMMON ========================================================================================================================================================== */
inline void*(*v_COM_InitFilesystem)(const char* pFullModPath);
inline char* const(*v_COM_GetPrintMessageBuffer)(void);
inline void(*v_COM_ExplainDisconnection)(bool bPrint, const char* fmt, ...);

const char* COM_FormatSeconds(int seconds);
void COM_ExplainDisconnection(bool bPrint, const char* fmt, ...);
///////////////////////////////////////////////////////////////////////////////
class VCommon : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("COM_InitFilesystem", v_COM_InitFilesystem);
		LogFunAdr("COM_GetPrintMessageBuffer", v_COM_GetPrintMessageBuffer);
		LogFunAdr("COM_ExplainDisconnection", v_COM_ExplainDisconnection);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 81 EC ?? ?? ?? ?? 48 8B F9 48 C7 44 24 ?? ?? ?? ?? ??").GetPtr(v_COM_InitFilesystem);
		g_GameDll.FindPatternSIMD("48 8D 05 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC 4C 89 44 24 ??").GetPtr(v_COM_GetPrintMessageBuffer);
		g_GameDll.FindPatternSIMD("48 8B C4 48 89 50 10 4C 89 40 18 4C 89 48 20 48 81 EC ?? ?? ?? ??").GetPtr(v_COM_ExplainDisconnection);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
