#include "core/stdafx.h"
#include "ebisusdk/EbisuSDK.h"
#include "engine/server/sv_main.h"

//-----------------------------------------------------------------------------
// Purpose: sets the EbisuSDK globals for dedicated to satisfy command callbacks
//-----------------------------------------------------------------------------
void HEbisuSDK_Init()
{
	if (IsDedicated())
	{
		*g_EbisuSDKInit = true; // <- 1st EbisuSDK
		*g_EbisuProfileInit = true; // <- 2nd EbisuSDK
		*g_NucleusID = 9990000; // <- 3rd EbisuSDK
	}
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
