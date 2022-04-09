#pragma once

/* ==== CVENGINESERVER ================================================================================================================================================== */
inline ADDRESS p_IVEngineServer__PersistenceAvailable = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x3B\x15\x00\x00\x00\x00\x7D\x33"), "xx????xx");
inline auto IVEngineServer__PersistenceAvailable = p_IVEngineServer__PersistenceAvailable.RCast<bool (*)(void* entidx, int clientidx)>(); /*3B 15 ?? ?? ?? ?? 7D 33*/

inline ADDRESS p_IVEngineServer__IsDedicatedServer = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x0F\xB6\x05\x00\x00\x00\x00\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48\x8B\x05\x00\x00\x00\x00\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x40\x53"), "xxx????xxxxxxxxxxxx????xxxxxxxxxxx");
inline auto IVEngineServer__IsDedicatedServer = p_IVEngineServer__IsDedicatedServer.RCast<bool (*)(void)>(); /*0F B6 05 ? ? ? ? C3 CC CC CC CC CC CC CC CC 48 8B 05 ? ? ? ? C3 CC CC CC CC CC CC CC CC 40 53*/

inline ADDRESS p_IVEngineServer__GetNumHumanPlayers = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x8B\x15\x00\x00\x00\x00\x33\xC0\x85\xD2\x7E\x24"), "xx????xxxxxx");
inline auto IVEngineServer__GetNumHumanPlayers = p_IVEngineServer__GetNumHumanPlayers.RCast<int64_t(*)(void)>(); /*8B 15 ? ? ? ? 33 C0 85 D2 7E 24*/

inline ADDRESS p_IVEngineServer__GetNumFakeClients = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x8B\x05\x00\x00\x00\x00\x33\xC9\x85\xC0\x7E\x2D"), "xx????xxxxxx");
inline auto IVEngineServer__GetNumFakeClients = p_IVEngineServer__GetNumFakeClients.RCast<int64_t(*)(void)>(); /*8B 05 ? ? ? ? 33 C9 85 C0 7E 2D*/

inline bool* g_bDedicated = p_IVEngineServer__IsDedicatedServer.Offset(0x0).ResolveRelativeAddress(0x3, 0x7).RCast<bool*>();

///////////////////////////////////////////////////////////////////////////////
bool HIVEngineServer__PersistenceAvailable(void* entidx, int clientidx);

void IVEngineServer_Attach();
void IVEngineServer_Detach();

///////////////////////////////////////////////////////////////////////////////
extern bool g_bIsPersistenceVarSet[MAX_PLAYERS];

///////////////////////////////////////////////////////////////////////////////
class HVEngineServer : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: IVEngineServer::PersistenceAvailable : 0x" << std::hex << std::uppercase << p_IVEngineServer__PersistenceAvailable.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: IVEngineServer::IsDedicatedServer    : 0x" << std::hex << std::uppercase << p_IVEngineServer__IsDedicatedServer.GetPtr()    << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: IVEngineServer::GetNumHumanPlayers   : 0x" << std::hex << std::uppercase << p_IVEngineServer__GetNumHumanPlayers.GetPtr()   << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: IVEngineServer::GetNumFakeClients    : 0x" << std::hex << std::uppercase << p_IVEngineServer__GetNumFakeClients.GetPtr()    << std::setw(npad) << " |" << std::endl;
		std::cout << "| VAR: g_bDedicated                         : 0x" << std::hex << std::uppercase << g_bDedicated                                    << std::setw(0) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HVEngineServer);
