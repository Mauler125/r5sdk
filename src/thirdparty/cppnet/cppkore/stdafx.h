#pragma once

// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN
// We do not want the non-portable library functions
//#define _CRT_SECURE_NO_WARNINGS

// Platform includes
#include <Windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <string>
#include <sstream>
#include <memory>
#include <Psapi.h>
#include <TlHelp32.h>
#include <io.h>
#include <immintrin.h>
#include <intrin.h>
#include <algorithm>
#include <fcntl.h>

// Debug printing macro
#if _DEBUG
#define dprintf(x, ...) printf(x, __VA_ARGS__);
#else
#define dprintf(x, ...) ((void*)0);
#endif