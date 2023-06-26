#include "tier1/utlrbtree.h"
#include "tier1/NetAdr.h"
#include "tier1/cvar.h"
#include "public/const.h"
#include "engine/sys_dll2.h"
#include "filesystem/filesystem.h"
#include "vstdlib/concommandhash.h"

//-----------------------------------------------------------------------------
// Purpose: create
//-----------------------------------------------------------------------------
ConVar* ConVar::StaticCreate(const char* pszName, const char* pszDefaultValue,
	int nFlags, const char* pszHelpString, bool bMin, float fMin, bool bMax,
	float fMax, FnChangeCallback_t pCallback, const char* pszUsageString)
{
	ConVar* pNewConVar = (ConVar*)malloc(sizeof(ConVar));

	pNewConVar->m_bRegistered = false;
	*(ConVar**)pNewConVar = g_pConVarVBTable;
	char* pConVarVFTable = (char*)pNewConVar + sizeof(ConCommandBase);
	*(IConVar**)pConVarVFTable = g_pConVarVFTable;

	pNewConVar->m_pszName = nullptr;
	pNewConVar->m_pszHelpString = nullptr;
	pNewConVar->m_pszUsageString = nullptr;
	pNewConVar->s_pAccessor = nullptr;
	pNewConVar->m_nFlags = FCVAR_NONE;
	pNewConVar->m_pNext = nullptr;

	pNewConVar->m_fnChangeCallbacks.Init();

	v_ConVar_Register(pNewConVar, pszName, pszDefaultValue, nFlags,
		pszHelpString, bMin, fMin, bMax, fMax, pCallback, pszUsageString);
	return pNewConVar;
}

//-----------------------------------------------------------------------------
// Purpose: destroy
//-----------------------------------------------------------------------------
void ConVar::Destroy(void)
{
	v_ConVar_Unregister(this);
}

//-----------------------------------------------------------------------------
// Purpose: construct/allocate
//-----------------------------------------------------------------------------
ConVar::ConVar(void)
	: m_pParent(nullptr)
	, m_pszDefaultValue(nullptr)
	, m_bHasMin(false)
	, m_fMinVal(0.f)
	, m_bHasMax(false)
	, m_fMaxVal(0.f)
{
	m_Value.m_pszString = nullptr;
	m_Value.m_iStringLength = 0;
	m_Value.m_fValue = 0.0f;
	m_Value.m_nValue = 0;
}

//-----------------------------------------------------------------------------
// Purpose: destructor
//-----------------------------------------------------------------------------
//ConVar::~ConVar(void)
//{
//	if (m_Value.m_pszString)
//	{
//		delete[] m_Value.m_pszString);
//		m_Value.m_pszString = NULL;
//	}
//}

////-----------------------------------------------------------------------------
//// Purpose: Returns the base ConVar name.
//// Output : const char*
////-----------------------------------------------------------------------------
//const char* ConVar::GetBaseName(void) const
//{
//	return m_pParent->m_pszName;
//}
//
////-----------------------------------------------------------------------------
//// Purpose: Returns the ConVar help text.
//// Output : const char*
////-----------------------------------------------------------------------------
//const char* ConVar::GetHelpText(void) const
//{
//	return m_pParent->m_pszHelpString;
//}
//
////-----------------------------------------------------------------------------
//// Purpose: Returns the ConVar usage text.
//// Output : const char*
////-----------------------------------------------------------------------------
//const char* ConVar::GetUsageText(void) const
//{
//	return m_pParent->m_pszUsageString;
//}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flMaxVal - 
//-----------------------------------------------------------------------------
void ConVar::SetMax(float flMaxVal)
{
	m_pParent->m_fMaxVal = flMaxVal;
	m_pParent->m_bHasMax = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flMinVal - 
//-----------------------------------------------------------------------------
void ConVar::SetMin(float flMinVal)
{
	m_pParent->m_fMinVal = flMinVal;
	m_pParent->m_bHasMin = true;
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
	ConVar* pCVar = reinterpret_cast<ConVar*>(m_pParent);
	pCVar->InternalSetIntValue(nValue);
}

//-----------------------------------------------------------------------------
// Purpose: sets the ConVar float value.
// Input  : flValue - 
//-----------------------------------------------------------------------------
void ConVar::SetValue(float flValue)
{
	ConVar* pCVar = reinterpret_cast<ConVar*>(m_pParent);
	pCVar->InternalSetFloatValue(flValue);
}

//-----------------------------------------------------------------------------
// Purpose: sets the ConVar string value.
// Input  : *szValue - 
//-----------------------------------------------------------------------------
void ConVar::SetValue(const char* pszValue)
{
	ConVar* pCVar = reinterpret_cast<ConVar*>(m_pParent);
	pCVar->InternalSetValue(pszValue);
}

//-----------------------------------------------------------------------------
// Purpose: sets the ConVar color value.
// Input  : value - 
//-----------------------------------------------------------------------------
void ConVar::SetValue(Color value)
{
	ConVar* pCVar = reinterpret_cast<ConVar*>(m_pParent);
	pCVar->InternalSetColorValue(value);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *value - 
//-----------------------------------------------------------------------------
void ConVar::InternalSetColorValue(Color value)
{
	// Stuff color values into an int
	int nValue = 0;

	unsigned char* pColorElement = (reinterpret_cast<unsigned char*>(&nValue));
	pColorElement[0] = value[0];
	pColorElement[1] = value[1];
	pColorElement[2] = value[2];
	pColorElement[3] = value[3];

	// Call the int internal set
	InternalSetIntValue(nValue);
}

//-----------------------------------------------------------------------------
// Purpose: Reset to default value.
//-----------------------------------------------------------------------------
void ConVar::Revert(void)
{
	SetValue(m_pszDefaultValue);
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
// Purpose: sets the ConVar color value from string.
// Input  : *pszValue - 
//-----------------------------------------------------------------------------
bool ConVar::SetColorFromString(const char* pszValue)
{
	bool bColor = false;

	// Try pulling RGBA color values out of the string.
	int nRGBA[4];
	int nParamsRead = sscanf_s(pszValue, "%i %i %i %i",
		&(nRGBA[0]), &(nRGBA[1]), &(nRGBA[2]), &(nRGBA[3]));

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
			unsigned char* pColorElement =
				(reinterpret_cast<unsigned char*>(&m_Value.m_nValue));

			pColorElement[0] = (unsigned char)nRGBA[0];
			pColorElement[1] = (unsigned char)nRGBA[1];
			pColorElement[2] = (unsigned char)nRGBA[2];
			pColorElement[3] = (unsigned char)nRGBA[3];

			// Copy that value into our float.
			m_Value.m_fValue = static_cast<float>(m_Value.m_nValue);
		}
	}

	return bColor;
}

//-----------------------------------------------------------------------------
// Purpose: changes the ConVar string value.
// Input  : *pszTempVal - flOldValue
//-----------------------------------------------------------------------------
void ConVar::ChangeStringValue(const char* pszTempVal)
{
	Assert(!(m_nFlags & FCVAR_NEVER_AS_STRING));

	char* pszOldValue = (char*)stackalloc(m_Value.m_iStringLength);
	memcpy(pszOldValue, m_Value.m_pszString, m_Value.m_iStringLength);

	size_t len = strlen(pszTempVal) + 1;

	if (len > m_Value.m_iStringLength)
	{
		if (m_Value.m_pszString)
		{
			delete[] m_Value.m_pszString;
		}

		m_Value.m_pszString = new char[len];
		m_Value.m_iStringLength = len;
	}

	memcpy(reinterpret_cast<void*>(m_Value.m_pszString), pszTempVal, len);

	// Invoke any necessary callback function
	for (int i = 0; i < m_fnChangeCallbacks.Count(); ++i)
	{
		m_fnChangeCallbacks[i](this, pszOldValue, NULL);
	}

	if (g_pCVar)
	{
		g_pCVar->CallGlobalChangeCallbacks(this, pszOldValue);
	}

	stackfree(pszOldValue);
}

//-----------------------------------------------------------------------------
// Purpose: Install a change callback (there shouldn't already be one....)
// Input  : callback - 
//			bInvoke - 
//-----------------------------------------------------------------------------
void ConVar::InstallChangeCallback(FnChangeCallback_t callback, bool bInvoke /*=true*/)
{
	if (!callback)
	{
		Warning(eDLL_T::ENGINE, "%s: Called with NULL callback; ignoring!!!\n",
			__FUNCTION__);
		return;
	}

	if (m_pParent->m_fnChangeCallbacks.Find(callback)
		!= m_pParent->m_fnChangeCallbacks.InvalidIndex())
	{
		// Same ptr added twice, sigh...
		Warning(eDLL_T::ENGINE, "%s: Ignoring duplicate change callback!!!\n",
			__FUNCTION__);
		return;
	}

	m_pParent->m_fnChangeCallbacks.AddToTail(callback);

	// Call it immediately to set the initial value...
	if (bInvoke)
	{
		callback(this, m_Value.m_pszString, m_Value.m_fValue);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Install a change callback (there shouldn't already be one....)
// Input  : callback - 
//-----------------------------------------------------------------------------
void ConVar::RemoveChangeCallback(FnChangeCallback_t callback)
{
	m_pParent->m_fnChangeCallbacks.FindAndRemove(callback);
}

//-----------------------------------------------------------------------------
// Purpose: Parse input flag string into bitfield
// Input  : pszFlags -
//			nFlags -
//			pszConVarName -
//-----------------------------------------------------------------------------
bool ConVar::ParseFlagString(const char* pszFlags, int& nFlags, const char* pszConVarName)
{
	size_t len = strlen(pszFlags);
	int flags = FCVAR_NONE;

	string sFlag = "";
	for (size_t i = 0; i < len; ++i)
	{
		char c = pszFlags[i];

		if (std::isspace(c))
			continue;

		if (c != '|')
			sFlag += c;

		if (c == '|' || i == len - 1)
		{
			if (sFlag == "")
				continue;

			if (s_ConVarFlags.count(sFlag) == 0)
			{
				Warning(eDLL_T::ENGINE,
					"%s: Attempted to parse invalid flag '%s' for convar '%s'\n",
					__FUNCTION__, sFlag.c_str(), pszConVarName);

				return false;
			}

			flags |= s_ConVarFlags.at(sFlag);

			sFlag = "";
		}
	}
	nFlags = flags;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ConVar_AppendFlags(ConCommandBase* var, char* buf, size_t bufsize)
{
	for (int i = 0; i < ARRAYSIZE(g_PrintConVarFlags); ++i)
	{
		const ConVarFlagsToString_t& info = g_PrintConVarFlags[i];
		if (var->IsFlagSet(info.m_nFlag))
		{
			char append[128];
			V_snprintf(append, sizeof(append), " %s", info.m_pszDesc);
			V_strncat(buf, append, bufsize);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ConVar_PrintDescription(ConCommandBase* pVar)
{
	bool bMin, bMax;
	float fMin, fMax;
	const char* pStr;

	Assert(pVar);

	Color clr(255, 100, 100, 255);

	char outstr[4096];
	outstr[0] = 0;

	if (!pVar->IsCommand())
	{
		ConVar* var = (ConVar*)pVar;

		bMin = var->GetMin(fMin);
		bMax = var->GetMax(fMax);

		const char* value = NULL;
		char tempVal[256];

		if (var->IsFlagSet(FCVAR_NEVER_AS_STRING))
		{
			value = tempVal;

			int intVal = var->GetInt();
			float floatVal = var->GetFloat();

			if (fabs((float)intVal - floatVal) < 0.000001)
			{
				V_snprintf(tempVal, sizeof(tempVal), "%d", intVal);
			}
			else
			{
				V_snprintf(tempVal, sizeof(tempVal), "%f", floatVal);
			}
		}
		else
		{
			value = var->GetString();
		}

		if (value)
		{
			AppendPrintf(outstr, sizeof(outstr), "\"%s\" = \"%s\"", var->GetName(), value);

			if (V_stricmp(value, var->GetDefault()))
			{
				AppendPrintf(outstr, sizeof(outstr), " ( def. \"%s\" )", var->GetDefault());
			}
		}

		if (bMin)
		{
			AppendPrintf(outstr, sizeof(outstr), " min. %f", fMin);
		}
		if (bMax)
		{
			AppendPrintf(outstr, sizeof(outstr), " max. %f", fMax);
		}
	}
	else
	{
		ConCommand* var = (ConCommand*)pVar;

		AppendPrintf(outstr, sizeof(outstr), "\"%s\" ", var->GetName());
	}

	ConVar_AppendFlags(pVar, outstr, sizeof(outstr));

	pStr = pVar->GetHelpText();
	if (pStr && *pStr)
	{
		DevMsg(eDLL_T::ENGINE, "%-80s - %.80s\n", outstr, pStr);
	}
	else
	{
		DevMsg(eDLL_T::ENGINE, "%-80s\n", outstr);
	}
}

struct ConVarFlags_t
{
	int			bit;
	const char* desc;
	const char* shortdesc;
};

#define CONVARFLAG( x, y )	{ FCVAR_##x, #x, #y }

static ConVarFlags_t g_ConVarFlags[] =
{
	CONVARFLAG(UNREGISTERED, "u" ),
	CONVARFLAG(ARCHIVE, "a"),
	CONVARFLAG(ARCHIVE_PLAYERPROFILE, "ap"),
	CONVARFLAG(SPONLY, "sp"),
	CONVARFLAG(GAMEDLL, "sv"),
	CONVARFLAG(CHEAT, "cheat"),
	CONVARFLAG(USERINFO, "user"),
	CONVARFLAG(NOTIFY, "nf"),
	CONVARFLAG(PROTECTED, "prot"),
	CONVARFLAG(PRINTABLEONLY, "print"),
	CONVARFLAG(UNLOGGED, "ulog"),
	CONVARFLAG(NEVER_AS_STRING, "numeric"),
	CONVARFLAG(REPLICATED, "rep"),
	CONVARFLAG(DEMO, "demo"),
	CONVARFLAG(DONTRECORD, "norecord"),
	CONVARFLAG(SERVER_CAN_EXECUTE, "server_can_execute"),
	CONVARFLAG(CLIENTCMD_CAN_EXECUTE, "clientcmd_can_execute"),
	CONVARFLAG(CLIENTDLL, "cl"),
};

static void PrintListHeader(FileHandle_t& f)
{
	char csvflagstr[1024];

	csvflagstr[0] = 0;

	int c = ARRAYSIZE(g_ConVarFlags);
	for (int i = 0; i < c; ++i)
	{
		char csvf[64];

		ConVarFlags_t& entry = g_ConVarFlags[i];
		Q_snprintf(csvf, sizeof(csvf), "\"%s\",", entry.desc);
		Q_strncat(csvflagstr, csvf, sizeof(csvflagstr) - strlen(csvflagstr) - 1);
	}

	FileSystem()->FPrintf(f, "\"%s\",\"%s\",%s,\"%s\"\n", "Name", "Value", csvflagstr, "Help Text");
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *var - 
//			*f - 
//-----------------------------------------------------------------------------
static void PrintCvar(ConVar* var, bool logging, FileHandle_t& fh)
{
	char flagstr[128];
	char csvflagstr[1024];

	flagstr[0] = 0;
	csvflagstr[0] = 0;

	int c = ARRAYSIZE(g_ConVarFlags);
	for (int i = 0; i < c; ++i)
	{
		char f[32];
		char csvf[64];
		size_t flen = sizeof(csvflagstr) - strlen(csvflagstr) - 1;

		ConVarFlags_t& entry = g_ConVarFlags[i];
		if (var->IsFlagSet(entry.bit))
		{
			Q_snprintf(f, sizeof(f), ", %s", entry.shortdesc);
			Q_strncat(flagstr, f, sizeof(flagstr) - strlen(flagstr) - 1);
			Q_snprintf(csvf, sizeof(csvf), "\"%s\",", entry.desc);
		}
		else
		{
			Q_snprintf(csvf, sizeof(csvf), ",");
		}

		Q_strncat(csvflagstr, csvf, flen);
	}


	char valstr[32];
	char tempbuff[512] = { 0 };

	// Clean up integers
	if (var->GetInt() == (int)var->GetFloat())
	{
		Q_snprintf(valstr, sizeof(valstr), "%-8i", var->GetInt());
	}
	else
	{
		Q_snprintf(valstr, sizeof(valstr), "%-8.3f", var->GetFloat());
	}

	// Print to console
	DevMsg(eDLL_T::ENGINE, "%-40s : %-8s : %-16s : %s\n", var->GetName(),
		valstr, flagstr, StripTabsAndReturns(var->GetHelpText(), tempbuff, sizeof(tempbuff)));
	if (logging)
	{
		FileSystem()->FPrintf(fh, "\"%s\",\"%s\",%s,\"%s\"\n", var->GetName(),
			valstr, csvflagstr, StripQuotes(var->GetHelpText(), tempbuff, sizeof(tempbuff)));
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output :
//-----------------------------------------------------------------------------
static void PrintCommand(const ConCommand* cmd, bool logging, FileHandle_t& f)
{
	// Print to console
	char tempbuff[512] = { 0 };
	DevMsg(eDLL_T::ENGINE, "%-40s : %-8s : %-16s : %s\n", cmd->GetName(),
		"cmd", "", StripTabsAndReturns(cmd->GetHelpText(), tempbuff, sizeof(tempbuff)));

	if (logging)
	{
		char emptyflags[256];

		emptyflags[0] = 0;

		int c = ARRAYSIZE(g_ConVarFlags);
		for (int i = 0; i < c; ++i)
		{
			char csvf[64];
			size_t len = sizeof(emptyflags) - strlen(emptyflags) - 1;

			Q_snprintf(csvf, sizeof(csvf), ",");
			Q_strncat(emptyflags, csvf, len);
		}
		// Names staring with +/- need to be wrapped in single quotes
		char nameBuf[256];
		Q_snprintf(nameBuf, sizeof(nameBuf), "%s", cmd->GetName());
		if (nameBuf[0] == '+' || nameBuf[0] == '-')
		{
			Q_snprintf(nameBuf, sizeof(nameBuf), "'%s'", cmd->GetName());
		}
		FileSystem()->FPrintf(f, "\"%s\",\"%s\",%s,\"%s\"\n", nameBuf, "cmd",
			emptyflags, StripQuotes(cmd->GetHelpText(), tempbuff, sizeof(tempbuff)));
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : bool
//-----------------------------------------------------------------------------
static bool ConCommandBaseLessFunc(ConCommandBase* const& lhs, ConCommandBase* const& rhs)
{
	const char* left = lhs->GetName();
	const char* right = rhs->GetName();

	if (*left == '-' || *left == '+')
		left++;
	if (*right == '-' || *right == '+')
		right++;

	return (Q_stricmp(left, right) < 0);
}

//-----------------------------------------------------------------------------
// Singleton CCvarUtilities
//-----------------------------------------------------------------------------
static CCvarUtilities g_CvarUtilities;
CCvarUtilities* cv = &g_CvarUtilities;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : int
//-----------------------------------------------------------------------------
int CCvarUtilities::CountVariablesWithFlags(int flags)
{
	int i = 0;
	ConCommandBase* var;
	CCvar::CCVarIteratorInternal* itint = g_pCVar->FactoryInternalIterator();

	for (itint->SetFirst(); itint->IsValid(); itint->Next()) // Loop through cvars...
	{
		var = itint->Get();
		if (!var->IsCommand())
		{
			if (var->IsFlagSet(flags))
			{
				i++;
			}
		}
	}

	delete itint;
	return i;
}

//-----------------------------------------------------------------------------
// Purpose: Removes the FCVAR_DEVELOPMENTONLY flag from all cvars, making them accessible
//-----------------------------------------------------------------------------
void CCvarUtilities::EnableDevCvars()
{
	// Loop through cvars...
	CCvar::CCVarIteratorInternal* itint = g_pCVar->FactoryInternalIterator();

	for (itint->SetFirst(); itint->IsValid(); itint->Next())
	{
		// remove flag from all cvars
		ConCommandBase* pCommandBase = itint->Get();
		pCommandBase->RemoveFlags(FCVAR_DEVELOPMENTONLY);
	}

	delete itint;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : void CCvar::CvarList_f
//-----------------------------------------------------------------------------
void CCvarUtilities::CvarList(const CCommand& args)
{
	ConCommandBase* var;	// Temporary Pointer to cvars
	int64 iArgs;						// Argument count
	const char* partial = NULL;		// Partial cvar to search for...
									// E.eg
	size_t ipLen = 0;				// Length of the partial cvar

	FileHandle_t f = FILESYSTEM_INVALID_HANDLE;         // FilePointer for logging
	bool bLogging = false;
	// Are we logging?
	iArgs = args.ArgC();		// Get count

	// Print usage?
	if (iArgs == 2 && !Q_strcasecmp(args[1], "?"))
	{
		DevMsg(eDLL_T::ENGINE, "convar_list:  [ log logfile ] [ partial ]\n");
		return;
	}

	if (!Q_strcasecmp(args[1], "log") && iArgs >= 3)
	{
		char fn[256];
		Q_snprintf(fn, sizeof(fn), "%s", args[2]);
		f = FileSystem()->Open(fn, "wb", nullptr, 0);
		if (f)
		{
			bLogging = true;
		}
		else
		{
			DevMsg(eDLL_T::ENGINE, "Couldn't open '%s' for writing!\n", fn);
			return;
		}

		if (iArgs == 4)
		{
			partial = args[3];
			ipLen = Q_strlen(partial);
		}
	}
	else
	{
		partial = args[1];
		ipLen = Q_strlen(partial);
	}

	// Banner
	DevMsg(eDLL_T::ENGINE, "convar list\n--------------\n");

	CUtlRBTree< ConCommandBase* > sorted(0, 0, ConCommandBaseLessFunc);
	CCvar::CCVarIteratorInternal* itint = g_pCVar->FactoryInternalIterator();

	for (itint->SetFirst(); itint->IsValid(); itint->Next()) // Loop through all instances.
	{
		var = itint->Get();

		if (!var->IsFlagSet(FCVAR_DEVELOPMENTONLY) &&
			!var->IsFlagSet(FCVAR_HIDDEN))
		{
			bool print = false;

			if (partial)  // Partial string searching?
			{
				if (!Q_strncasecmp(var->GetName(), partial, ipLen))
				{
					print = true;
				}
			}
			else
			{
				print = true;
			}

			if (print)
			{
				sorted.Insert(var);
			}
		}
	}

	delete itint;

	if (bLogging)
	{
		PrintListHeader(f);
	}
	for (unsigned short i = sorted.FirstInorder();
		i != sorted.InvalidIndex(); i = sorted.NextInorder(i))
	{
		var = sorted[i];
		if (var->IsCommand())
		{
			PrintCommand((ConCommand*)var, bLogging, f);
		}
		else
		{
			PrintCvar((ConVar*)var, bLogging, f);
		}
	}


	// Show total and syntax help...
	if (partial && partial[0])
	{
		DevMsg(eDLL_T::ENGINE, "--------------\n%3i convars/concommands for [%s]\n",
			sorted.Count(), partial);
	}
	else
	{
		DevMsg(eDLL_T::ENGINE, "--------------\n%3i total convars/concommands\n",
			sorted.Count());
	}

	if (bLogging)
	{
		FileSystem()->Close(f);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCvarUtilities::CvarHelp(const CCommand& args)
{
	const char* search;
	ConCommandBase* var;

	if (args.ArgC() != 2)
	{
		DevMsg(eDLL_T::ENGINE, "Usage:  help <cvarname>\n");
		return;
	}

	// Get name of var to find
	search = args[1];

	// Search for it
	var = g_pCVar->FindCommandBase(search);
	if (!var)
	{
		DevMsg(eDLL_T::ENGINE, "help:  no cvar or command named %s\n", search);
		return;
	}

	// Show info
	ConVar_PrintDescription(var);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCvarUtilities::CvarDifferences(const CCommand& args)
{
	CCvar::CCVarIteratorInternal* itint = g_pCVar->FactoryInternalIterator();
	int i = 0;

	for (itint->SetFirst(); itint->IsValid(); itint->Next()) // Loop through all instances.
	{
		ConCommandBase* pCommandBase = itint->Get();

		if (!pCommandBase->IsCommand() &&
			!pCommandBase->IsFlagSet(FCVAR_HIDDEN))
		{
			ConVar* pConVar = reinterpret_cast<ConVar*>(pCommandBase);

			if (V_strcmp(pConVar->GetString(), "FCVAR_NEVER_AS_STRING") != NULL)
			{
				if (V_stricmp(pConVar->GetString(), pConVar->GetDefault()) != NULL)
				{
					ConVar_PrintDescription(pConVar);
					i++;
				}
			}
		}
	}

	delete itint;
	DevMsg(eDLL_T::ENGINE, "--------------\n%3i changed convars\n", i);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCvarUtilities::CvarFindFlags_f(const CCommand& args)
{
	if (args.ArgC() < 2)
	{
		DevMsg(eDLL_T::ENGINE, "Usage:  convar_findByFlags <string>\n");
		DevMsg(eDLL_T::ENGINE, "Available flags to search for: \n");

		for (int i = 0; i < ARRAYSIZE(g_ConVarFlags); i++)
		{
			DevMsg(eDLL_T::ENGINE, "   - %s\n", g_ConVarFlags[i].desc);
		}
		return;
	}

	// Get substring to find
	const char* search = args[1];
	ConCommandBase* var;

	// Loop through vars and print out findings
	CCvar::CCVarIteratorInternal* itint = g_pCVar->FactoryInternalIterator();

	for (itint->SetFirst(); itint->IsValid(); itint->Next())
	{
		var = itint->Get();

		if (!var->IsFlagSet(FCVAR_DEVELOPMENTONLY) || !var->IsFlagSet(FCVAR_HIDDEN))
		{
			for (int i = 0; i < ARRAYSIZE(g_ConVarFlags); i++)
			{
				if (var->IsFlagSet(g_ConVarFlags[i].bit))
				{
					if (V_stristr(g_ConVarFlags[i].desc, search))
					{
						ConVar_PrintDescription(var);
					}
				}
			}
		}
	}

	delete itint;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CCvarUtilities::CvarFindFlagsCompletionCallback(const char* partial,
	char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	int flagC = ARRAYSIZE(g_ConVarFlags);
	char const* pcmd = "findflags ";
	size_t len = Q_strlen(partial);

	if (len < Q_strlen(pcmd))
	{
		int i = 0;
		for (; i < MIN(flagC, COMMAND_COMPLETION_MAXITEMS); i++)
		{
			Q_snprintf(commands[i], sizeof(commands[i]), "%s %s",
				pcmd, g_ConVarFlags[i].desc);
			Q_strlower(commands[i]);
		}
		return i;
	}

	char const* pSub = partial + Q_strlen(pcmd);
	size_t nSubLen = Q_strlen(pSub);

	int values = 0;
	for (int i = 0; i < flagC; ++i)
	{
		if (Q_strnicmp(g_ConVarFlags[i].desc, pSub, nSubLen))
			continue;

		Q_snprintf(commands[values], sizeof(commands[values]),
			"%s %s", pcmd, g_ConVarFlags[i].desc);
		Q_strlower(commands[values]);
		++values;

		if (values >= COMMAND_COMPLETION_MAXITEMS)
			break;
	}
	return values;
}

//-----------------------------------------------------------------------------
// Purpose: returns all ConVars
//-----------------------------------------------------------------------------
unordered_map<string, ConCommandBase*> CCvar::DumpToMap(void)
{
	stringstream ss;
	CCVarIteratorInternal* itint = FactoryInternalIterator(); // Allocate new InternalIterator.

	unordered_map<string, ConCommandBase*> allConVars;

	for (itint->SetFirst(); itint->IsValid(); itint->Next()) // Loop through all instances.
	{
		ConCommandBase* pCommand = itint->Get();
		const char* pszCommandName = pCommand->m_pszName;
		allConVars[pszCommandName] = pCommand;
	}

	delete itint;

	return allConVars;
}

///////////////////////////////////////////////////////////////////////////////
CCvar* g_pCVar = nullptr;


//-----------------------------------------------------------------------------
// Console command hash data structure
//-----------------------------------------------------------------------------
CConCommandHash::CConCommandHash()
{
	Purge(true);
}

CConCommandHash::~CConCommandHash()
{
	Purge(false);
}

void CConCommandHash::Purge(bool bReinitialize)
{
	m_aBuckets.Purge();
	m_aDataPool.Purge();
	if (bReinitialize)
	{
		Init();
	}
}

// Initialize.
void CConCommandHash::Init(void)
{
	// kNUM_BUCKETS must be a power of two.
	COMPILE_TIME_ASSERT((kNUM_BUCKETS & (kNUM_BUCKETS - 1)) == 0);

	// Set the bucket size.
	m_aBuckets.SetSize(kNUM_BUCKETS);
	for (int iBucket = 0; iBucket < kNUM_BUCKETS; ++iBucket)
	{
		m_aBuckets[iBucket] = m_aDataPool.InvalidIndex();
	}

	// Calculate the grow size.
	int nGrowSize = 4 * kNUM_BUCKETS;
	m_aDataPool.SetGrowSize(nGrowSize);
}

//-----------------------------------------------------------------------------
// Purpose: Insert data into the hash table given its key (unsigned int), 
//			WITH a check to see if the element already exists within the hash.
//-----------------------------------------------------------------------------
CConCommandHash::CCommandHashHandle_t CConCommandHash::Insert(ConCommandBase* cmd)
{
	// Check to see if that key already exists in the buckets (should be unique).
	CCommandHashHandle_t hHash = Find(cmd);
	if (hHash != InvalidHandle())
		return hHash;

	return FastInsert(cmd);
}
//-----------------------------------------------------------------------------
// Purpose: Insert data into the hash table given its key (unsigned int),
//          WITHOUT a check to see if the element already exists within the hash.
//-----------------------------------------------------------------------------
CConCommandHash::CCommandHashHandle_t CConCommandHash::FastInsert(ConCommandBase* cmd)
{
	// Get a new element from the pool.
	intptr_t iHashData = m_aDataPool.Alloc(true);
	HashEntry_t* RESTRICT pHashData = &m_aDataPool[iHashData];
	if (!pHashData)
		return InvalidHandle();

	HashKey_t key = Hash(cmd);

	// Add data to new element.
	pHashData->m_uiKey = key;
	pHashData->m_Data = cmd;

	// Link element.
	int iBucket = key & kBUCKETMASK; // HashFuncs::Hash( uiKey, m_uiBucketMask );
	m_aDataPool.LinkBefore(m_aBuckets[iBucket], iHashData);
	m_aBuckets[iBucket] = iHashData;

	return iHashData;
}

//-----------------------------------------------------------------------------
// Purpose: Remove a given element from the hash.
//-----------------------------------------------------------------------------
void CConCommandHash::Remove(CCommandHashHandle_t hHash) /*RESTRICT*/
{
	HashEntry_t* /*RESTRICT*/ entry = &m_aDataPool[hHash];
	HashKey_t iBucket = entry->m_uiKey & kBUCKETMASK;
	if (m_aBuckets[iBucket] == hHash)
	{
		// It is a bucket head.
		m_aBuckets[iBucket] = m_aDataPool.Next(hHash);
	}
	else
	{
		// Not a bucket head.
		m_aDataPool.Unlink(hHash);
	}

	// Remove the element.
	m_aDataPool.Remove(hHash);
}

//-----------------------------------------------------------------------------
// Purpose: Remove all elements from the hash
//-----------------------------------------------------------------------------
void CConCommandHash::RemoveAll(void)
{
	m_aBuckets.RemoveAll();
	m_aDataPool.RemoveAll();
}

//-----------------------------------------------------------------------------
// Find hash entry corresponding to a string name
//-----------------------------------------------------------------------------
CConCommandHash::CCommandHashHandle_t CConCommandHash::Find(
	const char* name, HashKey_t hashkey) const /*RESTRICT*/
{
	// hash the "key" - get the correct hash table "bucket"
	int iBucket = hashkey & kBUCKETMASK;

	for (datapool_t::IndexLocalType_t iElement = m_aBuckets[iBucket];
		iElement != m_aDataPool.InvalidIndex(); iElement = m_aDataPool.Next(iElement))
	{
		const HashEntry_t& element = m_aDataPool[iElement];
		if (element.m_uiKey == hashkey && // if hashes of strings match,
			Q_stricmp(name, element.m_Data->GetName()) == 0) // then test the actual strings
		{
			return iElement;
		}
	}

	// found nuffink
	return InvalidHandle();
}

//-----------------------------------------------------------------------------
// Find a command in the hash.
//-----------------------------------------------------------------------------
CConCommandHash::CCommandHashHandle_t CConCommandHash::Find(const ConCommandBase* cmd) const /*RESTRICT*/
{
	// Set this #if to 1 if the assert at bottom starts whining --
	// that indicates that a console command is being double-registered,
	// or something similarly non-fatally bad. With this #if 1, we'll search
	// by name instead of by pointer, which is more robust in the face
	// of double registered commands, but obviously slower.
#if 0 
	return Find(cmd->GetName());
#else
	HashKey_t hashkey = Hash(cmd);
	int iBucket = hashkey & kBUCKETMASK;

	// hunt through all entries in that bucket
	for (datapool_t::IndexLocalType_t iElement = m_aBuckets[iBucket];
		iElement != m_aDataPool.InvalidIndex(); iElement = m_aDataPool.Next(iElement))
	{
		const HashEntry_t& element = m_aDataPool[iElement];
		if (element.m_uiKey == hashkey && // if the hashes match... 
			element.m_Data == cmd) // and the pointers...
		{
			// in debug, test to make sure we don't have commands under the same name
			// or something goofy like that
			Assert(iElement == Find(cmd->GetName()),
				"ConCommand %s had two entries in the hash!", cmd->GetName());

			// return this element
			return iElement;
		}
	}

	// found nothing.
#ifdef DBGFLAG_ASSERT // double check against search by name
	CCommandHashHandle_t dbghand = Find(cmd->GetName());

	Assert(InvalidHandle() == dbghand,
		"ConCommand %s couldn't be found by pointer, but was found by name!", cmd->GetName());
#endif
	return InvalidHandle();
#endif
}


//#ifdef _DEBUG
// Dump a report to MSG
void CConCommandHash::Report(void)
{
	DevMsg(eDLL_T::ENGINE, "Console command hash bucket load:\n");
	int total = 0;
	for (int iBucket = 0; iBucket < kNUM_BUCKETS; ++iBucket)
	{
		int count = 0;
		CCommandHashHandle_t iElement = m_aBuckets[iBucket]; // get the head of the bucket
		while (iElement != m_aDataPool.InvalidIndex())
		{
			++count;
			iElement = m_aDataPool.Next(iElement);
		}

		DevMsg(eDLL_T::ENGINE, "%d: %d\n", iBucket, count);
		total += count;
	}

	DevMsg(eDLL_T::ENGINE, "\tAverage: %.1f\n", total / ((float)(kNUM_BUCKETS)));
}
//#endif

///////////////////////////////////////////////////////////////////////////////
void VCVar::Attach() const
{
	DetourAttach((LPVOID*)&v_ConVar_PrintDescription, &ConVar_PrintDescription);
}
void VCVar::Detach() const
{
	DetourDetach((LPVOID*)&v_ConVar_PrintDescription, &ConVar_PrintDescription);
}
