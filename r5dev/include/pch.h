#pragma once
#pragma message("Pre-compiling headers.\n")

#include "httplib.h"
#define WIN32_LEAN_AND_MEAN // Prevent winsock2 redefinition.
#include <windows.h>
#include <detours.h>
#include <thread>
#include <fstream>
#include <stdio.h>
#include <filesystem>
#include <sstream>
#include <shlobj.h>
#include <objbase.h>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <d3d11.h>
#include <string>
#include <Psapi.h>
#include <sinks/basic_file_sink.h>
#include <vector>



// Our headers

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "spdlog.h"
#include "utility.h"
#include "json.hpp"