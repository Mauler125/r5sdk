#pragma once
#include "pch.h"
#include "hooks.h"

void* HSQVM_PrintFunc(void* sqvm, char* fmt, ...);
__int64 HSQVM_LoadRson(const char* rson_name);
bool HSQVM_LoadScript(void* sqvm, const char* script_path, const char* script_name, int flag);

void AttachSQVMHooks();
void DetachSQVMHooks();