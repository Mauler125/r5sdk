#pragma once

//-------------------------------------------------------------------------
// RUNTIME: CL_CLEARSTATE
//-------------------------------------------------------------------------
inline CMemory p_CL_ClearState;
inline auto CL_ClearState = p_CL_ClearState.RCast<int(*)(void)>();

inline CMemory p_CL_EndMovie;
inline auto CL_EndMovie = p_CL_EndMovie.RCast<int(*)(void)>();


///////////////////////////////////////////////////////////////////////////////
class HCL_Main : public IDetour
{
	virtual void GetAdr(void) const
	{
		std::cout << "| FUN: CL_EndMovie                          : 0x" << std::hex << std::uppercase << p_CL_EndMovie.GetPtr()   << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: CL_ClearState                        : 0x" << std::hex << std::uppercase << p_CL_ClearState.GetPtr() << std::setw(nPad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CL_ClearState = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\x1D\x00\x00\x00\x00"), "xxxx?xxxx?xxxx?xxxx????xxx????");
		p_CL_EndMovie = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x48\x83\xEC\x68\x80\x3D\x00\x00\x00\x00\x00"), "xxxxxxxxx?????");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CL_ClearState = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\x0D\x00\x00\x00\x00\x48\x8B\x01"), "xxxx?xxxx?xxxx????xxx????xxx");
		p_CL_EndMovie = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x28\x80\x3D\x00\x00\x00\x00\x00\x74\x7B"), "xxxxxx?????xx");
#endif
		CL_ClearState = p_CL_ClearState.RCast<int(*)(void)>(); /*48 89 5C 24 ?? 48 89 74 24 ?? 57 48 81 EC ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 48 8B 01*/
		CL_EndMovie = p_CL_EndMovie.RCast<int(*)(void)>();     /*48 83 EC 28 80 3D ?? ?? ?? ?? ?? 74 7B*/
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HCL_Main);
