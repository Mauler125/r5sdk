#pragma once
#include <engine/server/sv_main.h>
#include <vguimatsurface/MatSystemSurface.h>

enum class PaintMode_t
{
	PAINT_UIPANELS     = (1 << 0),
	PAINT_INGAMEPANELS = (1 << 1),
};

class CEngineVGui
{
public:
	static int Paint(CEngineVGui* thisptr, PaintMode_t mode);
	void EnabledProgressBarForNextLoad(void)
	{
		int index = 31;
		CallVFunc<void>(index, this);
	}
	void ShowErrorMessage(void)
	{
		int index = 35;
		CallVFunc<void>(index, this);
	}
	void HideLoadingPlaque(void)
	{
		int index = 36;
		CallVFunc<void>(index, this);
	}
};

/* ==== CENGINEVGUI ===================================================================================================================================================== */
inline CMemory p_CEngineVGui_Paint;
inline auto CEngineVGui_Paint = p_CEngineVGui_Paint.RCast<int (*)(CEngineVGui* thisptr, PaintMode_t mode)>();

inline CMemory p_CEngineVGui_RenderStart;
inline auto CEngineVGui_RenderStart = p_CEngineVGui_RenderStart.RCast<void* (*)(CMatSystemSurface* pMatSystemSurface)>();

inline CMemory p_CEngineVGui_RenderEnd;
inline auto CEngineVGui_RenderEnd = p_CEngineVGui_RenderEnd.RCast<void* (*)(void)>();

inline CEngineVGui* g_pEngineVGui = nullptr;

///////////////////////////////////////////////////////////////////////////////
class VEngineVGui : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: CEngineVGui::Paint                   : {:#18x} |\n", p_CEngineVGui_Paint.GetPtr());
		spdlog::debug("| FUN: CEngineVGui::RenderStart             : {:#18x} |\n", p_CEngineVGui_RenderStart.GetPtr());
		spdlog::debug("| FUN: CEngineVGui::RenderEnd               : {:#18x} |\n", p_CEngineVGui_RenderEnd.GetPtr());
		spdlog::debug("| VAR: g_pEngineVGui                        : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pEngineVGui));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CEngineVGui_Paint = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x89\x54\x24\x10\x55\x56\x41\x55\x48\x81\xEC\x00\x00\x00\x00"), "xxxxxxxxxxx????");
		CEngineVGui_Paint = p_CEngineVGui_Paint.RCast<int (*)(CEngineVGui* thisptr, PaintMode_t mode)>(); /*41 55 41 56 48 83 EC 78 44 8B EA*/

		p_CEngineVGui_RenderStart = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x53\x56\x57\x48\x81\xEC\x00\x00\x00\x00\x0F\x29\x70\xD8"), "xxxxxxxxx????xxxx");
		CEngineVGui_RenderStart = p_CEngineVGui_RenderStart.RCast<void* (*)(CMatSystemSurface*)>(); /*48 8B C4 53 56 57 48 81 EC ?? ?? ?? ?? 0F 29 70 D8*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CEngineVGui_Paint = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x41\x55\x41\x56\x48\x83\xEC\x78\x44\x8B\xEA"), "xxxxxxxxxxx");
		CEngineVGui_Paint = p_CEngineVGui_Paint.RCast<int (*)(CEngineVGui* thisptr, PaintMode_t mode)>(); /*41 55 41 56 48 83 EC 78 44 8B EA*/

		p_CEngineVGui_RenderStart = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xF9"), "xxxxxx????xxx");
		CEngineVGui_RenderStart = p_CEngineVGui_RenderStart.RCast<void* (*)(CMatSystemSurface*)>(); /*40 53 57 48 81 EC ?? ?? ?? ?? 48 8B F9*/
#endif
		p_CEngineVGui_RenderEnd = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\x48\x8B\x0D\x00\x00\x00\x00\xC6\x05\x00\x00\x00\x00\x00\x48\x8B\x01"), "xxxxxxxxx????xx?????xxx");
		CEngineVGui_RenderEnd = p_CEngineVGui_RenderEnd.RCast<void* (*)(void)>(); /*40 53 48 83 EC 20 48 8B 0D ?? ?? ?? ?? C6 05 ?? ?? ?? ?? ?? 48 8B 01*/
	}
	virtual void GetVar(void) const
	{
		g_pEngineVGui = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>(
			"\x48\x8B\xC4\x48\x89\x48\x08\x48\x89\x50\x10\x4C\x89\x40\x18\x4C\x89\x48\x20\x53\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xD9\x48\x8D\x78\x10\xE8\x00\x00\x00\x00\x48\x89\x7C\x24\x00\x48\x8D\x54\x24\x00\x33\xFF\x4C\x8B\xCB\x41\xB8\x00\x00\x00\x00\x48\x89\x7C\x24\x00\x48\x8B\x08\x48\x83\xC9\x01\xE8\x00\x00\x00\x00\x85\xC0\x48\x8D\x54\x24\x00"),
			"xxxxxxxxxxxxxxxxxxxxxxxx????xxxxxxxx????xxxx?xxxx?xxxxxxx????xxxx?xxxxxxxx????xxxxxx?").FindPatternSelf("48 8D ?? ?? ?? ?? 01", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CEngineVGui*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VEngineVGui);