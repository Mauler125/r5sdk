#pragma once

typedef int SQRESULT;
#define SQ_OK (1)
#define SQ_ERROR (-1)
#define SQ_FAILED(res) (res<0)
#define SQ_SUCCEEDED(res) (res>=0)

struct SQFuncRegistration
{
	const char* m_szScriptName; // 00
	const char* m_szNativeName; // 08
	const char* m_szHelpString; // 10
	const char* m_szRetValType; // 18
	const char* m_szArgTypes;   // 20
	std::int16_t unk28;         // 28
	std::int16_t padding1;      // 2A
	std::int32_t unk2c;         // 2C
	std::int64_t unk30;         // 30
	std::int32_t unk38;         // 38
	std::int32_t padding2;      // 3C
	std::int64_t unk40;         // 40
	std::int64_t unk48;         // 48
	std::int64_t unk50;         // 50
	std::int32_t unk58;         // 58
	std::int32_t padding3;      // 5C
	void* m_pFunction;          // 60

	SQFuncRegistration()
	{
		memset(this, 0, sizeof(SQFuncRegistration));
		this->padding2 = 6;
	}
};

const static std::string SQVM_LOG_T[4] =
{
	"Script(S):",
	"Script(C):",
	"Script(U):",
	"Script(X):"
};

const static std::string SQVM_ANSI_LOG_T[4] =
{
	"\033[38;2;151;149;187mScript(S):",
	"\033[38;2;151;149;163mScript(C):",
	"\033[38;2;151;123;136mScript(U):",
	"\033[38;2;151;149;163mScript(X):"
};

namespace
{
	/* ==== SQUIRREL ======================================================================================================================================================== */
	ADDRESS p_SQVM_PrintFunc = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x8B\xC4\x48\x89\x50\x10\x4C\x89\x40\x18\x4C\x89\x48\x20\x53\x56\x57\x48\x81\xEC\x30\x08\x00\x00\x48\x8B\xDA\x48\x8D\x70\x18\x48\x8B\xF9\xE8\x00\x00\x00\xFF\x48\x89\x74\x24\x28\x48\x8D\x54\x24\x30\x33", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx???xxxxxxxxxxxx");
	void* SQVM_PrintFunc = (void*)p_SQVM_PrintFunc.GetPtr(); /*48 8B C4 48 89 50 10 4C 89 40 18 4C 89 48 20 53 56 57 48 81 EC 30 08 00 00 48 8B DA 48 8D 70 18 48 8B F9 E8 ?? ?? ?? FF 48 89 74 24 28 48 8D 54 24 30 33*/

	ADDRESS p_SQVM_WarningFunc = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x4C\x89\x4C\x24\x20\x44\x89\x44\x24\x18\x89\x54\x24\x10\x53\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x83\xEC\x00\x48\x8B", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxx?xx");
	void* (*SQVM_WarningFunc)(void* sqvm, int a2, int a3, int* nStringSize, void** ppString) = (void* (*)(void*, int, int, int*, void**))p_SQVM_WarningFunc.GetPtr(); /*4C 89 4C 24 20 44 89 44 24 18 89 54 24 10 53 55 56 57 41 54 41 55 41 56 41 57 48 83 EC ?? 48 8B*/

	ADDRESS p_SQVM_WarningCmd = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x53\x48\x83\xEC\x30\x33\xDB\x48\x8D\x44\x24\x00\x4C\x8D\x4C\x24\x00", "xxxxxxxxxxxx?xxxx?");
	void* (*SQVM_WarningCmd)(int a1, int a2) = (void* (*)(int, int))p_SQVM_WarningCmd.GetPtr(); /*40 53 48 83 EC 30 33 DB 48 8D 44 24 ?? 4C 8D 4C 24 ??*/
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	ADDRESS p_SQVM_LoadScript = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x10\x48\x89\x74\x24\x18\x48\x89\x7C\x24\x20\x48\x89\x4C\x24\x08\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\x6C", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
	bool (*SQVM_LoadScript)(void* sqvm, const char* szScriptPath, const char* szScriptName, int nFlag) = (bool (*)(void*, const char*, const char*, int))p_SQVM_LoadScript.GetPtr(); /*48 89 5C 24 10 48 89 74 24 18 48 89 7C 24 20 48 89 4C 24 08 55 41 54 41 55 41 56 41 57 48 8D 6C*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	ADDRESS p_SQVM_LoadScript = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x8B\xC4\x48\x89\x48\x08\x55\x41\x56\x48\x8D\x68", "xxxxxxxxxxxxx"); /*48 8B C4 48 89 48 08 55 41 56 48 8D 68*/
	bool (*SQVM_LoadScript)(void* sqvm, const char* szScriptPath, const char* szScriptName, int nFlag) = (bool (*)(void*, const char*, const char*, int))p_SQVM_LoadScript.GetPtr();
#endif
	ADDRESS p_SQVM_LoadRson = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x4C\x8B\xDC\x49\x89\x5B\x08\x57\x48\x81\xEC\xA0\x00\x00\x00\x33", "xxxxxxxxxxxxxxxx");
	void* (*SQVM_LoadRson)(const char* szRsonName) = (void* (*)(const char*))p_SQVM_LoadRson.GetPtr(); /*4C 8B DC 49 89 5B 08 57 48 81 EC A0 00 00 00 33*/

	ADDRESS p_SQVM_RegisterFunc = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x83\xEC\x38\x45\x0F\xB6\xC8", "xxxxxxxx"); /*48 83 EC 38 45 0F B6 C8*/
	void* (*SQVM_RegisterFunc)(void* sqvm, SQFuncRegistration* sqFunc, int a1) = (void* (*)(void*, SQFuncRegistration*, int))p_SQVM_RegisterFunc.GetPtr();

	ADDRESS p_SQVM_CreateUIVM = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x53\x48\x83\xEC\x20\x48\x8B\x1D\x00\x00\x00\x00\xC6\x05\x00\x00\x00\x00\x00", "xxxxxxxxx????xx?????"); /*40 53 48 83 EC 20 48 8B 1D ? ? ? ? C6 05 ? ? ? ? ?*/
	bool (*SQVM_CreateUIVM)() = (bool(*)())p_SQVM_CreateUIVM.GetPtr();
	ADDRESS p_SQVM_UIVM = (void*)p_SQVM_CreateUIVM.FindPatternSelf("48 8B 1D", ADDRESS::Direction::DOWN, 50).ResolveRelativeAddressSelf(0x3, 0x7).GetPtr();

	ADDRESS p_SQVM_RegisterOriginFuncs = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\xE8\x00\x00\x00\x00\x48\x8B\x0D\x00\x00\x00\x00\x48\x8D\x15\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8B\x05\x00\x00\x00\x00\xC7\x05\x00\x00\x00\x00\x00\x00\x00\x00", "x????xxx????xxx????x????xxx????xx????????"); /*E8 ? ? ? ? 48 8B 0D ? ? ? ? 48 8D 15 ? ? ? ? E8 ? ? ? ? 48 8B 05 ? ? ? ? C7 05 ? ? ? ? ? ? ? ?*/
	void (*SQVM_RegisterOriginFuncs)(void* sqvm) = (void(*)(void*))p_SQVM_RegisterOriginFuncs.FollowNearCall().GetPtr();
}

void* HSQVM_PrintFunc(void* sqvm, char* fmt, ...);
void* HSQVM_LoadRson(const char* szRsonName);
bool HSQVM_LoadScript(void* sqvm, const char* szScriptPath, const char* szScriptName, int nFlags);

void HSQVM_RegisterFunction(void* sqvm, const char* szName, const char* szHelpString, const char* szRetValType, const char* szArgTypes, void* pFunction);
void HSQVM_RegisterOriginFuncs(void* sqvm);

void SQVM_Attach();
void SQVM_Detach();

///////////////////////////////////////////////////////////////////////////////
class HSQVM : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: SQVM_PrintFunc                       : 0x" << std::hex << std::uppercase << p_SQVM_PrintFunc.GetPtr()           << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: SQVM_WarningFunc                     : 0x" << std::hex << std::uppercase << p_SQVM_WarningFunc.GetPtr()         << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: SQVM_WarningCmd                      : 0x" << std::hex << std::uppercase << p_SQVM_WarningCmd.GetPtr()          << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: SQVM_LoadScript                      : 0x" << std::hex << std::uppercase << p_SQVM_LoadScript.GetPtr()          << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: SQVM_LoadRson                        : 0x" << std::hex << std::uppercase << p_SQVM_LoadRson.GetPtr()            << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: SQVM_RegisterFunc                    : 0x" << std::hex << std::uppercase << p_SQVM_RegisterFunc.GetPtr()        << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: SQVM_CreateUIVM                      : 0x" << std::hex << std::uppercase << p_SQVM_CreateUIVM.GetPtr()          << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: SQVM_RegisterOriginFuncs             : 0x" << std::hex << std::uppercase << p_SQVM_RegisterOriginFuncs.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HSQVM);
