#pragma once
#include "squirrel/sqtype.h"

namespace
{
	/* ==== SQUIRREL ======================================================================================================================================================== */
	ADDRESS p_sq_pushbool = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x83\xEC\x38\x33\xC0\x48\xC7\x44\x24\x20\x08\x00\x00\x01\x48", "xxxxxxxxxxxxxxxx");
	void (*v_sq_pushbool)(HSQUIRRELVM* v, SQBool b) = (void (*)(HSQUIRRELVM*, SQBool))p_sq_pushbool.GetPtr(); /*48 83 EC 38 33 C0 48 C7 44 24 20 08 00 00 01 48*/
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1) || defined (GAMEDLL_S2)
	ADDRESS p_sq_pushstring = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x56\x48\x83\xEC\x30\x48\x8B\xF1\x48\x85\xD2\x0F\x84\x8C\x00", "xxxxxxxxxxxxxxxx");
	void (*v_sq_pushstring)(HSQUIRRELVM* v, const SQChar* string, int len) = (void (*)(HSQUIRRELVM*, const SQChar*, int))p_sq_pushstring.GetPtr(); /*40 56 48 83 EC 30 48 8B F1 48 85 D2 0F 84 8C 00*/
#elif defined (GAMEDLL_S3)
	ADDRESS p_sq_pushstring = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x56\x48\x83\xEC\x30\x48\x8B\xF1\x48\x85\xD2\x0F\x84\x8F\x00", "xxxxxxxxxxxxxxxx");
	void (*v_sq_pushstring)(HSQUIRRELVM* v, const SQChar* string, SQInteger len) = (void (*)(HSQUIRRELVM*, const SQChar*, SQInteger))p_sq_pushstring.GetPtr(); /*40 56 48 83 EC 30 48 8B F1 48 85 D2 0F 84 8F 00*/
#endif
	ADDRESS p_sq_pushinteger = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x83\xEC\x38\x33\xC0\x48\xC7\x44\x24\x20\x02\x00\x00\x05\x48", "xxxxxxxxxxxxxxxx");
	void (*v_sq_pushinteger)(HSQUIRRELVM* v, SQInteger val) = (void (*)(HSQUIRRELVM*, SQInteger))p_sq_pushinteger.GetPtr(); /*48 83 EC 38 33 C0 48 C7 44 24 20 02 00 00 05 48*/

	ADDRESS p_sq_pushconstant = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x30\x4C\x8B", "xxxx?xxxx?xxxx?xxxxxxx");
	void (*v_sq_pushconstant)(HSQUIRRELVM* v, const SQChar* name, SQInteger val) = (void (*)(HSQUIRRELVM*, const SQChar*, SQInteger))p_sq_pushconstant.GetPtr(); /*48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 30 4C 8B*/

	ADDRESS p_sq_newarray = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x30\x48\x8B\xD9\x48\xC7\x44\x24\x20\x40", "xxxxxxxxxxxxxxxxxxx");
	void (*v_sq_newarray)(HSQUIRRELVM* v, SQInteger size) = (void (*)(HSQUIRRELVM*, SQInteger))p_sq_newarray.GetPtr(); /*48 89 5C 24 08 57 48 83 EC 30 48 8B D9 48 C7 44 24 20 40*/

	ADDRESS p_sq_arrayappend = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x53\x48\x83\xEC\x20\x8B\x41\x00\x48\x8B\xD9\x2B\x41\x00\x83\xF8\x02\x7D", "xxxxxxxx?xxxxx?xxxx");
	void (*v_sq_arrayappend)(HSQUIRRELVM* v, SQInteger idx) = (void (*)(HSQUIRRELVM*, SQInteger))p_sq_arrayappend.GetPtr(); /*40 53 48 83 EC 20 8B 41 ?? 48 8B D9 2B 41 ?? 83 F8 02 7D*/

	ADDRESS p_sq_newtable = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x30\x48\x8B\xD9\x48\xC7\x44\x24\x20\x20", "xxxxxxxxxxxxxxxxxxx");
	void (*v_sq_newtable)(HSQUIRRELVM* v) = (void (*)(HSQUIRRELVM*))p_sq_newtable.GetPtr(); /*48 89 5C 24 08 57 48 83 EC 30 48 8B D9 48 C7 44 24 20 20*/

	ADDRESS p_sq_newslot = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x53\x48\x83\xEC\x30\x44\x8B\x49\x00\x48\x8B\xD9\x41\x8B\xC1", "xxxxxxxxx?xxxxxx");
	void (*v_sq_newslot)(HSQUIRRELVM* v, SQInteger idx) = (void (*)(HSQUIRRELVM*, SQInteger))p_sq_newslot.GetPtr(); /*40 53 48 83 EC 20 8B 41 ?? 48 8B D9 2B 41 ?? 83 F8 02 7D*/
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1) || defined (GAMEDLL_S2)
	ADDRESS p_sq_pushstructure = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x4C\x89\x4C\x24\x00\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8B\xEC", "xxxx?xxxx?xxxx?xxxx?xxxxxxxxxxxx");
	void (*v_sq_pushstructure)(HSQUIRRELVM* v, const SQChar* name, const SQChar* member, const SQChar* codeclass1, const SQChar* codeclass2) = (void (*)(HSQUIRRELVM*, const SQChar*, const SQChar*, const SQChar*, const SQChar*))p_sq_pushstructure.GetPtr(); /*48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 4C 89 4C 24 ? 55 41 54 41 55 41 56 41 57 48 8B EC*/
#elif defined (GAMEDLL_S3)
	ADDRESS p_sq_pushstructure = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8B\xEC\x48\x83\xEC\x60\x48\x8B\x59\x60", "xxxx?xxxx?xxxx?xxxxxxxxxxxxxxxxxxxx");
	void (*v_sq_pushstructure)(HSQUIRRELVM* v, const SQChar* name, const SQChar* member, const SQChar* codeclass1, const SQChar* codeclass2) = (void (*)(HSQUIRRELVM*, const SQChar*, const SQChar*, const SQChar*, const SQChar*))p_sq_pushstructure.GetPtr(); /*48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC 60 48 8B 59 60*/
#endif
	ADDRESS p_sq_compilebuffer = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x41\x56\x41\x57\x48\x83\xEC\x50\x41\x8B\xE9\x49\x8B\xF8", "xxxx?xxxx?xxxx?xxxxxxxxxxxxxxx");
	SQRESULT (*v_sq_compilebuffer)(HSQUIRRELVM v, SQBufState bufferState, const SQChar* buffer, SQCONTEXT context) = (SQRESULT (*)(HSQUIRRELVM, SQBufState, const SQChar*, SQCONTEXT))p_sq_compilebuffer.GetPtr(); /*48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC 50 41 8B E9 49 8B F8 */
}

///////////////////////////////////////////////////////////////////////////////
char* sq_getstring(HSQUIRRELVM* v, SQInteger i);
int sq_getinteger(HSQUIRRELVM* v, SQInteger i);
void sq_pushbool(HSQUIRRELVM* v, SQBool b);
void sq_pushstring(HSQUIRRELVM* v, const SQChar* string, SQInteger len);
void sq_pushinteger(HSQUIRRELVM* v, SQInteger val);
void sq_pushconstant(HSQUIRRELVM* v, const SQChar* name, SQInteger val);
void sq_newarray(HSQUIRRELVM* v, SQInteger size);
void sq_arrayappend(HSQUIRRELVM* v, SQInteger idx);
void sq_newtable(HSQUIRRELVM* v);
void sq_newslot(HSQUIRRELVM* v, SQInteger idx);
void sq_pushstructure(HSQUIRRELVM* v, const SQChar* name, const SQChar* member, const SQChar* codeclass1, const SQChar* codeclass2);

void SQAPI_Attach();
void SQAPI_Detach();

///////////////////////////////////////////////////////////////////////////////
class HSqapi : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: sq_pushbool                          : 0x" << std::hex << std::uppercase << p_sq_pushbool.GetPtr()      << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: sq_pushstring                        : 0x" << std::hex << std::uppercase << p_sq_pushstring.GetPtr()    << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: sq_pushinteger                       : 0x" << std::hex << std::uppercase << p_sq_pushinteger.GetPtr()   << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: sq_pushconstant                      : 0x" << std::hex << std::uppercase << p_sq_pushconstant.GetPtr()  << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: sq_newarray                          : 0x" << std::hex << std::uppercase << p_sq_newarray.GetPtr()      << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: sq_arrayappend                       : 0x" << std::hex << std::uppercase << p_sq_arrayappend.GetPtr()   << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: sq_newtable                          : 0x" << std::hex << std::uppercase << p_sq_newtable.GetPtr()      << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: sq_newslot                           : 0x" << std::hex << std::uppercase << p_sq_newslot.GetPtr()       << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: sq_pushstructure                     : 0x" << std::hex << std::uppercase << p_sq_pushstructure.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: sq_compilebuffer                     : 0x" << std::hex << std::uppercase << p_sq_compilebuffer.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HSqapi);
