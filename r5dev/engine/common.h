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
		p_COM_InitFilesystem = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xF9\x48\xC7\x44\x24\x00\x00\x00\x00\x00"), "xxxx?xxxx?xxxx????xxxxxxx?????");
		p_COM_ExplainDisconnection = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x48\x89\x50\x10\x4C\x89\x40\x18\x4C\x89\x48\x20\x48\x81\xEC\x00\x00\x00\x00"), "xxxxxxxxxxxxxxxxxx????");

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
