#include "core/stdafx.h"
#include "tier0/commandline.h"
#include "ebisusdk/EbisuSDK.h"
#include "engine/server/sv_main.h"

//-----------------------------------------------------------------------------
// Purpose: initialize the EbisuSDK
//-----------------------------------------------------------------------------
void HEbisuSDK_Init()
{
	const bool isDedicated = IsDedicated();
	const bool noOrigin = IsOriginDisabled();

	// Fill with default data if this is a dedicated server, or if the game was
	// launched with the platform system disabled. Engine code requires these
	// to be set for the game to function, else stuff like the "map" command
	// won't run as 'IsOriginInitialized()' returns false (which got inlined in
	// every place this was called in the game's executable).
	if (isDedicated || noOrigin)
	{
		*g_EbisuSDKInit = true;
		*g_EbisuProfileInit = true;
		*g_NucleusID = FAKE_BASE_NUCLEUD_ID;

		Q_snprintf(g_OriginAuthCode, 256, "%s", "INVALID_OAUTH_CODE");
		Q_snprintf(g_NucleusToken, 1024, "%s", "INVALID_NUCLEUS_TOKEN");

		if (!isDedicated)
		{
			platform_user_id->SetValue(FAKE_BASE_NUCLEUD_ID);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: runs the EbisuSDK state machine
//-----------------------------------------------------------------------------
void HEbisuSDK_RunFrame()
{
	if (IsOriginDisabled())
	{
		return;
	}

	EbisuSDK_RunFrame();
}

//-----------------------------------------------------------------------------
// Purpose: returns the currently set language
//-----------------------------------------------------------------------------
const char* HEbisuSDK_GetLanguage()
{
	static bool initialized = false;
	static char languageName[32];

	if (initialized)
	{
		return languageName;
	}

	const char* value = nullptr;
	bool useDefault = true;

	if (CommandLine()->CheckParm("-language", &value))
	{
		if (V_LocaleNameExists(value))
		{
			strncpy(languageName, value, sizeof(languageName));
			useDefault = false;
		}
	}

	if (useDefault)
	{
		strncpy(languageName, g_LanguageNames[0], sizeof(languageName));
	}

	languageName[sizeof(languageName) - 1] = '\0';
	initialized = true;

	return languageName;
}

//-----------------------------------------------------------------------------
// Purpose: checks if the EbisuSDK is disabled
// Output : true on success, false on failure
//-----------------------------------------------------------------------------
bool IsOriginDisabled()
{
	const static bool isDisabled = CommandLine()->CheckParm("-noorigin");
	return isDisabled;
}

//-----------------------------------------------------------------------------
// Purpose: checks if the EbisuSDK is initialized
// Output : true on success, false on failure
//-----------------------------------------------------------------------------
bool IsOriginInitialized()
{
	if (IsDedicated())
	{
		return true;
	}
	else if ((!(*g_OriginErrorLevel)
		&& (*g_EbisuSDKInit)
		&& (*g_NucleusID)
		&& (*g_EbisuProfileInit)))
	//	&& (*g_OriginAuthCode)
	//		&& (g_NucleusToken[0])))
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: validates if client's persona name meets EA's criteria
// Input  : *pszName -
// Output : true on success, false on failure
//-----------------------------------------------------------------------------
bool IsValidPersonaName(const char* pszName, int nMinLen, int nMaxLen)
{
	size_t len = strlen(pszName);

	if (len < nMinLen ||
		len > nMaxLen)
	{
		return false;
	}

	// Check if the name contains any special characters.
	size_t pos = strspn(pszName, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_");
	return pszName[pos] == '\0';
}

void VEbisuSDK::Detour(const bool bAttach) const
{
	DetourSetup(&EbisuSDK_RunFrame, &HEbisuSDK_RunFrame, bAttach);
	DetourSetup(&EbisuSDK_GetLanguage, &HEbisuSDK_GetLanguage, bAttach);
}
