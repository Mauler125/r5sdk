#pragma once
#include <iostream>
#include "utility.h"

void InstallOpcodes();
inline HANDLE GameProcess = GetCurrentProcess();

namespace
{
	/* -------------- ORIGIN ------------------------------------------------------------------------------------------------------------------------------------------------ */
	DWORD64 dst000 = /*0x14032C910*/ FindPattern("r5apex.exe", (const unsigned char*)"\x48\x83\xEC\x28\x80\x3D\x00\x00\x00\x23\x00\x0F\x85\x00\x02\x00", "xxxxxx???xxxx?xx");
	DWORD64 dst001 = /*0x14023C440*/ FindPattern("r5apex.exe", (const unsigned char*)"\x48\x81\xEC\x58\x04\x00\x00\x80\x3D\x00\x00\x00\x00\x00\x0F\x84", "xxxxxxxxx????xxx");

	/* -------------- ENGINE ------------------------------------------------------------------------------------------------------------------------------------------------ */
	DWORD64 dst002 = /*0x140438DE0*/ FindPattern("r5apex.exe", (const unsigned char*)"\x48\x89\x4C\x24\x08\x56\x41\x55\x48\x81\xEC\x68\x03\x00\x00\x4C", "xxxx?xxxxxxxxxxx");
	DWORD64 dst003 = /*0x1403604E0*/ FindPattern("r5apex.exe", (const unsigned char*)"\x40\x53\x41\x56\x41\x57\x48\x83\xEC\x20\x48\x8B\xD9\x48\x89\x74", "xxxxxxxxxxxxxxxx");

	/* -------------- NETCHAN ----------------------------------------------------------------------------------------------------------------------------------------------- */
	DWORD64 dst004 = /*0x14030BEF0*/ FindPattern("r5apex.exe", (const unsigned char*)"\x40\x55\x57\x41\x55\x41\x57\x48\x8D\xAC\x24\x28\xFF\xFF\xFF\x48", "xxxxxxxxxxxxxxxx");

	/* -------------- FAIRFIGHT --------------------------------------------------------------------------------------------------------------------------------------------- */
	DWORD64 dst005 = /*0x140302D90*/ FindPattern("r5apex.exe", (const unsigned char*)"\x40\x53\x48\x83\xEC\x20\x8B\x81\xB0\x03\x00\x00\x48\x8B\xD9\xC6", "xxxxxxxxxxxxxxxx");

	/* -------------- OTHER ------------------------------------------------------------------------------------------------------------------------------------------------- */
	/**/
	/* -------------- ------- ----------------------------------------------------------------------------------------------------------------------------------------------- */

	void PrintOAddress() // Test the sigscan results
	{
		std::cout << "+--------------------------------------------------------+" << std::endl;
		std::cout << "| dst000                   : " << std::hex << dst000 << std::endl;
		std::cout << "| dst001                   : " << std::hex << dst001 << std::endl;
		std::cout << "+--------------------------------------------------------+" << std::endl;
		std::cout << "| dst002                   : " << std::hex << dst002 << std::endl;
		std::cout << "| dst003                   : " << std::hex << dst003 << std::endl;
		std::cout << "+--------------------------------------------------------+" << std::endl;
		std::cout << "| dst004                   : " << std::hex << dst004 << std::endl;
		std::cout << "| dst005                   : " << std::hex << dst005 << std::endl;
		std::cout << "+--------------------------------------------------------+" << std::endl;

		// TODO implement error handling when sigscan fails or result is 0
	}
}
