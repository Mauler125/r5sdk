#pragma once
#include "hooks.h"

bool HConCommand_IsFlagSet(int* cmd, int flag);

void AttachConCommandHooks();
void DetachConCommandHooks();