#pragma once

inline HANDLE GameProcess = GetCurrentProcess();
void SetCHostState();

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


	// TODO: create patterns instead and rename to function names.
	// Renderer
	MemoryAddress r0 = 0x00000001402FE280; //
	MemoryAddress r1 = 0x00000001403B3A50; //
	MemoryAddress r2 = 0x00000001403DEE90; //
	MemoryAddress r3 = 0x00000001403BD120; //
	MemoryAddress r4 = 0x0000000140404380; //
	MemoryAddress r5 = 0x000000014040D850; //
	MemoryAddress r6 = 0x0000000140413260; //
	MemoryAddress r7 = 0x00000001404093F0; //
	MemoryAddress r8 = 0x00000001403D2E60; //
	MemoryAddress d3d11init = 0x000000014043CDF0; //

	// Engine
	MemoryAddress e0 = 0x0000000140236E40; // main Host_Init()?
	MemoryAddress e1 = 0x0000000140FB2F10; // also used by CServerGameDLL
	MemoryAddress addr_CEngine_Frame = 0x00000001402970E0;

	// SERVER
	MemoryAddress s0 = 0x0000000140237B00; // server Host_Init()?
	MemoryAddress s1 = 0x0000000140231C00; // _Host_RunFrame() with inlined CFrameTimer::MarkFrame()?
	MemoryAddress s2 = 0x00000001402312A0; // server HeartBeat? (baseserver.cpp)
	MemoryAddress s3 = 0x0000000140FB36D0; // TEMP??

	// CLIENT
	MemoryAddress c0 = 0x0000000140236640; // client Host_Init()?
	MemoryAddress c1 = 0x0000000140299100; // CreateGameWindowInit()?
	MemoryAddress c2 = 0x00000001403F4360; // 1403DF870 --> 1403F4360
	MemoryAddress c3 = 0x00000001403F8A80; // 1403DF870 --> 1403F8A40
	MemoryAddress c4 = 0x00000001405C27B0; // CHLClient + 1000
	MemoryAddress c5 = 0x00000001405BAC00; //
	MemoryAddress c6 = 0x00000001403CA2D0; //
	MemoryAddress c7 = 0x00000001403CC750; // lightmaps?
	MemoryAddress CreateGameWindow = 0x0000000140343DE0;

	// VGUI
	MemoryAddress v0 = 0x0000000140282E40; // jumptable
	MemoryAddress OnLevelLoadingStarted = 0x00000001402830D0;
	MemoryAddress SCR_BeginLoadingPlaque = 0x000000014023E870;


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