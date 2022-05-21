#pragma once

#include <cstdint>
#include "StringBase.h"

namespace Diagnostics
{
	// Information about a loaded module
	struct ProcessModule
	{
		// Returns the memory address that the module was loaded at.
		uint64_t BaseAddress;
		// Returns the amount of memory required to load the module.This does
		// not include any additional memory allocations made by the module once
		// it is running; it only includes the size of the static code and data
		// in the module file.
		uint64_t ModuleMemorySize;
		// Returns the memory address for function that runs when the module is loaded and run.
		uint64_t EntryPointAddress;

		// Returns the name of the Module.
		string ModuleName;
		// Returns the full file path for the location of the module.
		string FileName;
	};
}