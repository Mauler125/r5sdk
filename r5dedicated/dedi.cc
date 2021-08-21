#include "pch.h"
#include "hooks.h"

#include <mutex>

auto p_is_dedicated = (uint8_t*)0x162C61208;


// This shit inits everything, I set sv_IsDedicated to true somewhere around here...
PVOID sub_14044AFA0 = 0;
char __fastcall sub_14044AFA0_hk(__int64 a1) {
	*p_is_dedicated = 1; // HAS TO BE HERE!!!

	return reinterpret_cast<decltype(&sub_14044AFA0_hk)>(sub_14044AFA0)(a1);
}

void NukeRender() {
	// Day 4

	{
		// something calls unmap map shit
		MemoryAddress(0x00000001402FE280).Patch({ 0xC3 });
	}
	{
		// set shader stuff
		MemoryAddress(0x00000001403B3A50).Patch({ 0xC3 });
	}
	{
		// no comment
		MemoryAddress(0x00000001403DEE90).Patch({ 0xC3 });
	}
	{
		// Clear stuff rendering
		MemoryAddress(0x0000000140404380).Patch({ 0xC3 });
	}
	{
		// heavy render stuff...
		MemoryAddress(0x000000014040D850).Patch({ 0xC3 });
	}
	{
		// with set shader resource
		MemoryAddress(0x0000000140413260).Patch({ 0xC3 });
	}

	{
		// HLClient call inside eng->frame
		MemoryAddress(0x00000001402974F0).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });
	}
}

void Hooks::DedicatedPatch() {
	// for future reference 14171A9B4 - matsys mode

	*p_is_dedicated = 1;

	*(uintptr_t*)0x14D415040 = 0x1417304E8;

	*(uintptr_t*)0x14B37C3C0 = 0x141F10CA0;


	NukeRender();

	// Hooks first
	{
		// binder for the app group - 2 reset dedicated to 1
		auto addr_AppGroupBind = PVOID(0x14044AFA0);
		MH_CreateHook(addr_AppGroupBind, &sub_14044AFA0_hk, reinterpret_cast<void**>(&sub_14044AFA0));
		MH_EnableHook(addr_AppGroupBind);
	}

	{
		// begin
		MemoryAddress(0x00000001404093F0).Patch({ 0xC3, 0x90, 0x90, 0x90, 0x90 });
		// end
		MemoryAddress(0x00000001403D2E60).Patch({ 0xC3, 0x90, 0x90, 0x90, 0x90, 0x90 });
	}

	{
		MemoryAddress(0x0000000140282E40).Patch({ 0x48, 0x33, 0xC0, 0xC3, 0x90, 0x90, 0x90 });
	}

	// this is bs, TFO:R had same shit, but setting 1 and returning from some func worked cuz we had dedicated and were doing init ourselves lmfao...
	{
		//init_matshit_mode_based
		MemoryAddress(0x00000001403BD142).Patch({ 0xEB });
	}

	// ---
	// ??? 1403DFC30 = 0x94490 ??? // an expensive stuff that wasted many CPU cycles, this one seems to be the best candidate to return

}