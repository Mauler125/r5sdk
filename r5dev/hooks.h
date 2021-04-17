#pragma once

void InstallHooks();
void RemoveHooks();
void ToggleDevCommands();
void ToggleNetHooks();

bool Hook_Cvar_IsFlagSet(int** cvar, int flag);