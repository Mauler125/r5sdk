#pragma once

void SetupConsole();

bool Hook_ConVar_IsFlagSet(int** cvar, int flag);
bool Hook_ConCommand_IsFlagSet(int* cmd, int flag);

inline bool g_bDebugConsole   = false;
inline bool g_bReturnAllFalse = false;
