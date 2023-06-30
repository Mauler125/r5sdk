//=============================================================================//
//
// Purpose: Command line utilities
//
//=============================================================================//
#include "tier0/commandline.h"

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

		CommandLine()->AppendParm(szTextBuf, pArgValue);
	}

	fclose(pFile);
}

///////////////////////////////////////////////////////////////////////////////
CCommandLine* g_pCmdLine = nullptr;
