#pragma once

void InstallOpcodes();
inline HANDLE GameProcess = GetCurrentProcess();

namespace
{
	Module r5_op = Module("r5apex.exe"); // Create module class instance.

#pragma region Origin
	/*0x14032EEA0*/
	MemoryAddress Origin_Init = r5_op.PatternSearch("48 83 EC 28 80 3D ? ? ? 23 ? 0F 85 ? 02 ?");

	/*0x140330290*/
	MemoryAddress Origin_SetState = r5_op.PatternSearch("48 81 EC 58 04 ? ? 80 3D ? ? ? ? ? 0F 84");
#pragma endregion

#pragma region Engine
	/*0x14043FB90*/
	MemoryAddress dst002 = r5_op.PatternSearch("48 89 4C 24 08 56 41 55 48 81 EC 68 03 ? ? 4C");

	/*0x14022A4A0*/
	MemoryAddress dst004 = r5_op.PatternSearch("48 83 EC 38 0F 29 74 24 20 48 89 5C 24 40 48 8B");

	/*0x140238DA0*/
	MemoryAddress Host_NewGame = r5_op.PatternSearch("48 8B C4 ? 41 54 41 ? 48 81 EC ? ? ? ? F2");
#pragma endregion

#pragma region NetChannel
	/*0x14030D000*/
	MemoryAddress CServer_Auth = r5_op.PatternSearch("40 55 57 41 55 41 57 48 8D AC 24 ? ? ? ?");
#pragma endregion

#pragma region FairFight
	/*0x140303AE0*/
	MemoryAddress FairFight_Init = r5_op.PatternSearch("40 53 48 83 EC 20 8B 81 B0 03 ? ? 48 8B D9 C6");
#pragma endregion

	void PrintOAddress() // Test the sigscan results
	{
		std::cout << "+--------------------------------------------------------+" << std::endl;
		PRINT_ADDRESS("Origin_Init", Origin_Init.GetPtr());
		PRINT_ADDRESS("Origin_SetState", Origin_SetState.GetPtr());
		PRINT_ADDRESS("dst002", dst002.GetPtr());
		PRINT_ADDRESS("dst004", dst004.GetPtr());
		PRINT_ADDRESS("Host_NewGame", Host_NewGame.GetPtr());
		PRINT_ADDRESS("CServer_Auth", CServer_Auth.GetPtr());
		PRINT_ADDRESS("FairFight_Init", FairFight_Init.GetPtr());
		std::cout << "+--------------------------------------------------------+" << std::endl;

		// TODO implement error handling when sigscan fails or result is 0
	}
}