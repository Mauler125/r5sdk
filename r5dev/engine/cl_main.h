#pragma once

//-------------------------------------------------------------------------
// RUNTIME: CL_CLEARSTATE
//-------------------------------------------------------------------------
#if defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
inline CMemory CL_ClearState = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\x0D\x00\x00\x00\x00\x48\x8B\x01"), "xxxx?xxxx?xxxx????xxx????xxx");
#endif // 0x1402BE4C0 // 48 89 5C 24 ? 48 89 74 24 ? 57 48 81 EC ? ? ? ? 48 8B 0D ? ? ? ? 48 8B 01 //

///////////////////////////////////////////////////////////////////////////////
class HCL_Main : public IDetour
{
	virtual void GetAdr(void) const
	{
#if defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		std::cout << "| FUN: CL_ClearState                        : 0x" << std::hex << std::uppercase << CL_ClearState.GetPtr() << std::setw(nPad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
#endif // GAMEDLL_S2 || GAMEDLL_S3
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HCL_Main);
