#pragma once

namespace
{
	/* ==== COMMON ========================================================================================================================================================== */
	ADDRESS p_COM_ExplainDisconnection = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x8B\xC4\x48\x89\x50\x10\x4C\x89\x40\x18\x4C\x89\x48\x20\x48\x81\xEC\x00\x00\x00\x00", "xxxxxxxxxxxxxxxxxx????");
	void* (*COM_ExplainDisconnection)(void* unused, const char* fmt, ...) = (void* (*)(void*, const char*, ...))p_COM_ExplainDisconnection.GetPtr(); /*48 8B C4 48 89 50 10 4C 89 40 18 4C 89 48 20 48 81 EC ? ? ? ?*/
}

///////////////////////////////////////////////////////////////////////////////
class HCommon : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: COM_ExplainDisconnection             : 0x" << std::hex << std::uppercase << p_COM_ExplainDisconnection.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HCommon);
