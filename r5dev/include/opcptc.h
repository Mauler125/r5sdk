#pragma once
#include <iostream>
#include "utility.h"

void InstallOpcodes();

namespace
{
	/* -------------- ORIGIN ------------------------------------------------------------------------------------------------------------------------------------------------ */
	DWORD64 dst000 = /*0x14032C910*/ FindPattern("r5apex.exe", (const unsigned char*)"\x48\x83\xEC\x28\x80\x3D\x00\x00\x00\x22\x00\x0F\x85\x00\x02\x00", "xxxxxx???xxxx?xx");
	DWORD64 dst001 = /*0x14023C440*/ FindPattern("r5apex.exe", (const unsigned char*)"\x48\x89\x5C\x24\x18\x55\x41\x56\x41\x57\x48\x81\xEC\x40\x02\x00", "xxxxxxxxxxxxxxxx");

	/* -------------- ENGINE ------------------------------------------------------------------------------------------------------------------------------------------------ */
	DWORD64 dst002 = /*0x140E3E110*/ FindPattern("r5apex.exe", (const unsigned char*)"\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x4C\x8B\xF2\x4C\x8B", "xxxxxxx????xxxxx");

	/* -------------- NETCHAN ----------------------------------------------------------------------------------------------------------------------------------------------- */
	DWORD64 dst003 = /*0x14030BEF0*/ FindPattern("r5apex.exe", (const unsigned char*)"\x44\x89\x44\x24\x18\x55\x56\x57\x48\x8D\xAC\x24\x60\xFF\xFF\xFF", "xxxxxxxxxxxxxxxx");

	/* -------------- FAIRFIGHT --------------------------------------------------------------------------------------------------------------------------------------------- */
	DWORD64 dst004 = /*0x140302D90*/ FindPattern("r5apex.exe", (const unsigned char*)"\x40\x53\x57\x41\x57\x48\x83\xEC\x30\x8B\x81\xB0\x03\x00\x00\x48", "xxxxxxxxxxxxxxxx");

	/* -------------- ------- ----------------------------------------------------------------------------------------------------------------------------------------------- */

	void PrintOAddress() // Test the sigscan results
	{
		std::cout << "--------------------------------------------------" << std::endl;
		std::cout << " dst000                   : " << std::hex << dst000 << std::endl;
		std::cout << " dst001                   : " << std::hex << dst001 << std::endl;
		std::cout << " dst002                   : " << std::hex << dst001 << std::endl;
		std::cout << " dst003                   : " << std::hex << dst003 << std::endl;
		std::cout << " dst004                   : " << std::hex << dst004 << std::endl;
		std::cout << "--------------------------------------------------" << std::endl;

		// TODO implement error handling when sigscan fails or result is 0
	}
}
