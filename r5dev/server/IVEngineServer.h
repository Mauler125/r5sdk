#pragma once

namespace
{
	/* ==== CVENGINESERVER ================================================================================================================================================== */
	ADDRESS p_IVEngineServer_PersistenceAvailable = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x3B\x15\x00\x00\x00\x00\x7D\x33", "xx????xx");
	bool (*IVEngineServer_PersistenceAvailable)(void* entidx, int clientidx) = (bool (*)(void*, int))p_IVEngineServer_PersistenceAvailable.GetPtr(); /*3B 15 ?? ?? ?? ?? 7D 33*/
}

///////////////////////////////////////////////////////////////////////////////
bool HIVEngineServer_PersistenceAvailable(void* entidx, int clientidx);

void IVEngineServer_Attach();
void IVEngineServer_Detach();

///////////////////////////////////////////////////////////////////////////////
extern bool g_bIsPersistenceVarSet[128];

///////////////////////////////////////////////////////////////////////////////
class HVEngineServer : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: IVEngineServer::PersistenceAvailable : 0x" << std::hex << std::uppercase << p_IVEngineServer_PersistenceAvailable.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HVEngineServer);
