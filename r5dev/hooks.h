#pragma once

void InstallHooks();
void RemoveHooks();
void ToggleDevCommands();
void ToggleNetHooks();

bool Hook_ConVar_IsFlagSet(int** cvar, int flag);
bool Hook_ConCommand_IsFlagSet(int* cmd, int flag);