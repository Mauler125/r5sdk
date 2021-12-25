#pragma once

namespace
{
	ADDRESS p_Script_Remote_BeginRegisteringFunctions = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x83\xEC\x28\x83\x3D\x00\x00\x00\x00\x00\x74\x10", "xxxxxx?????xx");
	void* Script_Remote_BeginRegisteringFunctions = (void*)p_Script_Remote_BeginRegisteringFunctions.GetPtr(); /*48 83 EC 28 83 3D ?? ?? ?? ?? ?? 74 10*/

	std::uint32_t* g_nRemoteFunctionCallsChecksum = reinterpret_cast<std::uint32_t*>(p_Script_Remote_BeginRegisteringFunctions.FindPatternSelf("89 05", ADDRESS::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x6).GetPtr());
}

///////////////////////////////////////////////////////////////////////////////
class HSqInit : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: Remote_BeginRegisteringFunctions     : 0x" << std::hex << std::uppercase << p_Script_Remote_BeginRegisteringFunctions.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "| VAR: g_nRemoteFunctionCallsChecksum       : 0x" << std::hex << std::uppercase << g_nRemoteFunctionCallsChecksum                     << std::setw(0) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HSqInit);
