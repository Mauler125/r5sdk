#pragma once
#include "tier0/basetypes.h"

namespace
{
	/* ==== SQUIRREL ======================================================================================================================================================== */
	ADDRESS p_sq_pushbool = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x83\xEC\x38\x33\xC0\x48\xC7\x44\x24\x20\x08\x00\x00\x01\x48", "xxxxxxxxxxxxxxxx");
	void (*sq_pushbool)(void* sqvm, int val) = (void (*)(void*, int))p_sq_pushbool.GetPtr(); /*48 83 EC 38 33 C0 48 C7 44 24 20 08 00 00 01 48*/
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1) || defined (GAMEDLL_S2)
	ADDRESS p_sq_pushstring = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x56\x48\x83\xEC\x30\x48\x8B\xF1\x48\x85\xD2\x0F\x84\x8C\x00", "xxxxxxxxxxxxxxxx");
	void (*sq_pushstring)(void* sqvm, const char* string, int len) = (void (*)(void*, const char*, int))p_sq_pushstring.GetPtr(); /*40 56 48 83 EC 30 48 8B F1 48 85 D2 0F 84 8C 00*/
#elif defined (GAMEDLL_S3)
	ADDRESS p_sq_pushstring = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x56\x48\x83\xEC\x30\x48\x8B\xF1\x48\x85\xD2\x0F\x84\x8F\x00", "xxxxxxxxxxxxxxxx");
	void (*sq_pushstring)(void* sqvm, const char* string, int len) = (void (*)(void*, const char*, int))p_sq_pushstring.GetPtr(); /*40 56 48 83 EC 30 48 8B F1 48 85 D2 0F 84 8F 00*/
#endif
	ADDRESS p_sq_pushinteger = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x83\xEC\x38\x33\xC0\x48\xC7\x44\x24\x20\x02\x00\x00\x05\x48", "xxxxxxxxxxxxxxxx");
	void (*sq_pushinteger)(void* sqvm, int val) = (void (*)(void*, int))p_sq_pushinteger.GetPtr(); /*48 83 EC 38 33 C0 48 C7 44 24 20 02 00 00 05 48*/

	ADDRESS p_sq_newarray = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x30\x48\x8B\xD9\x48\xC7\x44\x24\x20\x40", "xxxxxxxxxxxxxxxxxxx");
	void (*sq_newarray)(void* sqvm, int size) = (void (*)(void*, int))p_sq_newarray.GetPtr(); /*48 89 5C 24 08 57 48 83 EC 30 48 8B D9 48 C7 44 24 20 40*/

	ADDRESS p_sq_arrayappend = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x53\x48\x83\xEC\x20\x8B\x41\x00\x48\x8B\xD9\x2B\x41\x00\x83\xF8\x02\x7D", "xxxxxxxx?xxxxx?xxxx");
	void (*sq_arrayappend)(void* sqvm, int idx) = (void (*)(void*, int))p_sq_arrayappend.GetPtr(); /*40 53 48 83 EC 20 8B 41 ?? 48 8B D9 2B 41 ?? 83 F8 02 7D*/

	ADDRESS p_sq_newtable = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x30\x48\x8B\xD9\x48\xC7\x44\x24\x20\x20", "xxxxxxxxxxxxxxxxxxx");
	void (*sq_newtable)(void* sqvm) = (void (*)(void*))p_sq_newtable.GetPtr(); /*48 89 5C 24 08 57 48 83 EC 30 48 8B D9 48 C7 44 24 20 20*/

	ADDRESS p_sq_newslot = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x53\x48\x83\xEC\x30\x44\x8B\x49\x00\x48\x8B\xD9\x41\x8B\xC1", "xxxxxxxxx?xxxxxx");
	void (*sq_newslot)(void* sqvm, int idx) = (void (*)(void*, int))p_sq_newslot.GetPtr(); /*40 53 48 83 EC 20 8B 41 ?? 48 8B D9 2B 41 ?? 83 F8 02 7D*/
}

///////////////////////////////////////////////////////////////////////////////
char* hsq_getstring(void* sqvm, int i);
int hsq_getinteger(void* sqvm, int i);

void hsq_pushbool(void* sqvm, int val);

void hsq_pushstring(void* sqvm, const char* string, int len);

void hsq_pushinteger(void* sqvm, int val);

void hsq_newarray(void* sqvm, int size);
void hsq_arrayappend(void* sqvm, int idx);

void hsq_newtable(void* sqvm);
void hsq_newslot(void* sqvm, int idx);

void SQAPI_Attach();
void SQAPI_Detach();

///////////////////////////////////////////////////////////////////////////////
class HSqapi : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: sq_pushbool                          : 0x" << std::hex << std::uppercase << p_sq_pushbool.GetPtr()    << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: sq_pushstring                        : 0x" << std::hex << std::uppercase << p_sq_pushstring.GetPtr()  << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: sq_pushinteger                       : 0x" << std::hex << std::uppercase << p_sq_pushinteger.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: sq_newarray                          : 0x" << std::hex << std::uppercase << p_sq_newarray.GetPtr()    << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: sq_arrayappend                       : 0x" << std::hex << std::uppercase << p_sq_arrayappend.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: sq_newtable                          : 0x" << std::hex << std::uppercase << p_sq_newtable.GetPtr()    << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: sq_newslot                           : 0x" << std::hex << std::uppercase << p_sq_newslot.GetPtr()     << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HSqapi);
