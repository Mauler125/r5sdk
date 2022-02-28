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
// Purpose: construct/allocate
//-----------------------------------------------------------------------------
ConVar::ConVar(const char* pszName, const char* pszDefaultValue, int nFlags, const char* pszHelpString, bool bMin, float fMin, bool bMax, float fMax, void* pCallback, const char* pszUsageString)
{
	ConVar* pNewConVar = reinterpret_cast<ConVar*>(MemAlloc_Wrapper(sizeof(ConVar))); // Allocate new memory with StdMemAlloc else we crash.
	memset(pNewConVar, '\0', sizeof(ConVar));                                         // Set all to null.

	pNewConVar->m_pConCommandBaseVTable = g_pConVarVtable.RCast<void*>();
	pNewConVar->m_pIConVarVTable = g_pIConVarVtable.RCast<void*>();

	ConVar_Register(pNewConVar, pszName, pszDefaultValue, nFlags, pszHelpString, bMin, fMin, bMax, fMax, pCallback, pszUsageString);
	*this = *pNewConVar;
}

//-----------------------------------------------------------------------------
// Purpose: destructor
//-----------------------------------------------------------------------------
ConVar::~ConVar(void)
{
	if (m_Value.m_pszString)
	{
		delete[] m_Value.m_pszString;
		m_Value.m_pszString = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: register ConVar
//-----------------------------------------------------------------------------
void ConVar::Init(void) const
{
	//-------------------------------------------------------------------------
	// ENGINE                                                                 |
	cm_debug_cmdquery               = new ConVar("cm_debug_cmdquery"              , "0", FCVAR_DEVELOPMENTONLY, "Prints the flags of each ConVar/ConCommand query to the console ( !slower! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	cm_return_false_cmdquery_all    = new ConVar("cm_return_false_cmdquery_all"   , "0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Returns false on every ConVar/ConCommand query ( !warning! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	cm_return_false_cmdquery_cheats = new ConVar("cm_return_false_cmdquery_cheats", "0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Returns false on all FCVAR_DEVELOPMENTONLY and FCVAR_CHEAT ConVar/ConCommand queries ( !warning! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	r_debug_overlay_nodecay         = new ConVar("r_debug_overlay_nodecay"        , "0", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT     , "Keeps all debug overlays alive regardless of their lifetime. Use command 'clear_debug_overlays' to clear everything.", false, 0.f, false, 0.f, nullptr, nullptr);

	// TODO: RconPasswordChanged_f
	rcon_address  = new ConVar("rcon_address",  "::", FCVAR_SERVER_CANNOT_QUERY | FCVAR_DONTRECORD | FCVAR_RELEASE, "Remote server access address.", false, 0.f, false, 0.f, nullptr, nullptr);
	rcon_password = new ConVar("rcon_password", ""  , FCVAR_SERVER_CANNOT_QUERY | FCVAR_DONTRECORD | FCVAR_RELEASE, "Remote server access password (rcon is disabled if empty).", false, 0.f, false, 0.f, nullptr, nullptr);
	//-------------------------------------------------------------------------
	// SERVER                                                                 |
	ai_dumpAINfileFromLoad = new ConVar("ai_dumpAINfileFromLoad" , "0", FCVAR_DEVELOPMENTONLY, "Dumps AIN data from node graphs loaded from the disk on load.", false, 0.f, false, 0.f, nullptr, nullptr);

	sv_showconnecting  = new ConVar("sv_showconnecting" , "1", FCVAR_RELEASE, "Logs information about the connecting client to the console.", false, 0.f, false, 0.f, nullptr, nullptr);
	sv_pylonvisibility = new ConVar("sv_pylonvisibility", "0", FCVAR_RELEASE, "Determines the visiblity to the Pylon Master Server, 0 = Not visible, 1 = Visible, 2 = Hidden !TODO: not implemented yet.", false, 0.f, false, 0.f, nullptr, nullptr);

#ifdef DEDICATED
	sv_rcon_debug       = new ConVar("sv_rcon_debug"      , "0" , FCVAR_RELEASE, "Show rcon debug information ( !slower! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	sv_rcon_banpenalty  = new ConVar("sv_rcon_banpenalty" , "10", FCVAR_RELEASE, "Number of minutes to ban users who fail rcon authentication.", false, 0.f, false, 0.f, nullptr, nullptr);
	sv_rcon_maxfailures = new ConVar("sv_rcon_maxfailures", "10", FCVAR_RELEASE, "Max number of times a user can fail rcon authentication before being banned.", false, 0.f, false, 0.f, nullptr, nullptr);
	sv_rcon_maxignores  = new ConVar("sv_rcon_maxignores" , "15", FCVAR_RELEASE, "Max number of times a user can ignore the no-auth message before being banned.", false, 0.f, false, 0.f, nullptr, nullptr);
	sv_rcon_maxsockets  = new ConVar("sv_rcon_maxsockets" , "32", FCVAR_RELEASE, "Max number of accepted sockets before the server starts closing redundant sockets.", false, 0.f, false, 0.f, nullptr, nullptr);
	sv_rcon_whitelist_address = new ConVar("sv_rcon_whitelist_address", "", FCVAR_RELEASE, "This address is not considered a 'redundant' socket and will never be banned for failed authentications. Example: '::ffff:127.0.0.1'.", false, 0.f, false, 0.f, nullptr, nullptr);
#endif // DEDICATED
	//-------------------------------------------------------------------------
	// CLIENT                                                                 |
#ifndef DEDICATED
	cl_drawconsoleoverlay           = new ConVar("cl_drawconsoleoverlay"          , "0" , FCVAR_DEVELOPMENTONLY, "Draws the RUI console overlay at the top of the screen.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_consoleoverlay_lines         = new ConVar("cl_consoleoverlay_lines"        , "3" , FCVAR_DEVELOPMENTONLY, "Number of lines of console output to draw.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_consoleoverlay_invert_rect_x = new ConVar("cl_consoleoverlay_invert_rect_x", "0" , FCVAR_DEVELOPMENTONLY, "Inverts the X rect for RUI console overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_consoleoverlay_invert_rect_y = new ConVar("cl_consoleoverlay_invert_rect_y", "0" , FCVAR_DEVELOPMENTONLY, "Inverts the Y rect for RUI console overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_consoleoverlay_offset_x      = new ConVar("cl_consoleoverlay_offset_x"     , "10", FCVAR_DEVELOPMENTONLY, "X offset for RUI console overlay.", false, 1.f, false, 50.f, nullptr, nullptr);
	cl_consoleoverlay_offset_y      = new ConVar("cl_consoleoverlay_offset_y"     , "10", FCVAR_DEVELOPMENTONLY, "Y offset for RUI console overlay.", false, 1.f, false, 50.f, nullptr, nullptr);

	cl_conoverlay_script_server_clr  = new ConVar("cl_conoverlay_script_server_clr", "130 120 245 255", FCVAR_DEVELOPMENTONLY, "Script SERVER VM RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	cl_conoverlay_script_client_clr  = new ConVar("cl_conoverlay_script_client_clr", "117 116 139 255", FCVAR_DEVELOPMENTONLY, "Script CLIENT VM RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	cl_conoverlay_script_ui_clr      = new ConVar("cl_conoverlay_script_ui_clr    ", "200 110 110 255", FCVAR_DEVELOPMENTONLY, "Script UI VM RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);

	cl_conoverlay_native_server_clr = new ConVar("cl_conoverlay_native_server_clr", "020 050 248 255", FCVAR_DEVELOPMENTONLY, "Native SERVER RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	cl_conoverlay_native_client_clr = new ConVar("cl_conoverlay_native_client_clr", "070 070 070 255", FCVAR_DEVELOPMENTONLY, "Native CLIENT RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	cl_conoverlay_native_ui_clr     = new ConVar("cl_conoverlay_native_ui_clr"    , "200 060 060 255", FCVAR_DEVELOPMENTONLY, "Native UI RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	cl_conoverlay_native_engine_clr = new ConVar("cl_conoverlay_native_engine_clr", "255 255 255 255", FCVAR_DEVELOPMENTONLY, "Native engine RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	cl_conoverlay_native_fs_clr     = new ConVar("cl_conoverlay_native_fs_clr"    , "000 100 225 255", FCVAR_DEVELOPMENTONLY, "Native filesystem RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	cl_conoverlay_native_rtech_clr  = new ConVar("cl_conoverlay_native_rtech_clr" , "025 100 100 255", FCVAR_DEVELOPMENTONLY, "Native rtech RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	cl_conoverlay_native_ms_clr     = new ConVar("cl_conoverlay_native_ms_clr"    , "200 020 180 255", FCVAR_DEVELOPMENTONLY, "Native materialsystem RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);

	cl_conoverlay_netcon_clr  = new ConVar("cl_conoverlay_netcon_clr" , "255 255 255 255", FCVAR_DEVELOPMENTONLY, "Net console RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	cl_conoverlay_warning_clr = new ConVar("cl_conoverlay_warning_clr", "180 180 020 255", FCVAR_DEVELOPMENTONLY, "Warning RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	cl_conoverlay_error_clr   = new ConVar("cl_conoverlay_error_clr"  , "225 050 050 255", FCVAR_DEVELOPMENTONLY, "Error RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);

	cl_showhoststats           = new ConVar("cl_showhoststats"          , "0", FCVAR_DEVELOPMENTONLY, "Host speeds debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_hoststats_invert_rect_x = new ConVar("cl_hoststats_invert_rect_x", "0", FCVAR_DEVELOPMENTONLY, "Inverts the X rect for host speeds debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_hoststats_invert_rect_y = new ConVar("cl_hoststats_invert_rect_y", "0", FCVAR_DEVELOPMENTONLY, "Inverts the Y rect for host speeds debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_hoststats_offset_x      = new ConVar("cl_hoststats_offset_x"    , "10", FCVAR_DEVELOPMENTONLY, "X offset for host speeds debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_hoststats_offset_y      = new ConVar("cl_hoststats_offset_y"    , "10", FCVAR_DEVELOPMENTONLY, "Y offset for host speeds debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);

	cl_showsimstats           = new ConVar("cl_showsimstats"          , "0"  , FCVAR_DEVELOPMENTONLY, "Shows the tick counter for the server/client simulation and the render frame.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_simstats_invert_rect_x = new ConVar("cl_simstats_invert_rect_x", "1"  , FCVAR_DEVELOPMENTONLY, "Inverts the X rect for simulation debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_simstats_invert_rect_y = new ConVar("cl_simstats_invert_rect_y", "1"  , FCVAR_DEVELOPMENTONLY, "Inverts the Y rect for simulation debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_simstats_offset_x      = new ConVar("cl_simstats_offset_x"     , "650", FCVAR_DEVELOPMENTONLY, "X offset for simulation debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_simstats_offset_y      = new ConVar("cl_simstats_offset_y"     , "120", FCVAR_DEVELOPMENTONLY, "Y offset for simulation debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);

	cl_showgpustats           = new ConVar("cl_showgpustats"            , "0", FCVAR_DEVELOPMENTONLY, "Texture streaming debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_gpustats_invert_rect_x = new ConVar("cl_gpustats_invert_rect_x"  , "1", FCVAR_DEVELOPMENTONLY, "Inverts the X rect for texture streaming debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_gpustats_invert_rect_y = new ConVar("cl_gpustats_invert_rect_y"  , "1", FCVAR_DEVELOPMENTONLY, "Inverts the Y rect for texture streaming debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_gpustats_offset_x      = new ConVar("cl_gpustats_offset_x"     , "650", FCVAR_DEVELOPMENTONLY, "X offset for texture streaming debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_gpustats_offset_y      = new ConVar("cl_gpustats_offset_y"     , "105", FCVAR_DEVELOPMENTONLY, "Y offset for texture streaming debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);

	con_max_size_logvector  = new ConVar("con_max_size_logvector", "1000", FCVAR_DEVELOPMENTONLY, "Maximum number of logs in the console until cleanup starts.", false, 0.f, false, 0.f, nullptr, nullptr);
	con_suggestion_limit    = new ConVar("con_suggestion_limit"  , "120" , FCVAR_DEVELOPMENTONLY, "Maximum number of suggestions the autocomplete window will show for the console.", false, 0.f, false, 0.f, nullptr, nullptr);
	con_suggestion_helptext = new ConVar("con_suggestion_helptext", "1"  , FCVAR_DEVELOPMENTONLY, "Show ConVar help text in autocomplete window.", false, 0.f, false, 0.f, nullptr, nullptr);
#endif // !DEDICATED
	//-------------------------------------------------------------------------
	// FILESYSTEM                                                             |
	fs_warning_level_sdk            = new ConVar("fs_warning_level_sdk"           , "0", FCVAR_DEVELOPMENTONLY, "Set the SDK filesystem warning level.", false, 0.f, false, 0.f, nullptr, nullptr);
	fs_show_warning_output          = new ConVar("fs_show_warning_output"         , "0", FCVAR_DEVELOPMENTONLY, "Logs the filesystem warnings to the console, filtered by 'fs_warning_level_native' ( !slower! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	fs_packedstore_entryblock_stats = new ConVar("fs_packedstore_entryblock_stats", "0", FCVAR_DEVELOPMENTONLY, "If set to 1, prints the stats of each file entry in the VPK during decompression ( !slower! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	//-------------------------------------------------------------------------
	// MATERIALSYSTEM                                                         |
#ifndef DEDICATED
	mat_showdxoutput = new ConVar("mat_showdxoutput", "0", FCVAR_DEVELOPMENTONLY, "Shows debug output for the DirectX system.", false, 0.f, false, 0.f, nullptr, nullptr);
#endif // !DEDICATED
	//-------------------------------------------------------------------------
	// SQUIRREL                                                               |
	sq_showrsonloading   = new ConVar("sq_showrsonloading"  , "0", FCVAR_DEVELOPMENTONLY, "Logs all 'rson' files loaded by the SQVM ( !slower! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	sq_showscriptloading = new ConVar("sq_showscriptloading", "0", FCVAR_DEVELOPMENTONLY, "Logs all scripts loaded by the SQVM to be pre-compiled ( !slower! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	sq_showvmoutput      = new ConVar("sq_showvmoutput"     , "0", FCVAR_DEVELOPMENTONLY, "Prints the VM output to the console. 1 = Log to file. 2 = 1 + log to console. 3 = 1 + 2 + log to overhead console. 4 = only log to overhead console.", false, 0.f, false, 0.f, nullptr, nullptr);
	sq_showvmwarning     = new ConVar("sq_showvmwarning"    , "0", FCVAR_DEVELOPMENTONLY, "Prints the VM warning output to the console. 1 = Log to file. 2 = 1 + log to console.", false, 0.f, false, 0.f, nullptr, nullptr);
	//-------------------------------------------------------------------------
	// NETCHANNEL                                                             |
	net_userandomkey           = new ConVar("net_userandomkey"          , "1"                        , FCVAR_RELEASE        , "If set to 1, the netchannel generates and sets a random base64 netkey.", false, 0.f, false, 0.f, nullptr, nullptr);
	r5net_matchmaking_hostname = new ConVar("r5net_matchmaking_hostname", "r5a-comp-sv.herokuapp.com", FCVAR_RELEASE        , "Holds the R5Net matchmaking hostname.", false, 0.f, false, 0.f, nullptr, nullptr);
	r5net_show_debug           = new ConVar("r5net_show_debug"          , "1"                        , FCVAR_DEVELOPMENTONLY, "Shows debug output for R5Net.", false, 0.f, false, 0.f, nullptr, nullptr);
}

//-----------------------------------------------------------------------------
// Purpose: Add's flags to ConVar.
// Input  : nFlags - 
//-----------------------------------------------------------------------------
void ConVar::AddFlags(int nFlags)
{
	m_pParent->m_nFlags |= nFlags;
}

//-----------------------------------------------------------------------------
// Purpose: Removes flags from ConVar.
// Input  : nFlags - 
//-----------------------------------------------------------------------------
void ConVar::RemoveFlags(int nFlags)
{
	m_nFlags &= ~nFlags;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the base ConVar name.
// Output : const char*
//-----------------------------------------------------------------------------
const char* ConVar::GetBaseName(void) const
{
	return m_pParent->m_pszName;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the ConVar help text.
// Output : const char*
//-----------------------------------------------------------------------------
const char* ConVar::GetHelpText(void) const
{
	return m_pParent->m_pszHelpString;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the ConVar usage text.
// Output : const char*
//-----------------------------------------------------------------------------
const char* ConVar::GetUsageText(void) const
{
	return m_pParent->m_pszUsageString;
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
	return m_pParent->m_Value.m_fValue;
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as an integer.
// Output : int
//-----------------------------------------------------------------------------
int ConVar::GetInt(void) const
{
	return m_pParent->m_Value.m_nValue;
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a color.
// Output : Color
//-----------------------------------------------------------------------------
Color ConVar::GetColor(void) const
{
	unsigned char* pColorElement = ((unsigned char*)&m_pParent->m_Value.m_nValue);
	return Color(pColorElement[0], pColorElement[1], pColorElement[2], pColorElement[3]);
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a string.
// Output : const char *
//-----------------------------------------------------------------------------
const char* ConVar::GetString(void) const
{
	if (m_nFlags & FCVAR_NEVER_AS_STRING)
	{
		return "FCVAR_NEVER_AS_STRING";
	}

	char const* str = m_pParent->m_Value.m_pszString;
	return str ? str : "";
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flMinVal - 
// Output : true if there is a min set.
//-----------------------------------------------------------------------------
bool ConVar::GetMin(float& flMinVal) const
{
	flMinVal = m_pParent->m_fMinVal;
	return m_pParent->m_bHasMin;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flMaxVal - 
// Output : true if there is a max set.
//-----------------------------------------------------------------------------
bool ConVar::GetMax(float& flMaxVal) const
{
	flMaxVal = m_pParent->m_fMaxVal;
	return m_pParent->m_bHasMax;
}

//-----------------------------------------------------------------------------
// Purpose: returns the min value.
// Output : float
//-----------------------------------------------------------------------------
float ConVar::GetMinValue(void) const
{
	return m_pParent->m_fMinVal;
}

//-----------------------------------------------------------------------------
// Purpose: returns the max value.
// Output : float
//-----------------------------------------------------------------------------
float ConVar::GetMaxValue(void) const
{
	return m_pParent->m_fMaxVal;
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
	if (nValue == m_Value.m_nValue)
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
	float flOldValue = m_Value.m_fValue;
	m_Value.m_fValue = flValue;
	m_Value.m_fValue = nValue;

	if (!(m_nFlags & FCVAR_NEVER_AS_STRING))
	{
		char szTempValue[32];
		snprintf(szTempValue, sizeof(szTempValue), "%d", m_Value.m_nValue);
		ChangeStringValue(szTempValue, flOldValue);
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets the ConVar float value.
// Input  : flValue - 
//-----------------------------------------------------------------------------
void ConVar::SetValue(float flValue)
{
	if (flValue == m_Value.m_fValue)
	{
		return;
	}

	// Only valid for root ConVars.
	assert(m_pParent == this);

	// Check bounds.
	ClampValue(flValue);

	// Redetermine value.
	float flOldValue = m_Value.m_fValue;
	m_Value.m_fValue = flValue;
	m_Value.m_nValue = (int)m_Value.m_fValue;

	if (!(m_nFlags & FCVAR_NEVER_AS_STRING))
	{
		char szTempValue[32];
		snprintf(szTempValue, sizeof(szTempValue), "%f", m_Value.m_fValue);
		ChangeStringValue(szTempValue, flOldValue);
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets the ConVar string value.
// Input  : *szValue - 
//-----------------------------------------------------------------------------
void ConVar::SetValue(const char* pszValue)
{
	if (strcmp(this->m_pParent->m_Value.m_pszString, pszValue) == 0)
	{
		return;
	}
	this->m_pParent->m_Value.m_pszString = pszValue;

	char szTempValue[32]{};
	const char* pszNewValue{};

	// Only valid for root convars.
	assert(m_pParent == this);

	float flOldValue = m_Value.m_fValue;
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
		m_Value.m_fValue = flNewValue;
		m_Value.m_nValue = (int)(m_Value.m_fValue);
	}

	if (!(m_nFlags & FCVAR_NEVER_AS_STRING))
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
		if (!(clValue.GetValue(i) == 0 && svResult.size() == 0))
		{
			svResult += std::to_string(clValue.GetValue(i));
			svResult.append(" ");
		}
	}

	this->m_pParent->m_Value.m_pszString = svResult.c_str();
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
	assert(!(m_nFlags & FCVAR_NEVER_AS_STRING));

	char* pszOldValue = (char*)_malloca(m_Value.m_iStringLength);
	if (pszOldValue != NULL)
	{
		memcpy(pszOldValue, m_Value.m_pszString, m_Value.m_iStringLength);
	}

	if (pszTempVal)
	{
		int len = strlen(pszTempVal) + 1;

		if (len > m_Value.m_iStringLength)
		{
			if (m_Value.m_pszString)
			{
				delete[] m_Value.m_pszString;
			}

			m_Value.m_pszString = new char[len];
			m_Value.m_iStringLength = len;
		}

		memcpy((char*)m_Value.m_pszString, pszTempVal, len);
	}
	else
	{
		m_Value.m_pszString = NULL;
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
			unsigned char* pColorElement = ((unsigned char*)&m_Value.m_nValue);
			pColorElement[0] = nRGBA[0];
			pColorElement[1] = nRGBA[1];
			pColorElement[2] = nRGBA[2];
			pColorElement[3] = nRGBA[3];

			// Copy that value into our float.
			m_Value.m_fValue = (float)(m_Value.m_nValue);
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
	if (m_bHasMin && (flValue < m_fMinVal))
	{
		flValue = m_fMinVal;
		return true;
	}

	if (m_bHasMax && (flValue > m_fMaxVal))
	{
		flValue = m_fMaxVal;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Checks if ConVar is registered.
// Output : bool
//-----------------------------------------------------------------------------
bool ConVar::IsRegistered(void) const
{
	return m_pParent->m_bRegistered;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if this is a command 
// Output : bool
//-----------------------------------------------------------------------------
bool ConVar::IsCommand(void) const
{
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
		printf(" Flaged: %08X\n", pConVar->m_nFlags);
	}
	if (cm_return_false_cmdquery_cheats->GetBool())
	{
		// Mask off FCVAR_CHEATS and FCVAR_DEVELOPMENTONLY.
		pConVar->RemoveFlags(FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT);
	}
	else // Mask off FCVAR_DEVELOPMENTONLY.
	{
		pConVar->RemoveFlags(FCVAR_DEVELOPMENTONLY);
	}
	if (cm_debug_cmdquery->GetBool())
	{
		printf(" Masked: %08X\n", pConVar->m_nFlags);
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
	return pConVar->HasFlags(nFlags) != 0;
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
			pCVar->m_Value.m_pszString = "0.0.0.0";
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
