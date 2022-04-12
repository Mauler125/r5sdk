#pragma once

///////////////////////////////////////////////////////////////////////////////

/* ==== SV_MAIN ======================================================================================================================================================= */
inline CMemory p_SV_InitGameDLL = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x81\xEC\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x80\x3D\x00\x00\x00\x00\x00\x0F\x85\x00\x00\x00\x00"), "xxx????x????xx?????xx????");
inline auto SV_InitGameDLL = p_SV_InitGameDLL.RCast<void(*)(void)>(); /*48 81 EC ? ? ? ? E8 ? ? ? ? 80 3D ? ? ? ? ? 0F 85 ? ? ? ?*/

inline CMemory p_SV_ShutdownGameDLL = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x28\x80\x3D\x00\x00\x00\x00\x00\x0F\x84\x00\x00\x00\x00\x48\x8B\x0D\x00\x00\x00\x00\x48\x89\x5C\x24\x00"), "xxxxxx?????xx????xxx????xxxx?");
inline auto SV_ShutdownGameDLL = p_SV_ShutdownGameDLL.RCast<void(*)(void)>(); /*48 83 EC 28 80 3D ? ? ? ? ? 0F 84 ? ? ? ? 48 8B 0D ? ? ? ? 48 89 5C 24 ?*/

inline CMemory p_SV_CreateBaseline = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x28\x48\x8B\x0D\x00\x00\x00\x00\x48\x85\xC9\x75\x07"), "xxxxxxx????xxxxx");
inline auto SV_CreateBaseline = p_SV_CreateBaseline.RCast<bool(*)(void)>(); /*48 83 EC 28 48 8B 0D ? ? ? ? 48 85 C9 75 07*/

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
inline CMemory CGameServer__SpawnServer = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x55\x56\x57\x41\x55\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00"), "xxxxxxxxxxxxxx????");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
inline CMemory CGameServer__SpawnServer = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x53\x55\x56\x57\x41\x54\x41\x55\x41\x57"), "xxxxxxxxxxxxx");
	// 0x140312D80 // 48 8B C4 53 55 56 57 41 54 41 55 41 57 //
#endif

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
class HSV_Main : public IDetour
{
	virtual void GetAdr(void) const
	{
		std::cout << "| FUN: SV_InitGameDLL                       : 0x" << std::hex << std::uppercase << p_SV_ShutdownGameDLL.GetPtr()     << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: SV_ShutdownGameDLL                   : 0x" << std::hex << std::uppercase << p_SV_ShutdownGameDLL.GetPtr()     << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: SV_CreateBaseline                    : 0x" << std::hex << std::uppercase << p_SV_CreateBaseline.GetPtr()      << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: CGameServer::SpawnServer             : 0x" << std::hex << std::uppercase << CGameServer__SpawnServer.GetPtr() << std::setw(nPad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HSV_Main);
