#pragma once

/* ==== COMMON ========================================================================================================================================================== */
inline CMemory p_COM_InitFilesystem;
inline auto COM_InitFilesystem = p_COM_InitFilesystem.RCast<void* (*)(const char* pFullModPath)>();

inline CMemory p_COM_ExplainDisconnection;
inline auto COM_ExplainDisconnection = p_COM_ExplainDisconnection.RCast<void* (*)(uint64_t level, const char* fmt, ...)>();

///////////////////////////////////////////////////////////////////////////////
class VCommon : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: COM_InitFilesystem                   : {:#18x} |\n", p_COM_InitFilesystem.GetPtr());
		spdlog::debug("| FUN: COM_ExplainDisconnection             : {:#18x} |\n", p_COM_ExplainDisconnection.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
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

REGISTER(VCommon);
