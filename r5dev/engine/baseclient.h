#pragma once

namespace
{
	/* ==== CBASECLIENT ===================================================================================================================================================== */
	ADDRESS p_CBaseClient_Clear = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x53\x41\x56\x41\x57\x48\x83\xEC\x20\x48\x8B\xD9\x48\x89\x74", "xxxxxxxxxxxxxxxx");
	std::int64_t* (*CBaseClient_Clear)(std::int64_t client) = (std::int64_t * (*)(std::int64_t))p_CBaseClient_Clear.GetPtr(); /*40 53 41 56 41 57 48 83 EC 20 48 8B D9 48 89 74*/
}

///////////////////////////////////////////////////////////////////////////////
std::int64_t* HCBaseClient_Clear(std::int64_t client);

void CBaseClient_Attach();
void CBaseClient_Detach();

///////////////////////////////////////////////////////////////////////////////
class HBaseClient : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: CBaseClient::Clear                   : 0x" << std::hex << std::uppercase << p_CBaseClient_Clear.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HBaseClient);
