//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Defines the entry point for the application.
//
// $NoKeywords: $
//===========================================================================//
#include "core/stdafx.h"
#include "tier0/commandline.h"
#include "launcher/launcher.h"

int HWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	// !TODO [AMOS]: 'RemoveSpuriousGameParameters()' is inline with 'LauncherMain()' in S0 and S1,
	// and its the only function where we could append our own command line parameters early enough
	// programatically (has to be after 'CommandLine()->CreateCmdLine()', but before 'SetPriorityClass()')
	// For S0 and S1 we should modify the command line buffer passed to the entry point instead (here).
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	string svCmdLine = lpCmdLine;
	if (!strstr(GetCommandLineA(), "-launcher"))
	{
		svCmdLine = LoadConfigFile(SDK_DEFAULT_CFG);
	}
	return v_WinMain(hInstance, hPrevInstance, const_cast<LPSTR>(svCmdLine.c_str()), nShowCmd);
#else
	return v_WinMain(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
#endif
}

int LauncherMain(HINSTANCE hInstance)
{
	int results = v_LauncherMain(hInstance);
	spdlog::info("LauncherMain returned: {:s}\n", ExitCodeToString(results));
	return results;
}

#if !defined (GAMEDLL_S0) || !defined (GAMEDLL_S1)
// Remove all but the last -game parameter.
// This is for mods based off something other than Half-Life 2 (like HL2MP mods).
// The Steam UI does 'steam -applaunch 320 -game c:\steam\steamapps\sourcemods\modname', but applaunch inserts
// its own -game parameter, which would supercede the one we really want if we didn't intercede here.
void RemoveSpuriousGameParameters()
{
	AppendSDKParametersPreInit();

	// Find the last -game parameter.
	int nGameArgs = 0;
	char lastGameArg[MAX_PATH];
	for (int i = 0; i < CommandLine()->ParmCount() - 1; i++)
	{
		if (_stricmp(CommandLine()->GetParm(i), "-game") == 0)
		{
			_snprintf(lastGameArg, sizeof(lastGameArg), "\"%s\"", CommandLine()->GetParm(i + 1));
			++nGameArgs;
			++i;
		}
	}

	// We only care if > 1 was specified.
	if (nGameArgs > 1)
	{
		CommandLine()->RemoveParm("-game");
		CommandLine()->AppendParm("-game", lastGameArg);
	}
}
#endif

// Append required command line parameters.
// This avoids having all these in the startup configuration files
// as all there are required to run the game with the game sdk.
void AppendSDKParametersPreInit()
{
#ifdef DEDICATED
	CommandLine()->AppendParm("-sw", "");
	CommandLine()->AppendParm("-lv", "");
	CommandLine()->AppendParm("-safe", "");
	CommandLine()->AppendParm("-high", "");
	CommandLine()->AppendParm("-rerun", "");
	CommandLine()->AppendParm("-collate", "");
	CommandLine()->AppendParm("-multiple", "");
	CommandLine()->AppendParm("-noorigin", "");
	CommandLine()->AppendParm("-nodiscord", "");
	CommandLine()->AppendParm("-novid", "");
	CommandLine()->AppendParm("-noshaderapi", "");
	CommandLine()->AppendParm("-nobakedparticles", "");
	CommandLine()->AppendParm("-nosound", "");
	CommandLine()->AppendParm("-nojoy", "");
	CommandLine()->AppendParm("-nomouse", "");
	CommandLine()->AppendParm("-nomenuvid", "");
	CommandLine()->AppendParm("-nosendtable", "");
	CommandLine()->AppendParm("-gamepad_ignore_local", "");
#endif
	// Assume default configs if the game isn't launched with the SDKLauncher.
	if (!CommandLine()->FindParm("-launcher"))
	{
		string svArguments = LoadConfigFile(SDK_DEFAULT_CFG);
		ParseAndApplyConfigFile(svArguments);
	}
}

string LoadConfigFile(const string& svConfig)
{
	fs::path cfgPath = fs::current_path() /= svConfig; // Get cfg path for default startup.
	ifstream cfgFile(cfgPath);
	string svArguments;

	if (cfgFile.good() && cfgFile)
	{
		stringstream ss;
		ss << cfgFile.rdbuf();
		svArguments = ss.str();
	}
	else
	{
		spdlog::error("{:s}: '{:s}' does not exist!\n", __FUNCTION__, svConfig);
		cfgFile.close();
		return "";
	}
	cfgFile.close();

	return svArguments;
}

void ParseAndApplyConfigFile(const string& svConfig)
{
	stringstream ss(svConfig);
	string svInput;

	if (!svConfig.empty())
	{
		while (std::getline(ss, svInput, '\n'))
		{
			string::size_type nPos = svInput.find(' ');
			if (!svInput.empty()
				&& nPos > 0
				&& nPos < svInput.size()
				&& nPos != svInput.size())
			{
				string svValue = svInput.substr(nPos + 1);
				string svArgument = svInput.erase(svInput.find(' '));

				CommandLine()->AppendParm(svArgument.c_str(), svValue.c_str());
			}
			else
			{
				CommandLine()->AppendParm(svInput.c_str(), "");
			}
		}
	}
}

const char* ExitCodeToString(int nCode)
{
	switch (nCode)
	{
	case EXIT_SUCCESS:
		return "EXIT_SUCCESS";
	case EXIT_FAILURE:
		return "EXIT_FAILURE";
	default:
		return "";
	}
}

void Launcher_Attach()
{
	DetourAttach((LPVOID*)&v_WinMain, &HWinMain);
	DetourAttach((LPVOID*)&v_LauncherMain, &LauncherMain);
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1)
	DetourAttach((LPVOID*)&v_RemoveSpuriousGameParameters, &RemoveSpuriousGameParameters);
#endif
}

void Launcher_Detach()
{
	DetourDetach((LPVOID*)&v_WinMain, &HWinMain);
	DetourDetach((LPVOID*)&v_LauncherMain, &LauncherMain);
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1)
	DetourDetach((LPVOID*)&v_RemoveSpuriousGameParameters, &RemoveSpuriousGameParameters);
#endif
}