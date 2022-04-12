//=============================================================================//
//
// Purpose: Implement things from GameInterface.cpp. Mostly the engine interfaces.
//
// $NoKeywords: $
//=============================================================================//

#include "core/stdafx.h"
#include "engine/sv_main.h"
#include "game/server/gameinterface.h"

//-----------------------------------------------------------------------------
// This is called when a new game is started. (restart, map)
//-----------------------------------------------------------------------------
void CServerGameDLL::GameInit(void)
{
	static int index = 1;
	CallVFunc<void>(index, this);
}

//-----------------------------------------------------------------------------
// This is called when scripts are getting recompiled. (restart, map, changelevel)
//-----------------------------------------------------------------------------
void CServerGameDLL::PrecompileScriptsJob(void)
{
	static int index = 2;
	CallVFunc<void>(index, this);
}

//-----------------------------------------------------------------------------
// Called when a level is shutdown (including changing levels)
//-----------------------------------------------------------------------------
void CServerGameDLL::LevelShutdown(void)
{
	static int index = 8;
	CallVFunc<void>(index, this);
}

//-----------------------------------------------------------------------------
// This is called when a game ends (server disconnect, death, restart, load)
// NOT on level transitions within a game
//-----------------------------------------------------------------------------
void CServerGameDLL::GameShutdown(void)
{
	static int index = 9;
	CallVFunc<void>(index, this);
}

//-----------------------------------------------------------------------------
// Purpose: Gets the simulation tick interfal
// Output : float
//-----------------------------------------------------------------------------
float CServerGameDLL::GetTickInterval(void)
{
	static int index = 11;
	return CallVFunc<float>(index, this);
}

// Pointer to CServerGameDLL virtual function table.
CServerGameDLL* g_pServerGameDLL = nullptr;
CServerGameClients* g_pServerGameClients = nullptr;
