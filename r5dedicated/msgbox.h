#pragma once
#include "hooks.h"

int HMSG_EngineError(char* fmt, va_list args);

void AttachMSGBoxHooks();
void DetachMSGBoxHooks();