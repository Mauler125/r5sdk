//=============================================================================//
//
// Purpose: Command line utilities
//
//=============================================================================//
#include "tier0/commandline.h"

bool g_bCommandLineCreated = false;

//-----------------------------------------------------------------------------
// Purpose: Create a command line from the passed in string
//  Note that if you pass in a @filename, then the routine will read settings
//  from a file instead of the command line
//-----------------------------------------------------------------------------
void CCommandLine::StaticCreateCmdLine(CCommandLine* thisptr, const char* pszCommandLine)
{
	// The SDK creates the cmdline instead, when loaded. We hook this
	// function, and skip the actual creation of subsequent calls.
	// This is required as otherwise our own appended parameters will
	// get lost when the game recreates it in 'LauncherMain'.
	if (!g_bCommandLineCreated)
	{
		CCommandLine__CreateCmdLine(thisptr, pszCommandLine);
	}
}

//-----------------------------------------------------------------------------
// Purpose: parses a text file and appends its parameters/values
//-----------------------------------------------------------------------------
void CCommandLine::AppendParametersFromFile(const char* const pszConfig)
{
	char szTextBuf[2048];
	FILE* pFile = fopen(pszConfig, "r");

	if (!pFile)
	{
		// Try the 'PLATFORM' folder.
		snprintf(szTextBuf, sizeof(szTextBuf), "platform/%s", pszConfig);
		pFile = fopen(szTextBuf, "r");

		if (!pFile)
		{
			printf("%s: '%s' does not exist!\n",
				__FUNCTION__, pszConfig);
			return;
		}
	}

	while (fgets(szTextBuf, sizeof(szTextBuf), pFile))
	{
		// Trim newlines...
		size_t nLen = strlen(szTextBuf);
		if (nLen > 0 && szTextBuf[nLen - 1] == '\n')
		{
			szTextBuf[--nLen] = '\0';
		}

		char* const pSpacePos = strchr(szTextBuf, ' ');
		const char* pArgValue = "";

		if (pSpacePos)
		{
			// Skip space and move to value.
			*pSpacePos = '\0';
			char* pArg = pSpacePos + 1;

			// Trim the quotes around the value.
			if (*pArg == '\"')
			{
				pArg++;
				char* const pEndQuote = strchr(pArg, '\"');

				if (pEndQuote)
				{
					*pEndQuote = '\0';
				}
			}

			pArgValue = pArg;
		}

		AppendParm(szTextBuf, pArgValue);
	}

	fclose(pFile);
}

///////////////////////////////////////////////////////////////////////////////
CCommandLine* g_pCmdLine = CModule::GetExportedSymbol(CModule::GetProcessEnvironmentBlock()->ImageBaseAddress, "g_pCmdLine").RCast<CCommandLine*>();


void VCommandLine::Detour(const bool bAttach) const
{
	DetourSetup(&CCommandLine__CreateCmdLine, &CCommandLine::StaticCreateCmdLine, bAttach);
}
