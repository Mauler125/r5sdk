#pragma once

namespace
{
	/* ==== UTILITY ========================================================================================================================================================= */
	ADDRESS p_Sys_Error_Internal = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x08\x48\x89\x74\x24\x10\x57\x48\x81\xEC\x30\x08\x00\x00\x48\x8B\xDA\x48\x8B\xF9\xE8\x00\x00\x00\xFF\x33\xF6\x48", "xxxxxxxxxxxxxxxxxxxxxxxxx???xxxx");
	int (*Sys_Error_Internal)(char* fmt, va_list args) = (int (*)(char*, va_list))p_Sys_Error_Internal.GetPtr(); /*48 89 5C 24 08 48 89 74 24 10 57 48 81 EC 30 08 00 00 48 8B DA 48 8B F9 E8 ?? ?? ?? FF 33 F6 48*/
}

///////////////////////////////////////////////////////////////////////////////
int HSys_Error_Internal(char* fmt, va_list args);

void SysDll_Attach();
void SysDll_Detach();

///////////////////////////////////////////////////////////////////////////////
class HSys_Dll : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: Sys_Error_Internal                   : 0x" << std::hex << std::uppercase << p_Sys_Error_Internal.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HSys_Dll);