//=============================================================================//
//
// Purpose: Console Variables
//
//=============================================================================//

#include "core/stdafx.h"
#include "tier0/cvar.h"
#include "tier0/IConVar.h"
#include "engine/sys_utils.h"
#include "engine/sys_dll2.h"
#include "mathlib/bits.h"

//-----------------------------------------------------------------------------
// purpose: construct/allocate
//-----------------------------------------------------------------------------
ConVar::ConVar(const char* pszName, const char* pszDefaultValue, int nFlags, const char* pszHelpString, bool bMin, float fMin, bool bMax, float fMax, void* pCallback, void* unk)
{
	// Here we should make a proper constructor so we don't need casts etc. Maybe make a custom class/struct or use existing one, and then use register function at bottom.
	ConVar* allocatedConvar = reinterpret_cast<ConVar*>(MemAlloc_Wrapper(0xA0)); // Allocate new memory with StdMemAlloc else we crash.
	memset(allocatedConvar, 0, 0xA0);                                            // Set all to null.
	std::uintptr_t cvarPtr = reinterpret_cast<std::uintptr_t>(allocatedConvar);  // To ptr.

	*(void**)(cvarPtr + 0x40) = g_pIConVarVtable.RCast<void*>(); // 0x40 to ICvar table.
	*(void**)cvarPtr = g_pConVarVtable.RCast<void*>();           // 0x0 to ConVar vtable.

	p_ConVar_Register.RCast<void(*)(ConVar*, const char*, const char*, int, const char*, bool, float, bool, float, void*, void*)>()
		(allocatedConvar, pszName, pszDefaultValue, nFlags, pszHelpString, bMin, fMin, bMax, fMax, pCallback, unk); // Call to create ConVar.

	*this = *allocatedConvar;
}

//-----------------------------------------------------------------------------
// purpose: destructor
//-----------------------------------------------------------------------------
ConVar::~ConVar(void)
{
	if (m_pzsCurrentValue)
	{
		delete[] m_pzsCurrentValue;
		m_pzsCurrentValue = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: register ConVar
//-----------------------------------------------------------------------------
void ConVar::Init(void)
{
	//-------------------------------------------------------------------------
	// ENGINE                                                                 |
	cm_debug_cmdquery               = new ConVar("cm_debug_cmdquery", "0", FCVAR_DEVELOPMENTONLY, "Prints the flags of each ConVar/ConCommand query to the console ( !slower! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	cm_return_false_cmdquery_all    = new ConVar("cm_return_false_cmdquery_all", "0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Returns false on every ConVar/ConCommand query ( !warning! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	cm_return_false_cmdquery_cheats = new ConVar("cm_return_false_cmdquery_cheats", "0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Returns false on all FCVAR_DEVELOPMENTONLY and FCVAR_CHEAT ConVar/ConCommand queries ( !warning! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	r_debug_overlay_nodecay         = new ConVar("r_debug_overlay_nodecay", "0", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Keeps all debug overlays alive regardless of their lifetime. Use command 'clear_debug_overlays' to clear everything.", false, 0.f, false, 0.f, nullptr, nullptr);

	rcon_address  = new ConVar("rcon_address",  "::", FCVAR_SERVER_CANNOT_QUERY | FCVAR_DONTRECORD | FCVAR_RELEASE, "Remote console address.", false, 0.f, false, 0.f, nullptr, nullptr);
	rcon_password = new ConVar("rcon_password", "", FCVAR_SERVER_CANNOT_QUERY | FCVAR_DONTRECORD | FCVAR_RELEASE, "Remote console password, RCON is disabled if empty.", false, 0.f, false, 0.f, nullptr, nullptr);
	//-------------------------------------------------------------------------
	// SERVER                                                                 |
	sv_showconnecting  = new ConVar("sv_showconnecting", "1", FCVAR_RELEASE, "Logs information about the connecting client to the console.", false, 0.f, false, 0.f, nullptr, nullptr);
	sv_pylonvisibility = new ConVar("sv_pylonvisibility", "0", FCVAR_RELEASE, "Determines the visiblity to the Pylon Master Server, 0 = Not visible, 1 = Visible, 2 = Hidden BUG BUG: not implemented yet.", false, 0.f, false, 0.f, nullptr, nullptr);

	//RconPasswordChanged_f
	sv_rcon_banpenalty  = new ConVar("sv_rcon_banpenalty", "10", FCVAR_RELEASE, "Number of minutes to ban users who fail rcon authentication.", false, 0.f, false, 0.f, nullptr, nullptr);
	sv_rcon_maxfailures = new ConVar("sv_rcon_maxfailures", "10", FCVAR_RELEASE, "Max number of times a user can fail rcon authentication before being banned.", false, 0.f, false, 0.f, nullptr, nullptr);
	sv_rcon_whitelist_address = new ConVar("sv_rcon_whitelist_address", "", FCVAR_RELEASE, "When set, rcon failed authentications will never ban this address, e.g. '127.0.0.1'.", false, 0.f, false, 0.f, nullptr, nullptr);

	//-------------------------------------------------------------------------
	// CLIENT                                                                 |
	cl_drawconsoleoverlay      = new ConVar("cl_drawconsoleoverlay", "0", FCVAR_DEVELOPMENTONLY, "Draw the console overlay at the top of the screen.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_consoleoverlay_lines    = new ConVar("cl_consoleoverlay_lines", "3", FCVAR_DEVELOPMENTONLY, "Number of lines of console output to draw.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_consoleoverlay_offset_x = new ConVar("cl_consoleoverlay_offset_x", "10", FCVAR_DEVELOPMENTONLY, "X offset for console overlay.", false, 1.f, false, 50.f, nullptr, nullptr);
	cl_consoleoverlay_offset_y = new ConVar("cl_consoleoverlay_offset_y", "10", FCVAR_DEVELOPMENTONLY, "Y offset for console overlay.", false, 1.f, false, 50.f, nullptr, nullptr);

	cl_consoleoverlay_native_clr = new ConVar("cl_consoleoverlay_native_clr", "255 255 255 255", FCVAR_DEVELOPMENTONLY, "Native RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	cl_consoleoverlay_server_clr = new ConVar("cl_consoleoverlay_server_clr", "190 183 240 255", FCVAR_DEVELOPMENTONLY, "Server script VM RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	cl_consoleoverlay_client_clr = new ConVar("cl_consoleoverlay_client_clr", "117 116 139 255", FCVAR_DEVELOPMENTONLY, "Client script VM RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	cl_consoleoverlay_ui_clr     = new ConVar("cl_consoleoverlay_ui_clr", "197 160 177 255", FCVAR_DEVELOPMENTONLY, "UI script VM RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);

	cl_showsimstats      = new ConVar("cl_showsimstats", "0", FCVAR_DEVELOPMENTONLY, "Shows the tick counter for the server/client simulation and the render frame.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_simstats_offset_x = new ConVar("cl_simstats_offset_x", "1250", FCVAR_DEVELOPMENTONLY, "X offset for simulation debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_simstats_offset_y = new ConVar("cl_simstats_offset_y", "885", FCVAR_DEVELOPMENTONLY, "Y offset for simulation debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);

	cl_showgpustats      = new ConVar("cl_showgpustats", "0", FCVAR_DEVELOPMENTONLY, "Texture streaming debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_gpustats_offset_x = new ConVar("cl_gpustats_offset_x", "1250", FCVAR_DEVELOPMENTONLY, "X offset for texture streaming debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_gpustats_offset_y = new ConVar("cl_gpustats_offset_y", "900", FCVAR_DEVELOPMENTONLY, "Y offset for texture streaming debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);

	con_max_size_logvector  = new ConVar("con_max_size_logvector", "1000", FCVAR_DEVELOPMENTONLY, "Maximum number of logs in the console until cleanup starts.", false, 0.f, false, 0.f, nullptr, nullptr);
	con_suggestion_limit    = new ConVar("con_suggestion_limit", "120", FCVAR_DEVELOPMENTONLY, "Maximum number of suggestions the autocomplete window will show for the console.", false, 0.f, false, 0.f, nullptr, nullptr);
	con_suggestion_helptext = new ConVar("con_suggestion_helptext", "0", FCVAR_DEVELOPMENTONLY, "Show ConVar help text in autocomplete window. Disabled by default to keep window less populated.", false, 0.f, false, 0.f, nullptr, nullptr);
	//-------------------------------------------------------------------------
	// FILESYSTEM                                                             |
	fs_warning_level_sdk    = new ConVar("fs_warning_level_sdk", "0", FCVAR_DEVELOPMENTONLY, "Set the SDK filesystem warning level.", false, 0.f, false, 0.f, nullptr, nullptr);
	fs_show_warning_output  = new ConVar("fs_show_warning_output", "0", FCVAR_DEVELOPMENTONLY, "Logs the filesystem warnings to the console, filtered by 'fs_warning_level_native' ( !slower! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	fs_packedstore_entryblock_stats = new ConVar("fs_packedstore_entryblock_stats", "0", FCVAR_DEVELOPMENTONLY, "If set to 1, prints the stats of each file entry in the VPK during decompression ( !slower! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	//-------------------------------------------------------------------------
	// FILESYSTEM                                                             |
	mat_showdxoutput = new ConVar("mat_showdxoutput", "0", FCVAR_DEVELOPMENTONLY, "Shows debug output for the DirectX system.", false, 0.f, false, 0.f, nullptr, nullptr);
	//-------------------------------------------------------------------------
	// SQUIRREL                                                               |
	sq_showrsonloading   = new ConVar("sq_showrsonloading", "0", FCVAR_DEVELOPMENTONLY, "Logs all 'rson' files loaded by the SQVM ( !slower! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	sq_showscriptloading = new ConVar("sq_showscriptloading", "0", FCVAR_DEVELOPMENTONLY, "Logs all scripts loaded by the SQVM to be pre-compiled ( !slower! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	sq_showvmoutput      = new ConVar("sq_showvmoutput", "0", FCVAR_DEVELOPMENTONLY, "Prints the VM output to the console. 1 = Log to file. 2 = 1 + log to console. 3 = 1 + 2 + log to overhead console. 4 = only log to overhead console.", false, 0.f, false, 0.f, nullptr, nullptr);
	sq_showvmwarning     = new ConVar("sq_showvmwarning", "0", FCVAR_DEVELOPMENTONLY, "Prints the VM warning output to the console. 1 = Log to file. 2 = 1 + log to console.", false, 0.f, false, 0.f, nullptr, nullptr);
	//-------------------------------------------------------------------------
	// NETCHANNEL                                                             |
	net_userandomkey = new ConVar("net_userandomkey", "1", FCVAR_RELEASE, "If set to 1, the netchannel generates and sets a random base64 netkey.", false, 0.f, false, 0.f, nullptr, nullptr);
	r5net_matchmaking_hostname = new ConVar("r5net_matchmaking_hostname", "r5a-comp-sv.herokuapp.com", FCVAR_RELEASE, "Holds the R5Net matchmaking hostname.", false, 0.f, false, 0.f, nullptr, nullptr);
	r5net_show_debug           = new ConVar("r5net_show_debug", "1", FCVAR_DEVELOPMENTONLY, "Shows debug output for R5Net.", false, 0.f, false, 0.f, nullptr, nullptr);
}

//-----------------------------------------------------------------------------
// Purpose: Returns the base ConVar name.
// Output : const char*
//-----------------------------------------------------------------------------
const char* ConVar::GetBaseName(void) const
{
	return m_pParent->m_ConCommandBase.m_pszName;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the ConVar help text.
// Output : const char*
//-----------------------------------------------------------------------------
const char* ConVar::GetHelpText(void) const
{
	return m_pParent->m_ConCommandBase.m_pszHelpString;
}

//-----------------------------------------------------------------------------
// Purpose: Add's flags to ConVar.
// Input  : nFlags - 
//-----------------------------------------------------------------------------
void ConVar::AddFlags(int nFlags)
{
	m_pParent->m_ConCommandBase.m_nFlags |= nFlags;
}

//-----------------------------------------------------------------------------
// Purpose: Removes flags from ConVar.
// Input  : nFlags - 
//-----------------------------------------------------------------------------
void ConVar::RemoveFlags(int nFlags)
{
	m_ConCommandBase.m_nFlags &= ~nFlags;
}

//-----------------------------------------------------------------------------
// Purpose: Checks if ConVar is registered.
// Output : bool
//-----------------------------------------------------------------------------
bool ConVar::IsRegistered(void) const
{
	return m_pParent->m_ConCommandBase.m_bRegistered;
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a boolean.
// Output : bool
//-----------------------------------------------------------------------------
bool ConVar::GetBool(void) const
{
	return !!GetInt();
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a float.
// Output : float
//-----------------------------------------------------------------------------
float ConVar::GetFloat(void) const
{
	return m_pParent->m_flValue;
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as an integer.
// Output : int
//-----------------------------------------------------------------------------
int ConVar::GetInt(void) const
{
	return m_pParent->m_iValue;
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a color.
// Output : Color
//-----------------------------------------------------------------------------
Color ConVar::GetColor(void) const
{
	unsigned char* pColorElement = ((unsigned char*)&m_pParent->m_iValue);
	return Color(pColorElement[0], pColorElement[1], pColorElement[2], pColorElement[3]);
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a string.
// Output : const char *
//-----------------------------------------------------------------------------
const char* ConVar::GetString(void) const
{
	if (m_ConCommandBase.m_nFlags & FCVAR_NEVER_AS_STRING)
	{
		return "FCVAR_NEVER_AS_STRING";
	}

	char const* str = m_pParent->m_pzsCurrentValue;
	return str ? str : "";
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flMinVal - 
// Output : true if there is a min set.
//-----------------------------------------------------------------------------
bool ConVar::GetMin(float& flMinVal) const
{
	flMinVal = m_pParent->m_flMinValue;
	return m_pParent->m_bHasMin;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flMaxVal - 
// Output : true if there is a max set.
//-----------------------------------------------------------------------------
bool ConVar::GetMax(float& flMaxVal) const
{
	flMaxVal = m_pParent->m_flMaxValue;
	return m_pParent->m_bHasMax;
}

//-----------------------------------------------------------------------------
// Purpose: returns the min value.
// Output : float
//-----------------------------------------------------------------------------
float ConVar::GetMinValue(void) const
{
	return m_pParent->m_flMinValue;
}

//-----------------------------------------------------------------------------
// Purpose: returns the max value.
// Output : float
//-----------------------------------------------------------------------------
float ConVar::GetMaxValue(void) const
{
	return m_pParent->m_flMaxValue;;
}

//-----------------------------------------------------------------------------
// Purpose: checks if ConVar has min value.
// Output : bool
//-----------------------------------------------------------------------------
bool ConVar::HasMin(void) const
{
	return m_pParent->m_bHasMin;
}

//-----------------------------------------------------------------------------
// Purpose: checks if ConVar has max value.
// Output : bool
//-----------------------------------------------------------------------------
bool ConVar::HasMax(void) const
{
	return m_pParent->m_bHasMax;
}

//-----------------------------------------------------------------------------
// Purpose: sets the ConVar int value.
// Input  : nValue - 
//-----------------------------------------------------------------------------
void ConVar::SetValue(int nValue)
{
	if (nValue == m_iValue)
	{
		return;
	}

	// Only valid for root ConVars.
	assert(m_pParent == this);

	float flValue = (float)nValue;

	// Check bounds.
	if (ClampValue(flValue))
	{
		nValue = (int)(flValue);
	}

	// Redetermine value.
	float flOldValue = m_flValue;
	m_flValue = flValue;
	m_iValue = nValue;

	if (!(m_ConCommandBase.m_nFlags & FCVAR_NEVER_AS_STRING))
	{
		char szTempValue[32];
		snprintf(szTempValue, sizeof(szTempValue), "%d", m_iValue);
		ChangeStringValue(szTempValue, flOldValue);
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets the ConVar float value.
// Input  : flValue - 
//-----------------------------------------------------------------------------
void ConVar::SetValue(float flValue)
{
	if (flValue == m_flValue)
	{
		return;
	}

	// Only valid for root ConVars.
	assert(m_pParent == this);

	// Check bounds.
	ClampValue(flValue);

	// Redetermine value.
	float flOldValue = m_flValue;
	m_flValue = flValue;
	m_iValue = (int)m_flValue;

	if (!(m_ConCommandBase.m_nFlags & FCVAR_NEVER_AS_STRING))
	{
		char szTempValue[32];
		snprintf(szTempValue, sizeof(szTempValue), "%f", m_flValue);
		ChangeStringValue(szTempValue, flOldValue);
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets the ConVar string value.
// Input  : *szValue - 
//-----------------------------------------------------------------------------
void ConVar::SetValue(const char* pszValue)
{
	if (strcmp(this->m_pParent->m_pzsCurrentValue, pszValue) == 0)
	{
		return;
	}
	this->m_pParent->m_pzsCurrentValue = pszValue;

	char szTempValue[32]{};
	const char* pszNewValue{};

	// Only valid for root convars.
	assert(m_pParent == this);

	float flOldValue = m_flValue;
	pszNewValue = (char*)pszValue;
	if (!pszNewValue)
	{
		pszNewValue = "";
	}

	if (!SetColorFromString(pszValue))
	{
		// Not a color, do the standard thing
		float flNewValue = (float)atof(pszValue);
		if (!IsFinite(flNewValue))
		{
			DevMsg(eDLL_T::ENGINE ,"Warning: ConVar '%s' = '%s' is infinite, clamping value.\n", GetBaseName(), pszValue);
			flNewValue = FLT_MAX;
		}

		if (ClampValue(flNewValue))
		{
			snprintf(szTempValue, sizeof(szTempValue), "%f", flNewValue);
			pszNewValue = szTempValue;
		}

		// Redetermine value
		m_flValue = flNewValue;
		m_iValue = (int)(m_flValue);
	}

	if (!(m_ConCommandBase.m_nFlags & FCVAR_NEVER_AS_STRING))
	{
		ChangeStringValue(pszNewValue, flOldValue);
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets the ConVar color value.
// Input  : clValue - 
//-----------------------------------------------------------------------------
void ConVar::SetValue(Color clValue)
{
	std::string svResult = "";

	for (int i = 0; i < 4; i++)
	{
		if (!(clValue._color[i] == 0 && svResult.size() == 0))
		{
			svResult += std::to_string(clValue._color[i]);
			svResult.append(" ");
		}
	}

	this->m_pParent->m_pzsCurrentValue = svResult.c_str();
}


//-----------------------------------------------------------------------------
// Purpose: Reset to default value.
//-----------------------------------------------------------------------------
void ConVar::Revert(void)
{
	this->SetValue(this->m_pszDefaultValue);
}

//-----------------------------------------------------------------------------
// Purpose: returns the default ConVar value.
// Output : const char
//-----------------------------------------------------------------------------
const char* ConVar::GetDefault(void) const
{
	return m_pParent->m_pszDefaultValue;
}

//-----------------------------------------------------------------------------
// Purpose: sets the default ConVar value.
// Input  : *pszDefault -
//-----------------------------------------------------------------------------
void ConVar::SetDefault(const char* pszDefault)
{
	static const char* pszEmpty = "";
	m_pszDefaultValue = pszDefault ? pszDefault : pszEmpty;
	assert(m_pszDefaultValue);
}

//-----------------------------------------------------------------------------
// Purpose: changes the ConVar string value.
// Input  : *pszTempVal - flOldValue
//-----------------------------------------------------------------------------
void ConVar::ChangeStringValue(const char* pszTempVal, float flOldValue)
{
	assert(!(m_ConCommandBase.m_nFlags & FCVAR_NEVER_AS_STRING));

	char* pszOldValue = (char*)_malloca(m_iStringLength);
	if (pszOldValue != NULL)
	{
		memcpy(pszOldValue, m_pzsCurrentValue, m_iStringLength);
	}

	if (pszTempVal)
	{
		int len = strlen(pszTempVal) + 1;

		if (len > m_iStringLength)
		{
			if (m_pzsCurrentValue)
			{
				delete[] m_pzsCurrentValue;
			}

			m_pzsCurrentValue = new char[len];
			m_iStringLength = len;
		}

		memcpy((char*)m_pzsCurrentValue, pszTempVal, len);
	}
	else
	{
		m_pzsCurrentValue = NULL;
	}

	pszOldValue = 0;
}

//-----------------------------------------------------------------------------
// Purpose: sets the ConVar color value from string.
// Input  : *pszValue - 
//-----------------------------------------------------------------------------
bool ConVar::SetColorFromString(const char* pszValue)
{
	bool bColor = false;

	// Try pulling RGBA color values out of the string.
	int nRGBA[4]{};
	int nParamsRead = sscanf_s(pszValue, "%i %i %i %i", &(nRGBA[0]), &(nRGBA[1]), &(nRGBA[2]), &(nRGBA[3]));

	if (nParamsRead >= 3)
	{
		// This is probably a color!
		if (nParamsRead == 3)
		{
			// Assume they wanted full alpha.
			nRGBA[3] = 255;
		}

		if (nRGBA[0] >= 0 && nRGBA[0] <= 255 &&
			nRGBA[1] >= 0 && nRGBA[1] <= 255 &&
			nRGBA[2] >= 0 && nRGBA[2] <= 255 &&
			nRGBA[3] >= 0 && nRGBA[3] <= 255)
		{
			//printf("*** WOW! Found a color!! ***\n");

			// This is definitely a color!
			bColor = true;

			// Stuff all the values into each byte of our int.
			unsigned char* pColorElement = ((unsigned char*)&m_iValue);
			pColorElement[0] = nRGBA[0];
			pColorElement[1] = nRGBA[1];
			pColorElement[2] = nRGBA[2];
			pColorElement[3] = nRGBA[3];

			// Copy that value into our float.
			m_flValue = (float)(m_iValue);
		}
	}

	return bColor;
}

//-----------------------------------------------------------------------------
// Purpose: Check whether to clamp and then perform clamp.
// Input  : flValue - 
// Output : Returns true if value changed.
//-----------------------------------------------------------------------------
bool ConVar::ClampValue(float& flValue)
{
	if (m_bHasMin && (flValue < m_flMinValue))
	{
		flValue = m_flMinValue;
		return true;
	}

	if (m_bHasMax && (flValue > m_flMaxValue))
	{
		flValue = m_flMaxValue;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Test each ConVar query before setting the value.
// Input  : *pConVar - nFlags
// Output : False if change is permitted, true if not.
//-----------------------------------------------------------------------------
bool ConVar::IsFlagSet(ConVar* pConVar, int nFlags)
{
	if (cm_debug_cmdquery->GetBool())
	{
		printf("--------------------------------------------------\n");
		printf(" Flaged: %08X\n", pConVar->m_ConCommandBase.m_nFlags);
	}
	if (cm_return_false_cmdquery_cheats->GetBool())
	{
		// Mask off FCVAR_CHEATS and FCVAR_DEVELOPMENTONLY.
		pConVar->m_ConCommandBase.RemoveFlags(FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT);
	}
	else // Mask off FCVAR_DEVELOPMENTONLY.
	{
		pConVar->m_ConCommandBase.RemoveFlags(FCVAR_DEVELOPMENTONLY);
	}
	if (cm_debug_cmdquery->GetBool())
	{
		printf(" Masked: %08X\n", pConVar->m_ConCommandBase.m_nFlags);
		printf(" Verify: %08X\n", nFlags);
		printf("--------------------------------------------------\n");
	}
	if (nFlags & FCVAR_RELEASE && !cm_return_false_cmdquery_all->GetBool())
	{
		// Default retail behaviour.
		return IConVar_IsFlagSet(pConVar, nFlags);
	}
	if (cm_return_false_cmdquery_all->GetBool())
	{
		// Returning false on all queries may cause problems.
		return false;
	}
	// Return false on every FCVAR_DEVELOPMENTONLY query.
	return pConVar->m_ConCommandBase.HasFlags(nFlags) != 0;
}

//-----------------------------------------------------------------------------
// Purpose: clear all hostname ConVar's.
//-----------------------------------------------------------------------------
void ConVar::ClearHostNames(void)
{
	const char* pszHostnameArray[] =
	{
		"pin_telemetry_hostname",
		"assetdownloads_hostname",
		"users_hostname",
		"persistence_hostname",
		"speechtotexttoken_hostname",
		"communities_hostname",
		"persistenceDef_hostname",
		"party_hostname",
		"speechtotext_hostname",
		"serverReports_hostname",
		"subscription_hostname",
		"steamlink_hostname",
		"staticfile_hostname",
		"matchmaking_hostname",
		"skill_hostname",
		"publication_hostname",
		"stats_hostname"
	};

	for (int i = 0; i < (&pszHostnameArray)[1] - pszHostnameArray; i++)
	{
		const char* pszName = pszHostnameArray[i];
		ConVar* pCVar = g_pCVar->FindVar(pszName);

		if (pCVar != nullptr)
		{
			pCVar->m_pzsCurrentValue = "0.0.0.0";
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
void IConVar_Attach()
{
	DetourAttach((LPVOID*)&IConVar_IsFlagSet, &ConVar::IsFlagSet);
}

void IConVar_Detach()
{
	DetourDetach((LPVOID*)&IConVar_IsFlagSet, &ConVar::IsFlagSet);
}

///////////////////////////////////////////////////////////////////////////////
ConVar* g_pConVar = new ConVar();
