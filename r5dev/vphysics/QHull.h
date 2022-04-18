#pragma once

inline CMemory p_QHull_PrintFunc;
inline auto QHull_PrintFunc = p_QHull_PrintFunc.RCast<int (*)(const char* fmt, ...)>();

//inline CMemory p_speex_warning_int;
//inline auto speex_warning_int = p_speex_warning_int.RCast<int (*)(FILE* stream, const char* format, ...)>();

///////////////////////////////////////////////////////////////////////////////
int HQHull_PrintFunc(const char* fmt, ...);

void QHull_Attach();
void QHull_Detach();

///////////////////////////////////////////////////////////////////////////////
class HQHull : public IDetour
{
	virtual void GetAdr(void) const
	{
		std::cout << "| FUN: QHull_PrintFunc                      : 0x" << std::hex << std::uppercase << p_QHull_PrintFunc.GetPtr() << std::setw(nPad) << " |" << std::endl;
		//std::cout << "| FUN: speex_warning_int                    : 0x" << std::hex << std::uppercase << p_speex_warning_int.GetPtr() << std::setw(nPad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
	virtual void GetFun(void) const
	{
		p_QHull_PrintFunc = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x4C\x24\x08\x48\x89\x54\x24\x10\x4C\x89\x44\x24\x18\x4C\x89\x4C\x24\x20\x53\xB8\x40\x27\x00\x00\x00\x00\x00\x00\x00\x48"), "xxxxxxxxxxxxxxxxxxxxxxxxxx????xx");
		QHull_PrintFunc = p_QHull_PrintFunc.RCast<int (*)(const char* fmt, ...)>(); /*48 89 4C 24 08 48 89 54 24 10 4C 89 44 24 18 4C 89 4C 24 20 53 B8 40 27 00 00 ?? ?? ?? ?? 00 48*/

		//p_speex_warning_int = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x54\x24\x10\x4C\x89\x44\x24\x18\x4C\x89\x4C\x24\x20\x53\x56\x57\x48\x83\xEC\x30\x48\x8B\xFA\x48\x8D\x74\x24\x60\x48\x8B"), "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
		//speex_warning_int = p_speex_warning_int.RCast<int (*)(FILE* stream, const char* format, ...)>(); /*48 89 54 24 10 4C 89 44 24 18 4C 89 4C 24 20 53 56 57 48 83 EC 30 48 8B FA 48 8D 74 24 60 48 8B*/
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HQHull);
