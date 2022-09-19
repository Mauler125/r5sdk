#pragma once
#include "engine/common.h"

/* ==== UTILITY ========================================================================================================================================================= */
inline CMemory p_Sys_Error_Internal;
inline auto Sys_Error_Internal = p_Sys_Error_Internal.RCast<int (*)(char* fmt, va_list args)>();

inline bool* gfExtendedError = nullptr;

///////////////////////////////////////////////////////////////////////////////
int HSys_Error_Internal(char* fmt, va_list args);

void SysDll_Attach();
void SysDll_Detach();

///////////////////////////////////////////////////////////////////////////////
class VSys_Dll : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: Sys_Error_Internal                   : {:#18x} |\n", p_Sys_Error_Internal.GetPtr());
		spdlog::debug("| VAR: gfExtendedError                      : {:#18x} |\n", reinterpret_cast<uintptr_t>(gfExtendedError));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_Sys_Error_Internal = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x08\x48\x89\x74\x24\x10\x57\x48\x81\xEC\x30\x08\x00\x00\x48\x8B\xDA\x48\x8B\xF9\xE8\x00\x00\x00\xFF\x33\xF6\x48"), "xxxxxxxxxxxxxxxxxxxxxxxxx???xxxx");
		Sys_Error_Internal = p_Sys_Error_Internal.RCast<int (*)(char*, va_list)>(); /*48 89 5C 24 08 48 89 74 24 10 57 48 81 EC 30 08 00 00 48 8B DA 48 8B F9 E8 ?? ?? ?? FF 33 F6 48*/
	}
	virtual void GetVar(void) const
	{
		gfExtendedError = p_COM_ExplainDisconnection.Offset(0x0).FindPatternSelf("C6 05", CMemory::Direction::DOWN, 300).ResolveRelativeAddressSelf(0x2, 0x7).RCast<bool*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VSys_Dll);