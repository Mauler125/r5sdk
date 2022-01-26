#pragma once

namespace
{
	/* ==== CVENGINESERVER ================================================================================================================================================== */
	ADDRESS p_IVEngineServer_PersistenceAvailable = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x3B\x15\x00\x00\x00\x00\x7D\x33", "xx????xx");
	bool (*IVEngineServer_PersistenceAvailable)(void* entidx, int clientidx) = (bool (*)(void*, int))p_IVEngineServer_PersistenceAvailable.GetPtr(); /*3B 15 ?? ?? ?? ?? 7D 33*/

	ADDRESS p_IVEngineServer_IsDedicatedServer = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x0F\xB6\x05\x00\x00\x00\x00\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48\x8B\x05\x00\x00\x00\x00\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x40\x53", "xxx????xxxxxxxxxxxx????xxxxxxxxxxx");
	bool (*IVEngineServer_IsDedicatedServer)(void) = (bool (*)(void))p_IVEngineServer_IsDedicatedServer.GetPtr(); /*0F B6 05 ? ? ? ? C3 CC CC CC CC CC CC CC CC 48 8B 05 ? ? ? ? C3 CC CC CC CC CC CC CC CC 40 53*/

	bool* g_bDedicated = p_IVEngineServer_IsDedicatedServer.Offset(0x0).ResolveRelativeAddress(0x3, 0x7).RCast<bool*>();
}

///////////////////////////////////////////////////////////////////////////////
bool HIVEngineServer_PersistenceAvailable(void* entidx, int clientidx);

void IVEngineServer_Attach();
void IVEngineServer_Detach();

///////////////////////////////////////////////////////////////////////////////
extern bool g_bIsPersistenceVarSet[MAX_PLAYERS];

///////////////////////////////////////////////////////////////////////////////
class HVEngineServer : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: IVEngineServer::PersistenceAvailable : 0x" << std::hex << std::uppercase << p_IVEngineServer_PersistenceAvailable.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: IVEngineServer::IsDedicatedServer    : 0x" << std::hex << std::uppercase << p_IVEngineServer_IsDedicatedServer.GetPtr()    << std::setw(npad) << " |" << std::endl;
		std::cout << "| VAR: g_bDedicated                         : 0x" << std::hex << std::uppercase << g_bDedicated                                   << std::setw(0) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HVEngineServer);
