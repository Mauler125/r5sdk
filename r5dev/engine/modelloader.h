#pragma once

namespace
{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	ADDRESS CModelLoader__FindModel = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x55\x41\x55\x41\x56\x48\x8D\xAC\x24\x00\x00\x00\x00", "xxxxxxxxxx????");
	// 0x1402A1F10 // 40 55 41 55 41 56 48 8D AC 24 ? ? ? ? //

	ADDRESS CModelLoader__LoadModel = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x53\x57\x41\x56\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xFA", "xxxxxxxx????xxx");
	// 0x1402A23B0 // 40 53 57 41 56 48 81 EC ? ? ? ? 48 8B FA //

	ADDRESS CModelLoader__Studio_LoadModel = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x00\x55\x56\x57\x41\x54\x41\x56\x48\x8D\xAC\x24\x00\x00\x00\x00", "xxxx?xxxxxxxxxxx????");
	// 0x140252F10 // 48 89 5C 24 ? 55 56 57 41 54 41 57 48 81 EC ? ? ? ? //
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	ADDRESS CModelLoader__FindModel = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x55\x41\x57\x48\x83\xEC\x48\x80\x3A\x2A", "xxxxxxxxxxx");
	// 0x140253530 // 40 55 41 57 48 83 EC 48 80 3A 2A //

	ADDRESS CModelLoader__LoadModel = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x53\x57\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\x05\x00\x00\x00\x00", "xxxxxxxx????xxx????");
	// 0x140253810 // 40 53 57 41 57 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? //

	ADDRESS CModelLoader__Studio_LoadModel = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x00\x55\x56\x57\x41\x54\x41\x57\x48\x81\xEC\x00\x00\x00\x00", "xxxx?xxxxxxxxxx????");
	// 0x140252F10 // 48 89 5C 24 ? 55 56 57 41 54 41 57 48 81 EC ? ? ? ? //
#endif
}

///////////////////////////////////////////////////////////////////////////////
class HModelLoader : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: CModelLoader::FindModel              : 0x" << std::hex << std::uppercase << CModelLoader__FindModel.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: CModelLoader::LoadModel              : 0x" << std::hex << std::uppercase << CModelLoader__LoadModel.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: CModelLoader::Studio_LoadModel       : 0x" << std::hex << std::uppercase << CModelLoader__Studio_LoadModel.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HModelLoader);
