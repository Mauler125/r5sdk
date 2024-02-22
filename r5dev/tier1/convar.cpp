//=============================================================================//
//
// Purpose:
//
//=============================================================================//
#include "tier0/tslist.h"
#include "tier1/convar.h"

//-----------------------------------------------------------------------------
// Statically constructed list of ConCommandBases, 
// used for registering them with the ICVar interface
//-----------------------------------------------------------------------------
ConCommandBase* ConCommandBase::s_pConCommandBases = NULL;
IConCommandBaseAccessor* ConCommandBase::s_pAccessor = NULL;

static int s_nCVarFlag = 0;

// An unique identifier indicating which DLL this convar came from
static int s_nDLLIdentifier = -1;
static bool s_bRegistered = false;

class CDefaultAccessor : public IConCommandBaseAccessor
{
public:
	virtual bool RegisterConCommandBase(ConCommandBase* const pVar)
	{
		// Link to engine's list instead
		g_pCVar->RegisterConCommand(pVar);
		return true;
	}
};

static CDefaultAccessor s_DefaultAccessor;

//-----------------------------------------------------------------------------
// Called by the framework to register ConCommandBases with the ICVar
//-----------------------------------------------------------------------------
void ConVar_Register(int nCVarFlag, IConCommandBaseAccessor* pAccessor)
{
	if (!g_pCVar || s_bRegistered)
		return;

	Assert(s_nDLLIdentifier < 0);
	s_bRegistered = true;
	s_nCVarFlag = nCVarFlag;
	s_nDLLIdentifier = g_pCVar->AllocateDLLIdentifier();

	ConCommandBase* pCur, * pNext;

	ConCommandBase::s_pAccessor = pAccessor ? pAccessor : &s_DefaultAccessor;
	pCur = ConCommandBase::s_pConCommandBases;

	while (pCur)
	{
		pNext = pCur->m_pNext;
		pCur->AddFlags(s_nCVarFlag);
		pCur->Init();
		pCur = pNext;
	}

	g_pCVar->ProcessQueuedMaterialThreadConVarSets();

	ConCommandBase::s_pConCommandBases = NULL;
}

void ConVar_Unregister()
{
	if (!g_pCVar || !s_bRegistered)
		return;

	Assert(s_nDLLIdentifier >= 0);

	// Do this after unregister!!!
	g_pCVar->UnregisterConCommands(s_nDLLIdentifier);
	s_nDLLIdentifier = -1;
	s_bRegistered = false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if this is a command 
//-----------------------------------------------------------------------------
bool ConCommandBase::IsCommand(void) const
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Checks if ConCommand has requested flags.
// Input  : flag - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool ConCommandBase::IsFlagSet(const int flag) const
{
	return (flag & m_nFlags) ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flags - 
//-----------------------------------------------------------------------------
void ConCommandBase::AddFlags(const int flags)
{
	m_nFlags |= flags;
}

//-----------------------------------------------------------------------------
// Purpose: removes specified flags
//-----------------------------------------------------------------------------
void ConCommandBase::RemoveFlags(const int flags)
{
	m_nFlags &= ~flags;
}

//-----------------------------------------------------------------------------
// Purpose: returns current flags
//-----------------------------------------------------------------------------
int ConCommandBase::GetFlags() const
{
	return m_nFlags;
}

//-----------------------------------------------------------------------------
// Purpose: Return name of the command/var
// Output : const char
//-----------------------------------------------------------------------------
const char* ConCommandBase::GetName(void) const
{
	return m_pszName;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char* ConCommandBase::GetHelpText(void) const
{
	return m_pszHelpString;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char* ConCommandBase::GetUsageText(void) const
{
	return m_pszUsageString;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ConCommandBase::SetAccessor(IConCommandBaseAccessor* const pAccessor)
{
	m_pAccessor = pAccessor;
}

//-----------------------------------------------------------------------------
// Purpose: Has this cvar been registered
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool ConCommandBase::IsRegistered(void) const
{
	return m_bRegistered;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the DLL identifier
//-----------------------------------------------------------------------------
int ConCommandBase::GetDLLIdentifier(void) const
{
	return s_nDLLIdentifier;
}

//-----------------------------------------------------------------------------
// Purpose: Used internally by OneTimeInit to initialize.
//-----------------------------------------------------------------------------
void ConCommandBase::Init()
{
	if (s_pAccessor)
	{
		s_pAccessor->RegisterConCommandBase(this);
	}
}

void ConCommandBase::Shutdown()
{
	if (g_pCVar)
	{
		g_pCVar->UnregisterConCommand(this);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pName - 
//			callback - 
//			*pHelpString - 
//			flags - 
//-----------------------------------------------------------------------------
ConCommandBase* ConCommandBase::Create(const char* pName, const char* pHelpString, int flags, const char* pszUsageString)
{
	m_bRegistered = false;

	// Name should be static data
	Assert(pName);
	m_pszName = pName;

	m_pszHelpString = pHelpString ? pHelpString : "";
	m_pszUsageString = pszUsageString ? pszUsageString : "";

	m_pAccessor = nullptr;

	m_nFlags = flags;

	const int releaseMask = FCVAR_ARCHIVE|FCVAR_USERINFO|FCVAR_CHEAT|
		FCVAR_RELEASE|FCVAR_ARCHIVE_PLAYERPROFILE|FCVAR_CLIENTCMD_CAN_EXECUTE;

	if (!(m_nFlags & releaseMask))
	{
		m_nFlags |= FCVAR_DEVELOPMENTONLY;
	}

	if (!(m_nFlags & FCVAR_UNREGISTERED))
	{
		m_pNext = s_pConCommandBases;
		s_pConCommandBases = this;
	}
	else
	{
		// It's unregistered
		m_pNext = NULL;
	}

	// If s_pAccessor is already set (this ConVar is not a global variable),
	//  register it.
	if (s_pAccessor)
	{
		Init();
	}

	return m_pNext;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const ConCommandBase
//-----------------------------------------------------------------------------
ConCommandBase* ConCommandBase::GetNext(void) const
{
	return m_pNext;
}

//-----------------------------------------------------------------------------
// Purpose: Copies string using local new/delete operators
// Input  : *szFrom - 
// Output : char
//-----------------------------------------------------------------------------
char* ConCommandBase::CopyString(const char* szFrom) const
{
	size_t nLen;
	char* szTo;

	nLen = strlen(szFrom);
	if (nLen <= 0)
	{
		szTo = new char[1];
		szTo[0] = 0;
	}
	else
	{
		szTo = new char[nLen + 1];
		memmove(szTo, szFrom, nLen + 1);
	}
	return szTo;
}

//-----------------------------------------------------------------------------
// Default do nothing function
//-----------------------------------------------------------------------------
void DefaultNullSub()
{
	; /*DO NOTHING*/
}

//-----------------------------------------------------------------------------
// Default console command autocompletion function
//-----------------------------------------------------------------------------
int DefaultCompletionFunc(const char* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	return 0;
}

ConCommand::ConCommand(const char* pName, FnCommandCallbackV1_t callback, const char* pHelpString /*= 0*/, 
	int flags /*= 0*/, FnCommandCompletionCallback completionFunc /*= 0*/, const char* pszUsageString /*= 0*/)
{
	m_nNullCallBack = DefaultNullSub;
	m_pSubCallback = nullptr;

	// Set the callback
	m_fnCommandCallbackV1 = callback;
	m_bUsingNewCommandCallback = false;
	m_bUsingCommandCallbackInterface = false;
	m_fnCompletionCallback = completionFunc ? completionFunc : DefaultCompletionFunc;
	m_bHasCompletionCallback = completionFunc != 0 ? true : false;

	// Setup the rest
	BaseClass::Create(pName, pHelpString, flags, pszUsageString);
}

ConCommand::ConCommand(const char* pName, FnCommandCallback_t callback, const char* pHelpString /*= 0*/, 
	int flags /*= 0*/, FnCommandCompletionCallback completionFunc /*= 0*/, const char* pszUsageString /*= 0*/)
{
	m_nNullCallBack = DefaultNullSub;
	m_pSubCallback = nullptr;

	// Set the callback
	m_fnCommandCallback = callback;
	m_bUsingNewCommandCallback = true;
	m_fnCompletionCallback = completionFunc ? completionFunc : DefaultCompletionFunc;
	m_bHasCompletionCallback = completionFunc != 0 ? true : false;
	m_bUsingCommandCallbackInterface = false;

	// Setup the rest
	BaseClass::Create(pName, pHelpString, flags, pszUsageString);
}

ConCommand::ConCommand(const char* pName, ICommandCallback* pCallback, const char* pHelpString /*= 0*/, 
	int flags /*= 0*/, ICommandCompletionCallback* pCompletionCallback /*= 0*/, const char* pszUsageString /*= 0*/)
{
	m_nNullCallBack = DefaultNullSub;
	m_pSubCallback = nullptr;

	// Set the callback
	m_pCommandCallback = pCallback;
	m_bUsingNewCommandCallback = false;
	m_pCommandCompletionCallback = pCompletionCallback;
	m_bHasCompletionCallback = (pCompletionCallback != 0);
	m_bUsingCommandCallbackInterface = true;

	// Setup the rest
	BaseClass::Create(pName, pHelpString, flags, pszUsageString);
}

//-----------------------------------------------------------------------------
// Purpose: destructor
//-----------------------------------------------------------------------------
ConCommand::~ConCommand()
{
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if this is a command 
//-----------------------------------------------------------------------------
bool ConCommand::IsCommand(void) const
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: calls the autocompletion method to get autocompletion suggestions
//-----------------------------------------------------------------------------
int	ConCommand::AutoCompleteSuggest(const char* partial, CUtlVector< CUtlString >& commands)
{
	if (m_bUsingCommandCallbackInterface)
	{
		if (!m_pCommandCompletionCallback)
			return 0;

		return m_pCommandCompletionCallback->CommandCompletionCallback(partial, commands);
	}

	Assert(m_fnCompletionCallback);

	if (!m_fnCompletionCallback)
		return 0;

	char rgpchCommands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH];
	const int iret = (m_fnCompletionCallback)(partial, rgpchCommands);

	for (int i = 0; i < iret; ++i)
	{
		const CUtlString str = rgpchCommands[i];
		commands.AddToTail(str);
	}

	return iret;
}

//-----------------------------------------------------------------------------
// Returns true if the console command can autocomplete 
//-----------------------------------------------------------------------------
bool ConCommand::CanAutoComplete(void) const
{
	return m_bHasCompletionCallback;
}

//-----------------------------------------------------------------------------
// Purpose: invoke the function if there is one
//-----------------------------------------------------------------------------
void ConCommand::Dispatch(const CCommand& command)
{
	if (m_bUsingNewCommandCallback)
	{
		if (m_fnCommandCallback)
		{
			(*m_fnCommandCallback)(command);
			return;
		}
	}
	else if (m_bUsingCommandCallbackInterface)
	{
		if (m_pCommandCallback)
		{
			m_pCommandCallback->CommandCallback(command);
			return;
		}
	}
	else
	{
		if (m_fnCommandCallbackV1)
		{
			(*m_fnCommandCallbackV1)();
			return;
		}
	}

	// Command without callback!!!
	AssertMsg(0, "Encountered ConCommand '%s' without a callback!\n", GetName());
}

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
	pNewConVar->m_pAccessor = nullptr;
	pNewConVar->m_nFlags = FCVAR_NONE;
	pNewConVar->m_pNext = nullptr;

	pNewConVar->m_fnChangeCallbacks.Init();

	ConVar__Register(pNewConVar, pszName, pszDefaultValue, nFlags,
		pszHelpString, bMin, fMin, bMax, fMax, pCallback, pszUsageString);
	return pNewConVar;
}

//-----------------------------------------------------------------------------
// Purpose: destroy
//-----------------------------------------------------------------------------
void ConVar::Destroy(void)
{
	ConVar__Unregister(this);
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
		Warning(eDLL_T::COMMON, "%s: Called with NULL callback; ignoring!!!\n",
			__FUNCTION__);
		return;
	}

	if (m_pParent->m_fnChangeCallbacks.Find(callback)
		!= m_pParent->m_fnChangeCallbacks.InvalidIndex())
	{
		// Same ptr added twice, sigh...
		Warning(eDLL_T::COMMON, "%s: Ignoring duplicate change callback!!!\n",
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
