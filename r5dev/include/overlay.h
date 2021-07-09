#pragma once
#include "imgui.h"
#include "serverlisting.h"
#include "json.hpp"
#include <vector>
/////////////////////////////////////////////////////////////////////////////
// Initialization
void PrintDXAddress();
void InstallDXHooks();
void RemoveDXHooks();
void ShowGameConsole(bool* p_open);

/////////////////////////////////////////////////////////////////////////////
// Internals
int Stricmp(const char* s1, const char* s2);
int Strnicmp(const char* s1, const char* s2, int n);
char* Strdup(const char* s);
void  Strtrim(char* s);

/////////////////////////////////////////////////////////////////////////////
// Globals
inline ImVector<char*>       Items;

inline std::string OriginUID = "1010417302770";

/////////////////////////////////////////////////////////////////////////////

using json = nlohmann::json;