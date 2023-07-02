#pragma once

/* ==== COMMON ========================================================================================================================================================== */
inline CMemory p_COM_InitFilesystem;
inline void*(*COM_InitFilesystem)(const char* pFullModPath);

inline CMemory p_COM_ExplainDisconnection;
inline void*(*COM_ExplainDisconnection)(uint64_t level, const char* fmt, ...);

const char* COM_FormatSeconds(int seconds);
///////////////////////////////////////////////////////////////////////////////
class VCommon : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("COM_InitFilesystem", p_COM_InitFilesystem.GetPtr());
		LogFunAdr("COM_ExplainDisconnection", p_COM_ExplainDisconnection.GetPtr());
	}
	virtual void GetFun(void) const
	{
		p_COM_InitFilesystem = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 81 EC ?? ?? ?? ?? 48 8B F9 48 C7 44 24 ?? ?? ?? ?? ??");
		p_COM_ExplainDisconnection = g_GameDll.FindPatternSIMD("48 8B C4 48 89 50 10 4C 89 40 18 4C 89 48 20 48 81 EC ?? ?? ?? ??");

		COM_InitFilesystem = p_COM_InitFilesystem.RCast<void* (*)(const char*)>();                            /*48 89 5C 24 ?? 48 89 74 24 ?? 57 48 81 EC ?? ?? ?? ?? 48 8B F9 48 C7 44 24 ?? ?? ?? ?? ??*/
		COM_ExplainDisconnection = p_COM_ExplainDisconnection.RCast<void* (*)(uint64_t, const char*, ...)>(); /*48 8B C4 48 89 50 10 4C 89 40 18 4C 89 48 20 48 81 EC ?? ?? ?? ??*/
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////
