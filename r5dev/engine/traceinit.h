#ifndef TRACEINIT_H
#define TRACEINIT_H

inline CMemory p_TRACEINIT;
inline auto v_TRACEINIT = p_TRACEINIT.RCast<void(*)(void* undef, const char* initfunc, const char* shutdownfunc)>();

///////////////////////////////////////////////////////////////////////////////
class VTraceInit : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: TRACEINIT                            : {:#18x} |\n", p_TRACEINIT.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_TRACEINIT = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\x48\x8B\x05\x00\x00\x00\x00\x49\x8B\xF8\x48\x8B\xF2\x48\x85\xC0"), "xxxx?xxxx?xxxxxxxx????xxxxxxxxx");
		v_TRACEINIT = p_TRACEINIT.RCast<void (*)(void*, const char*, const char*)>(); /*48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 8B 05 ? ? ? ? 49 8B F8 48 8B F2 48 85 C0*/
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VTraceInit);

#endif // TRACEINIT_H
