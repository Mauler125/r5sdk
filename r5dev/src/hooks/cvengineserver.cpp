#include "pch.h"
#include "hooks.h"

namespace Hooks
{
	IsPersistenceDataAvailableFn originalIsPersistenceDataAvailable = nullptr;
}

// TODO: turn this into a playerstruct constructor if it ever becomes necessary
bool Hooks::IsPersistenceDataAvailable(__int64 thisptr, int client)
{
	static bool isPersistenceVarSet[256];

	// TODO: Maybe not hardcode
	std::uintptr_t playerStructBase = 0x16073B200;
	std::uintptr_t playerStructSize = 0x4A4C0;
	std::uintptr_t persistenceVar = 0x5BC;

	std::uintptr_t targetPlayerStruct = playerStructBase + client * playerStructSize;

	*(char*)(targetPlayerStruct + persistenceVar) = (char)0x5;

	if (!isPersistenceVarSet[client])
		isPersistenceVarSet[client] = true;

	return originalIsPersistenceDataAvailable(thisptr, client);
}