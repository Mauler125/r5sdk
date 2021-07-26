#pragma once
#include "pch.h"
#include "hooks.h"

bool HPersistence_IsAvailable(__int64 thisptr, int client);

void AttachCEngineServerHooks();
void DetachCEngineServerHooks();