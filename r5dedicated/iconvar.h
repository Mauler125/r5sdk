#pragma once
#include "hooks.h"

bool HConVar_IsFlagSet(int** cvar, int flag);

void AttachIConVarHooks();
void DetachIConVarHooks();