#pragma once
#include <iostream>
#include <iomanip>

#include "utility.h"

void InstallOpcodes();
void InstallGlobals();
inline HANDLE GameProcess = GetCurrentProcess();

namespace
{
	/* -------------- ORIGIN ------------------------------------------------------------------------------------------------------------------------------------------------ */
	DWORD64 dst000 = /*0x14032EEA0*/ FindPattern("r5apex.exe", (const unsigned char*)"\x48\x83\xEC\x28\x80\x3D\x00\x00\x00\x23\x00\x0F\x85\x00\x02\x00", "xxxxxx???xxxx?xx");
	DWORD64 dst001 = /*0x140330290*/ FindPattern("r5apex.exe", (const unsigned char*)"\x48\x81\xEC\x58\x04\x00\x00\x80\x3D\x00\x00\x00\x00\x00\x0F\x84", "xxxxxxxxx????xxx");

	/* -------------- ENGINE ------------------------------------------------------------------------------------------------------------------------------------------------ */
	DWORD64 dst002 = /*0x14043FB90*/ FindPattern("r5apex.exe", (const unsigned char*)"\x48\x89\x4C\x24\x08\x56\x41\x55\x48\x81\xEC\x68\x03\x00\x00\x4C", "xxxx?xxxxxxxxxxx");
	DWORD64 dst003 = /*0x140302FF0*/ FindPattern("r5apex.exe", (const unsigned char*)"\x40\x53\x41\x56\x41\x57\x48\x83\xEC\x20\x48\x8B\xD9\x48\x89\x74", "xxxxxxxxxxxxxxxx");
	DWORD64 dst004 = /*0x14022A4A0*/ FindPattern("r5apex.exe", (const unsigned char*)"\x48\x83\xEC\x38\x0F\x29\x74\x24\x20\x48\x89\x5C\x24\x40\x48\x8B", "xxxxxxxxxxxxxxxx");
	DWORD64 dst005 = /*0x140238DA0*/ FindPattern("r5apex.exe", (const unsigned char*)"\x48\x8B\xC4\x00\x41\x54\x41\x00\x48\x81\xEC\x00\x00\x00\x00\xF2", "xxx?xxx?xxx??xxx");

	/* -------------- NETCHAN ----------------------------------------------------------------------------------------------------------------------------------------------- */
	DWORD64 dst006 = /*0x14030D000*/ FindPattern("r5apex.exe", (const unsigned char*)"\x40\x55\x57\x41\x55\x41\x57\x48\x8D\xAC\x24\x28\xFF\xFF\xFF\x48", "xxxxxxxxxxxxxxxx");

	/* -------------- FAIRFIGHT --------------------------------------------------------------------------------------------------------------------------------------------- */
	DWORD64 dst007 = /*0x140303AE0*/ FindPattern("r5apex.exe", (const unsigned char*)"\x40\x53\x48\x83\xEC\x20\x8B\x81\xB0\x03\x00\x00\x48\x8B\xD9\xC6", "xxxxxxxxxxxxxxxx");

	/* -------------- ------- ----------------------------------------------------------------------------------------------------------------------------------------------- */

	/* -------------- GLOBALS ----------------------------------------------------------------------------------------------------------------------------------------------- */
	DWORD64 ofs000 = 0x000000016073B7BC;

	void PrintOAddress() // Test the sigscan results
	{
		std::cout << "+--------------------------------------------------------+" << std::endl;
		std::cout << "| dst000                   : " << std::hex << dst000 << std::setw(20) << " |" << std::endl;
		std::cout << "| dst001                   : " << std::hex << dst001 << std::setw(20) << " |" << std::endl;
		std::cout << "+--------------------------------------------------------+" << std::endl;
		std::cout << "| dst002                   : " << std::hex << dst002 << std::setw(20) << " |" << std::endl;
		std::cout << "| dst003                   : " << std::hex << dst003 << std::setw(20) << " |" << std::endl;
		std::cout << "| dst004                   : " << std::hex << dst004 << std::setw(20) << " |" << std::endl;
		std::cout << "| dst005                   : " << std::hex << dst005 << std::setw(20) << " |" << std::endl;
		std::cout << "+--------------------------------------------------------+" << std::endl;
		std::cout << "| dst006                   : " << std::hex << dst006 << std::setw(20) << " |" << std::endl;
		std::cout << "| dst007                   : " << std::hex << dst007 << std::setw(20) << " |" << std::endl;
		std::cout << "+--------------------------------------------------------+" << std::endl;

		// TODO implement error handling when sigscan fails or result is 0
	}
}
