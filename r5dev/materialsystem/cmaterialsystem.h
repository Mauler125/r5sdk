#pragma once

namespace
{
	/* ==== MATERIALSYSTEM ================================================================================================================================================== */
	ADDRESS CMaterialSystem__Init = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x83\xEC\x70\x48\x83\x3D\x00\x00\x00\x00\x00"), "xxxx?xxxxxxxxxxxxxxxxxx?????");
	// 0x1403BBFD0 // 48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 83 EC 70 48 83 3D ? ? ? ? ? //

	ADDRESS p_DrawStreamOverlay = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x41\x56\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x2B\xE0\xC6\x02\x00"), "xxx????x????xxxxxx"); // 41 56 B8 ? ? ? ? E8 ? ? ? ? 48 2B E0 C6 02 00 //
	const char* (*DrawStreamOverlay)(void* thisptr, std::uint8_t* a2, void* unused, void* a4) = (const char* (*)(void*, std::uint8_t*, void*, void*))p_DrawStreamOverlay.GetPtr();

#ifndef DEDICATED
	ADDRESS p_StreamDB_Init = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x41\x54\x41\x56\x41\x57\x48\x83\xEC\x40\x48\x8B\xE9"), "xxxx?xxxx?xxxx?xxxx?xxxxxxxxxxxxx");
	void (*StreamDB_Init)(const char* pszStreamDbFile) = (void (*)(const char* pszStreamDbFile))p_StreamDB_Init.GetPtr(); /*48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 54 41 56 41 57 48 83 EC 40 48 8B E9*/
#endif // !DEDICATED

	void* g_pMaterialSystem = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\x0D\x00\x00\x00\x00\x48\x85\xC9\x74\x11\x48\x8B\x01\x48\x8D\x15\x00\x00\x00\x00"), "xxx????xxxxxxxxxxx????").ResolveRelativeAddressSelf(0x3, 0x7).RCast<void*>();

	int* total_streaming_tex_memory = p_DrawStreamOverlay.Offset(0x0).FindPatternSelf("48 8B 05", ADDRESS::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).RCast<int*>();
	int* unfree_streaming_tex_memory = p_DrawStreamOverlay.Offset(0x20).FindPatternSelf("48 8B 05", ADDRESS::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).RCast<int*>();
	int* unusable_streaming_tex_memory = p_DrawStreamOverlay.Offset(0x50).FindPatternSelf("48 8B 05", ADDRESS::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).RCast<int*>();

}

void CMaterialSystem_Attach();
void CMaterialSystem_Detach();
///////////////////////////////////////////////////////////////////////////////
class HMaterialSystem : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: CMaterialSystem::Init                : 0x" << std::hex << std::uppercase << CMaterialSystem__Init.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: DrawStreamOverlay                    : 0x" << std::hex << std::uppercase << p_DrawStreamOverlay.GetPtr()   << std::setw(npad) << " |" << std::endl;
#ifndef DEDICATED
		std::cout << "| FUN: StreamDB_Init                        : 0x" << std::hex << std::uppercase << p_StreamDB_Init.GetPtr()       << std::setw(npad) << " |" << std::endl;
#endif // !DEDICATED
		std::cout << "| VAR: g_pMaterialSystem                    : 0x" << std::hex << std::uppercase << g_pMaterialSystem              << std::setw(0)    << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HMaterialSystem);
