#include "core/stdafx.h"
#include "tier1/cvar.h"
#include "ebisusdk/EbisuSDK.h"

//-----------------------------------------------------------------------------
// Purpose: sets the EbisuSDK globals for dedicated to satisfy command callbacks
//-----------------------------------------------------------------------------
void HEbisuSDK_Init()
{
#ifdef DEDICATED
	*g_EbisuSDKInit     = true; // <- 1st EbisuSDK
	*g_EbisuProfileInit = true; // <- 2nd EbisuSDK
	*g_NucleusID        = true; // <- 3rd EbisuSDK
#endif // DEDICATED
}

//-----------------------------------------------------------------------------
// Purpose: checks if the EbisuSDK is initialized
// Output : true on success, false on failure
//-----------------------------------------------------------------------------
bool IsOriginInitialized()
{
#ifndef DEDICATED
	if ((!(*g_OriginErrorLevel)
		&& (*g_EbisuSDKInit)
		&& (*g_NucleusID)
		&& (*g_EbisuProfileInit)))
	//	&& (*g_OriginAuthCode)
	//		&& (g_NucleusToken[0])))
#endif // DEDICATED
	{
		return true;
	}
	return false;
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: validates if client's persona name meets EA's criteria
// Input  : *pszName -
// Output : true on success, false on failure
//-----------------------------------------------------------------------------
bool IsValidPersonaName(const char* pszName)
{
	if (!sv_validatePersonaName->GetBool())
	{
		return true;
	}

	size_t len = strlen(pszName);

	if (len < sv_minPersonaNameLength->GetInt() || 
		len > sv_maxPersonaNameLength->GetInt())
	{
		return false;
	}

	// Check if the name contains any special characters.
	size_t pos = strspn(pszName, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_");
	return pszName[pos] == '\0';
}
#endif // !CLIENT_DLL
