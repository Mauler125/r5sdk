#pragma once
#pragma message("Pre-compiling headers.\n")

#define WIN32_LEAN_AND_MEAN // Prevent winsock2 redefinition.
#include <windows.h>
#include <minhook.h>
#include <WinSock2.h>
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
#include <string>
#include <Psapi.h>
#include <vector>


// Our headers

#include "spdlog.h"
#include "sinks/basic_file_sink.h"
#include "sinks/stdout_sinks.h"
#include "sinks/ostream_sink.h"
#include "utility.h"
//#include "httplib.h"
//#include "json.hpp"

#include "address.h"

#pragma once

#define FUNC_AT_ADDRESS(name, funcbody, addr) \
   using _##name = funcbody; \
   _##name name = (funcbody)addr \

#define PRINT_ADDRESS(name, address) std::cout << name << ": " << std::hex << std::uppercase << address << std::endl;
