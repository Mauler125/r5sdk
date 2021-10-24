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
	MemoryAddress gCShaderGlue__Init = 0x00000001403B3A50; //
	MemoryAddress gMatSync = 0x00000001403DEE90; //
	MemoryAddress gCMaterialSystem__MatsysMode_Init = 0x00000001403BD120; //
	MemoryAddress r4 = 0x0000000140404380; //
	MemoryAddress r5 = 0x000000014040D850; //
	MemoryAddress r6 = 0x0000000140413260; //
	MemoryAddress r7 = 0x00000001404093F0; //
	MemoryAddress r8 = 0x00000001403D2E60; //
	MemoryAddress d3d11init = 0x000000014043CDF0; //

	// Engine
	MemoryAddress gHost_Init_0 = 0x0000000140236E40; // main Host_Init()?
	MemoryAddress e1 = 0x0000000140FB2F10; // also used by CServerGameDLL
	MemoryAddress addr_CEngine_Frame = 0x00000001402970E0;
	MemoryAddress e3 = 0x0000000140231C00;
	MemoryAddress e4 = 0x0000000140BE1970;
	MemoryAddress e5 = 0x0000000140DBBAF0;
	MemoryAddress e6 = 0x0000000140DBE610;
	MemoryAddress e7 = 0x000000014044AFA0;
	MemoryAddress e8 = 0x000000014027EC50; // RenderFrame?
	MemoryAddress gCEngineAPI__Init = 0x0000000140342FB0; //
	MemoryAddress gCEngineAPI__ModInit = 0x0000000140343DE0; //
	MemoryAddress gCEngineAPI__Connect = 0x0000000140342BA0; //
	MemoryAddress gCEngineAPI__OnStartup = 0x0000000140343860; //
	MemoryAddress gCSourceAppSystemGroup__Create = 0x000000014044AFA0; //
	MemoryAddress gCShaderSystem__Init = 0x00000001403DF870; //
	MemoryAddress gInitMaterialSystem = 0x000000014024B390; //
	MemoryAddress gCVideoMode_Common__DrawStartupGraphic = 0x000000014027F0F0; //
	MemoryAddress gShaderDispatch = 0x00000001403EE5C0;
	MemoryAddress gShaderCreate = 0x00000001403ECD00; //
	MemoryAddress gTextureCreate = 0x00000001403EDCD0;

	MemoryAddress gCShaderSystem__9 = 0x00000001403DFC30;
	MemoryAddress gBSP_LUMP_INIT = 0x00000001402546F0; // BSP.


	MemoryAddress e9 = 0x00000001404066E0;
	MemoryAddress e10 = 0x00000001403B49E0; // CMaterialGlue?

	// SERVER
	MemoryAddress gHost_Init_1 = 0x0000000140237B00; // server Host_Init()?
	MemoryAddress s1 = 0x0000000140231C00; // _Host_RunFrame() with inlined CFrameTimer::MarkFrame()?
	MemoryAddress s2 = 0x00000001402312A0; // server HeartBeat? (baseserver.cpp)
	MemoryAddress s3 = 0x0000000140FB36D0; // TEMP??

	// CLIENT
	MemoryAddress gHost_Init_2 = 0x0000000140236640; // client Host_Init()?
	MemoryAddress gCGame__CreateGameWindow = 0x0000000140299100; //
	MemoryAddress c2 = 0x00000001403F4360; // 1403DF870 --> 1403F4360
	MemoryAddress c3 = 0x00000001403F8A80; // 1403DF870 --> 1403F8A40
	MemoryAddress gCHLClient__1000 = 0x00000001405C27B0; // CHLClient + 1000
	MemoryAddress gCHLClient__HudMessage = 0x00000001405BAC00; // CHudMessage
	MemoryAddress c6 = 0x00000001403CA2D0; //

	// VGUI
	MemoryAddress gCEngineVGui__Init = 0x0000000140282E40; // jumptable
	MemoryAddress gCEngineVGui__OnLevelLoadingStarted = 0x00000001402830D0;
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