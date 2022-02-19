#pragma once

///////////////////////////////////////////////////////////////////////////////

namespace
{
	/* ==== SV_MAIN ======================================================================================================================================================= */
	ADDRESS p_SV_ShutdownGameDLL = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x83\xEC\x28\x80\x3D\x00\x00\x00\x00\x00\x0F\x84\x00\x00\x00\x00\x48\x8B\x0D\x00\x00\x00\x00\x48\x89\x5C\x24\x00", "xxxxxx?????xx????xxx????xxxx?");
	void (*SV_ShutdownGameDLL)() = (void(*)())p_SV_ShutdownGameDLL.GetPtr(); /*48 83 EC 28 80 3D ? ? ? ? ? 0F 84 ? ? ? ? 48 8B 0D ? ? ? ? 48 89 5C 24 ?*/

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	ADDRESS CGameServer__SpawnServer = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x53\x55\x56\x57\x41\x55\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00", "xxxxxxxxxxxxxx????");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	ADDRESS CGameServer__SpawnServer = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x8B\xC4\x53\x55\x56\x57\x41\x54\x41\x55\x41\x57", "xxxxxxxxxxxxx");
	// 0x140312D80 // 48 8B C4 53 55 56 57 41 54 41 55 41 57 //
#endif
}

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
class HSV_Main : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: SV_ShutdownGameDLL                   : 0x" << std::hex << std::uppercase << p_SV_ShutdownGameDLL.GetPtr()     << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: CGameServer::SpawnServer             : 0x" << std::hex << std::uppercase << CGameServer__SpawnServer.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HSV_Main);
