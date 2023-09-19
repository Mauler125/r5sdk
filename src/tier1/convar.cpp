//=============================================================================//
//
// Purpose:
//
//=============================================================================//
#include "tier0/tslist.h"
#include "tier1/convar.h"

//-----------------------------------------------------------------------------
// Purpose: Checks if ConCommand has requested flags.
// Input  : nFlags - 
// Output : True if ConCommand has nFlags.
//-----------------------------------------------------------------------------
bool ConCommandBase::HasFlags(int nFlags) const
{
	return m_nFlags & nFlags;
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

//-----------------------------------------------------------------------------
// Purpose: create
//-----------------------------------------------------------------------------
ConCommand* ConCommand::StaticCreate(const char* pszName, const char* pszHelpString, const char* pszUsageString,
	int nFlags, FnCommandCallback_t pCallback, FnCommandCompletionCallback pCompletionFunc)
{
	ConCommand* pCommand = (ConCommand*)malloc(sizeof(ConCommand));
	*(ConCommand**)pCommand = g_pConCommandVFTable;

	pCommand->m_pNext = nullptr;
	pCommand->m_bRegistered = false;

	pCommand->m_pszName = pszName;
	pCommand->m_pszHelpString = pszHelpString;
	pCommand->m_pszUsageString = pszUsageString;
	pCommand->s_pAccessor = nullptr;
	pCommand->m_nFlags = nFlags;

	pCommand->m_nNullCallBack = DefaultNullSub;
	pCommand->m_pSubCallback = nullptr;
	pCommand->m_fnCommandCallback = pCallback;
	pCommand->m_bHasCompletionCallback = pCompletionFunc != nullptr ? true : false;
	pCommand->m_bUsingNewCommandCallback = true;
	pCommand->m_bUsingCommandCallbackInterface = false;
	pCommand->m_fnCompletionCallback = pCompletionFunc ? pCompletionFunc : DefaultCompletionFunc;

	g_pCVar->RegisterConCommand(pCommand);
	return pCommand;
}

//-----------------------------------------------------------------------------
// Purpose: construct/allocate
//-----------------------------------------------------------------------------
ConCommand::ConCommand()
	: m_nNullCallBack(nullptr)
	, m_pSubCallback(nullptr)
	, m_fnCommandCallbackV1(nullptr)
	, m_fnCompletionCallback(nullptr)
	, m_bHasCompletionCallback(false)
	, m_bUsingNewCommandCallback(false)
	, m_bUsingCommandCallbackInterface(false)
{
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
