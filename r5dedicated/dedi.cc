#include "pch.h"
#include "hooks.h"

#include <mutex>

/*
// only few people know from where this came from lmfao
char __fastcall sub_1802AAD00(__int64 a1, _QWORD *a2)
{
  __int64 v4; // rcx
  __int64 v6; // [rsp+48h] [rbp+10h]

  byte_18243B249 = (*(__int64 (__fastcall **)(__int64))(*(_QWORD *)qword_18243B298 + 80i64))(qword_18243B298);
  (*(void (__fastcall **)(__int64, char **, __int64))(*(_QWORD *)qword_18242F9F0 + 808i64))(
    qword_18242F9F0,
    &off_1807F4240,
    10i64);
  dword_180821448 = 0;
  qword_182B99280 = (char *)a2[1];
  qword_182B99288 = (void *)sub_1802A8A90(a2[2]);
  qword_182B99290 = (void *)a2[3];
  sub_1802B42E0(v4, "COM_InitFilesystem( info.m_pInitialMod )", "COM_ShutdownFileSystem()");
  sub_180249400(a2[2]);
  (*(void (__fastcall **)(__int64))(*(_QWORD *)qword_18243B5C8 + 168i64))(qword_18243B5C8);
  sub_1800F3270();
  qword_18242CAF8 = (*(__int64 (__fastcall **)(__int64))(*(_QWORD *)qword_18243B5C8 + 248i64))(qword_18243B5C8);
  if ( !qword_18242CAF8 )
    sub_1802A54A0("Could not get the material system config record!");
  if ( !(unsigned __int8)sub_1802B2690(0i64) )
    return 0;
  v6 = operator new(208i64);
  sub_18032CBA0(v6, a2[4]);
  *(_QWORD *)v6 = &CModAppSystemGroup::`vftable';
  *(_BYTE *)(v6 + 200) = 1;
  *(_QWORD *)(a1 + 8) = v6;
  qword_18243B0C0 = (__int64)sub_18032E6E0;
  sub_18032E710(*(_QWORD *)(a1 + 8));
  sub_1802642C0();
  return 1;
}
*/

/*
char __fastcall LLIye(__int64 a1, _QWORD *a2)
{
//   __int64 v4; // rcx
//   __int64 v6; // [rsp+48h] [rbp+10h]

//   byte_18243B249 = (*(__int64 (__fastcall **)(__int64))(*(_QWORD *)qword_18243B298 + 80i64))(qword_18243B298);
//   (*(void (__fastcall **)(__int64, char **, __int64))(*(_QWORD *)qword_18242F9F0 + 808i64))(
//     qword_18242F9F0,
//     &off_1807F4240,
//     10i64);
//   dword_180821448 = 0;
//   qword_182B99280 = (char *)a2[1];
//   qword_182B99288 = (void *)sub_1802A8A90(a2[2]);
//   qword_182B99290 = (void *)a2[3];
    sub_14029A1D0(v13, "COM_InitFilesystem( m_StartupInfo.m_szInitialMod )", "COM_ShutdownFileSystem()"); // string ref, nuff said
	sub_140210F70(a1 + 292);
//   (*(void (__fastcall **)(__int64))(*(_QWORD *)qword_18243B5C8 + 168i64))(qword_18243B5C8);
//   sub_1800F3270();
//   qword_18242CAF8 = (*(__int64 (__fastcall **)(__int64))(*(_QWORD *)qword_18243B5C8 + 248i64))(qword_18243B5C8); // &materials->GetCurrentConfigForVideoCard();
//   if ( !qword_18242CAF8 )
//     sub_1802A54A0("Could not get the material system config record!");
//   if ( !(unsigned __int8)sub_1802B2690(0i64) )
//     return 0;
//   v6 = operator new(208i64);
//   sub_18032CBA0(v6, a2[4]);
//   *(_QWORD *)v6 = &CModAppSystemGroup::`vftable';
//   *(_BYTE *)(v6 + 200) = 1;
//   *(_QWORD *)(a1 + 8) = v6;
//   qword_18243B0C0 = (__int64)sub_18032E6E0;
//   sub_18032E710(*(_QWORD *)(a1 + 8));
//   sub_1802642C0();
  return 1;
}
*/

/*
Notes:
patch je2jmp in mode based init bs

patch Shader_Connect to `mov al, 1; ret`

patch GUI_related_rpak_handlers_and_shit to 0xC3?
 * patch sub_1404084E0
 * patch sub_140415190
or instead patch sub_1404066E0 (graphics madness)
On the same note - sub_1403DF870 looks beyond broken because of mode 2, i feel like that's not how it works... it calls that madness...

sub_1403C66D0 in matsys demands a buffer... first one makes it, second one adds a ref; ret_t = void

in StudioRender sub_140456780@000000014045681A call to a MatSys handler init bs, breaks mode2 in half and causes almost every problem...

14B37C3C0 - CHLClient, can be 0?
162C61208 - sv_IsDedicated
141741BA0 - engine instance
 * 5th vfunc is run frame?
*/

auto p_is_dedicated = (uint8_t*)0x162C61208;


// This shit inits everything, I set sv_IsDedicated to true somewhere around here...
PVOID sub_14044AFA0 = 0;
char __fastcall sub_14044AFA0_hk(__int64 a1) {
	*p_is_dedicated = 1; // HAS TO BE HERE!!!

	return reinterpret_cast<decltype(&sub_14044AFA0_hk)>(sub_14044AFA0)(a1);
}

/*
PVOID CModAppSystemGroup__Main = 0;
__int64 __fastcall CModAppSystemGroup__Main_hk(__int64 a1, __int64 a2) {

	return reinterpret_cast<decltype(&CModAppSystemGroup__Main_hk)>(CModAppSystemGroup__Main)(a1, a2);
}
*/

struct DedicatedExportsVtbl {
	char pad[0x40];
	void* Sys_Printf;
	void* RunFrame;
} devtbl;

static_assert(offsetof(DedicatedExportsVtbl, RunFrame) == 0x48);
static_assert(offsetof(DedicatedExportsVtbl, Sys_Printf) == 0x40);

struct {
	DedicatedExportsVtbl* vtbl = &devtbl;
} DedicatedExports;

void DERunFrame() {
	auto engine = (uintptr_t**)0x141741BA0;
	using frame_t = void(__fastcall*)(uintptr_t**);

	// Yes, trust me it's less broken this way...
	// Nvm, this game is more stable smh
	//*(uint8_t*)0x14171A9B4 = 1;

	using void_f = void(__fastcall*)();
	//void_f(0x14095A140)(); // init miles???
	//void_f(0x140456780)(); // Precache mat shit...

	//using vpk_load_f = void(__fastcall*)(const char*, const char*, char);
	//vpk_load_f(0x140341700)("vpk/client_mp_common.bsp", 0, 0);

	//void_f(0x140297D30)();

	spdlog::info("Reached DERunFrame yay!");
	puts("DERF");

	auto frame = frame_t((*engine)[5]);
	while (1) {
		frame(engine);
		Sleep(1000 / 80);
	}
}

void __fastcall DEPrintf(__int64 thisptr, char* text) {
	printf("%s", text);
}

/*
PVOID sub_140345130 = 0;
__int64 __fastcall sub_140345130_hk(__int64 a1) {

	return reinterpret_cast<decltype(&sub_14044AFA0_hk)>(sub_14044AFA0)(a1);
} // */

// Create window...
PVOID sub_140299100 = 0;
char __fastcall sub_140299100_hk(int* a1) {
	using void_f = void(__fastcall*)();
	void_f(0x140297D30)();

	return 1;
}

void Hooks::DedicatedPatch() {
	// for future reference 14171A9B4 - matsys mode

	*p_is_dedicated = 1;

	*(uintptr_t*)0x14D415040 = 0x1417304E8;

	*(void**)0x14C119C10 = &DedicatedExports;

	*(uintptr_t*)0x14B37C3C0 = 0x141F10CA0;

	devtbl.RunFrame = &DERunFrame;
	devtbl.Sys_Printf = &DEPrintf;

	// Hooks first
	{
		// binder for the app group - 2 reset dedicated to 1
		auto addr_AppGroupBind = PVOID(0x14044AFA0);
		MH_CreateHook(addr_AppGroupBind, &sub_14044AFA0_hk, reinterpret_cast<void**>(&sub_14044AFA0));
		MH_EnableHook(addr_AppGroupBind);
	}
	/*
	{
		// group main, I don't remember why I needed this?
		auto addr_AppGroupMain = PVOID(0x0);
		MH_CreateHook(addr_AppGroupMain, &CModAppSystemGroup__Main_hk, reinterpret_cast<void**>(&CModAppSystemGroup__Main));
		MH_EnableHook(addr_AppGroupMain);
	}
	*/

	{
		// begin
		MemoryAddress(0x00000001404093F0).Patch({ 0xC3, 0x90, 0x90, 0x90, 0x90 });
		// end
		MemoryAddress(0x00000001403D2E60).Patch({ 0xC3, 0x90, 0x90, 0x90, 0x90, 0x90 });
	}

	{
		MemoryAddress(0x0000000140282E40).Patch({ 0x48, 0x33, 0xC0, 0xC3, 0x90, 0x90, 0x90 });
	}

	{
		// Make dedi exports run anyway
		MemoryAddress(0x140345160).Patch({ 0x90, 0x90 });
	}

	{
		// write 1 to dedi, get rid of hook
		//MemoryAddress(0x000000014030C607).Patch({ 0xC6, 0x05, 0xFA, 0x4B, 0x95, 0x22, 0x01 });
	}

	// this is bs, TFO:R had same shit, but setting 1 and returning from some func worked cuz we had dedicated and were doing init ourselves lmfao...
	{
		//init_matshit_mode_based
		MemoryAddress(0x00000001403BD142).Patch({ 0xEB });
	}

	//return;

	/*
	{
		// Shader_Connect related things
		MemoryAddress(0x0000000140342BA0).Patch({ 0xB0, 0x01, 0xC3, 0x90 }); // with pad
	}

	{
		// Graphics Madness (C)
		MemoryAddress(0x00000001404066E0).Patch({ 0xC3, 0x90 });
	}

	{
		// StudioRender::Init
		//MemoryAddress(0x0000000140456780).Patch({ 0xC3, 0x90, 0x90, 0x90 });

		// Avi::Init
		MemoryAddress(0x00000001402ABD10).Patch({ 0xC3, 0x90, 0x90, 0x90 });

		// Bik::Init
		MemoryAddress(0x00000001402B0930).Patch({ 0xC3, 0x90, 0x90, 0x90, 0x90 });

		// CEngineApi::Init
		MemoryAddress(0x0000000140342FB0).Patch({ 0xC3, 0x90, 0x90, 0x90, 0x90 });

		// CMatSysSur::Init
		MemoryAddress(0x000000014053DCC0).Patch({ 0xC3, 0x90, 0x90, 0x90, 0x90 });
	}
	*/

	{
		// Skip create window and stuff, game is only aware of CL since RSPN didn't do enough #ifdef
		//MemoryAddress(0x0000000140344225).Patch({ 0xE9, 0x19, 0x02, 0x00, 0x00 });

		auto addr_sub_140299100 = PVOID(0x140299100);
		//MH_CreateHook(addr_sub_140299100, &sub_140299100_hk, reinterpret_cast<void**>(&sub_140299100));
		//MH_EnableHook(addr_sub_140299100);
	}

	{
		// Make dedicated in mod info 1, and jump cuz we replaced cmp
		//MemoryAddress(0x0000000140344D01).Patch({ 0x41, 0xC6, 0x87, 0xA8, 0x00, 0x00, 0x00, 0x01, 0xEB });
	}

	return;

	{
		// this move broke hearts of many
		MemoryAddress(0x0000000140236D33).Patch({ 0x90, 0x90, 0x90, 0x90 });
	}

	{
		// Return in ui mat stuff
		MemoryAddress(0x00000001402F4740).Patch({ 0xC3, 0x90, 0x90, 0x90, 0x90 });

		// Return in settings stuff
		MemoryAddress(0x0000000140FB2F10).Patch({ 0xC3, 0x90 });
	}

	{
		// Calls some HLClient func, we don't have that obv
		MemoryAddress(0x000000014023658E).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });
	}

	{
		// Honestly, don't remember much
		MemoryAddress(0x0000000140282C40).Patch({ 0xC3, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });
	}

	{
		// Shader bs
		MemoryAddress(0x0000000140417B30).Patch({ 0x33, 0xC0, 0xC3 });
	}

	// Day 2 of madness

	{
		//whaddefuq
		//MemoryAddress(0x000000014031CC40).Patch({ 0xC3, 0x90, 0x90, 0x90 }); // fixed by convar

		//hb thing
		MemoryAddress(0x00000001402312A0).Patch({ 0xC3, 0x90, 0x90, 0x90 });

		// HLClient FFS
		// first is interpolate
		// second is delayPostSnapshotNotificationsToAfterInterpolation
		MemoryAddress(0x00000001402327B1).Patch({
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90
		});
	}

	// Day 3 of LLIye
	{
		// nop this lmfao
		MemoryAddress(0x0000000140456903).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });
	}

	{
		// something with gamemode
		MemoryAddress(0x000000014022A4F8).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });
	}

	{
		// gameui shit
		MemoryAddress(0x0000000140283131).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });
	}

	{
		// hl client call near some bsp
		MemoryAddress(0x0000000140238DF0).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });
	}

	// ---
	// 1403DFC30 = 0x94490 // an expensive stuff that wasted many CPU cycles, this one seems to be the best candidate to return

}