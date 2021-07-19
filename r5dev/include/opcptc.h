#pragma once

void InstallOpcodes();
inline HANDLE GameProcess = GetCurrentProcess();

namespace
{
	/* -------------- ORIGIN ------------------------------------------------------------------------------------------------------------------------------------------------ */
	DWORD64 dst000 /*0x14032EEA0*/ = reinterpret_cast<DWORD64>(PatternScan("r5apex.exe", "48 83 EC 28 80 3D ? ? ? 23 ? 0F 85 ? 02 ?"));
	DWORD64 dst001 /*0x140330290*/ = reinterpret_cast<DWORD64>(PatternScan("r5apex.exe", "48 81 EC 58 04 ? ? 80 3D ? ? ? ? ? 0F 84"));

	/* -------------- ENGINE ------------------------------------------------------------------------------------------------------------------------------------------------ */
	DWORD64 dst002 /*0x14043FB90*/ = reinterpret_cast<DWORD64>(PatternScan("r5apex.exe", "48 89 4C 24 08 56 41 55 48 81 EC 68 03 ? ? 4C"));
	DWORD64 dst004 /*0x14022A4A0*/ = reinterpret_cast<DWORD64>(PatternScan("r5apex.exe", "48 83 EC 38 0F 29 74 24 20 48 89 5C 24 40 48 8B"));
	DWORD64 Host_NewGame /*0x140238DA0*/ = reinterpret_cast<DWORD64>(PatternScan("r5apex.exe", "48 8B C4 ? 41 54 41 ? 48 81 EC ? ? ? ? F2"));

	/* -------------- NETCHAN ----------------------------------------------------------------------------------------------------------------------------------------------- */
	DWORD64 dst006 /*0x14030D000*/ = reinterpret_cast<DWORD64>(PatternScan("r5apex.exe", "40 55 57 41 55 41 57 48 8D AC 24 ? ? ? ?"));

	/* -------------- FAIRFIGHT --------------------------------------------------------------------------------------------------------------------------------------------- */
	DWORD64 dst007 /*0x140303AE0*/  = reinterpret_cast<DWORD64>(PatternScan("r5apex.exe", "40 53 48 83 EC 20 8B 81 B0 03 ? ? 48 8B D9 C6"));

	/* -------------- ------- ----------------------------------------------------------------------------------------------------------------------------------------------- */

	void PrintOAddress() // Test the sigscan results
	{
		std::cout << "+--------------------------------------------------------+" << std::endl;
		std::cout << "| dst000                   : " << std::hex << std::uppercase << dst000 << std::setw(20) << " |" << std::endl;
		std::cout << "| dst001                   : " << std::hex << std::uppercase << dst001 << std::setw(20) << " |" << std::endl;
		std::cout << "+--------------------------------------------------------+" << std::endl;
		std::cout << "| dst002                   : " << std::hex << std::uppercase << dst002 << std::setw(20) << " |" << std::endl;
		std::cout << "| dst004                   : " << std::hex << std::uppercase << dst004 << std::setw(20) << " |" << std::endl;
		std::cout << "| Host_NewGame             : " << std::hex << std::uppercase << Host_NewGame << std::setw(20) << " |" << std::endl;
		std::cout << "+--------------------------------------------------------+" << std::endl;
		std::cout << "| dst006                   : " << std::hex << std::uppercase << dst006 << std::setw(20) << " |" << std::endl;
		std::cout << "| dst007                   : " << std::hex << std::uppercase << dst007 << std::setw(20) << " |" << std::endl;
		std::cout << "+--------------------------------------------------------+" << std::endl;

		// TODO implement error handling when sigscan fails or result is 0
	}
}
