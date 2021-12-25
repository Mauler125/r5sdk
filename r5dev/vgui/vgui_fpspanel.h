#pragma once
#include "tier0/IConVar.h"
namespace
{
	/* ==== CFPSPANEL ======================================================================================================================================================= */
	ADDRESS p_CFPSPanel_Paint = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x8B\xC4\x55\x56\x41\x00\x48\x8D\xA8\x00\xFD\xFF\xFF\x48\x81\xEC\x80", "xxxxxx?xxx?xxxxxxx");
	ConVar* (*CFPSPanel_Paint)(void* thisptr) = (ConVar* (*)(void*))p_CFPSPanel_Paint.GetPtr(); /*48 8B C4 55 56 41 ?? 48 8D A8 ?? FD FF FF 48 81 EC 80*/
}

void CFPSPanel_Attach();
void CFPSPanel_Detach();

///////////////////////////////////////////////////////////////////////////////
class HFPSPanel : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: CFPSPanel::Paint                     : 0x" << std::hex << std::uppercase << p_CFPSPanel_Paint.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HFPSPanel);
