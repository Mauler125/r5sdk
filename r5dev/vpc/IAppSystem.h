#pragma once

/* ==== IAPPSYSTEM ============================================================================================================================================== */
inline ADDRESS p_IAppSystem_LoadLibrary = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x28\x48\x8B\x0D\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x85\xC9\x74\x11"), "xxxxxxx????xxx????xxx????xxxxx");
inline void* (*IAppSystem_LoadLibrary)() = (void * (*)())p_IAppSystem_LoadLibrary.GetPtr(); /*48 83 EC 28 48 8B 0D ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 48 89 05 ?? ?? ?? ?? 48 85 C9 74 11*/ // C initializers/terminators

///////////////////////////////////////////////////////////////////////////////
class HAppSystem : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: IAppSystem::LoadLibrary              : 0x" << std::hex << std::uppercase << p_IAppSystem_LoadLibrary.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HAppSystem);
