//=============================================================================//
//
// Purpose: Console Commands
//
//=============================================================================//

#include "tier0/tslist.h"
#include "tier0/memstd.h"
#include "tier0/commandline.h"
#include "tier1/cmd.h"
#include "tier1/cvar.h"
#include "tier1/characterset.h"
#include "tier1/utlstring.h"

//-----------------------------------------------------------------------------
// Global methods
//-----------------------------------------------------------------------------
static characterset_t s_BreakSet;
static bool s_bBuiltBreakSet = false;


//-----------------------------------------------------------------------------
// Tokenizer class
//-----------------------------------------------------------------------------
CCommand::CCommand()
{
	if (!s_bBuiltBreakSet)
	{
		s_bBuiltBreakSet = true;
		CharacterSetBuild(&s_BreakSet, "{}()':");
	}

	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: constructor
// Input  : nArgC - 
//			**ppArgV - 
//			source - 
//-----------------------------------------------------------------------------
CCommand::CCommand(int nArgC, const char** ppArgV, cmd_source_t source)
{
	Assert(nArgC > 0);

	if (!s_bBuiltBreakSet)
	{
		s_bBuiltBreakSet = true;
		CharacterSetBuild(&s_BreakSet, "{}()':");
	}

	Reset();

	char* pBuf = m_pArgvBuffer;
	char* pSBuf = m_pArgSBuffer;
	m_nArgc = nArgC;
	for (int i = 0; i < nArgC; ++i)
	{
		m_ppArgv[i] = pBuf;
		int64 nLen = strlen(ppArgV[i]);
		memcpy(pBuf, ppArgV[i], nLen + 1);
		if (i == 0)
		{
			m_nArgv0Size = nLen;
		}
		pBuf += nLen + 1;

		bool bContainsSpace = strchr(ppArgV[i], ' ') != NULL;
		if (bContainsSpace)
		{
			*pSBuf++ = '\"';
		}
		memcpy(pSBuf, ppArgV[i], nLen);
		pSBuf += nLen;
		if (bContainsSpace)
		{
			*pSBuf++ = '\"';
		}

		if (i != nArgC - 1)
		{
			*pSBuf++ = ' ';
		}
	}

	m_nQueuedVal = source;
}

//-----------------------------------------------------------------------------
// Purpose: tokenizer
// Input  : *pCommand - 
//			source - 
//			*pBreakSet - 
// Output : true on success, false on failure
//-----------------------------------------------------------------------------
bool CCommand::Tokenize(const char* pCommand, cmd_source_t source, characterset_t* pBreakSet)
{
	/* !TODO (CUtlBuffer).
	Reset();
	m_nQueuedVal = source;

	if (!pCommand)
		return false;

	// Use default break set
	if (!pBreakSet)
	{
		pBreakSet = &s_BreakSet;
	}

	// Copy the current command into a temp buffer
	// NOTE: This is here to avoid the pointers returned by DequeueNextCommand
	// to become invalid by calling AddText. Is there a way we can avoid the memcpy?
	int nLen = Q_strlen(pCommand);
	if (nLen >= COMMAND_MAX_LENGTH - 1)
	{
		Warning(eDLL_T::ENGINE, "%s: Encountered command which overflows the tokenizer buffer.. Skipping!\n", __FUNCTION__);
		return false;
	}

	memcpy(m_pArgSBuffer, pCommand, nLen + 1);

	// Parse the current command into the current command buffer
	CUtlBuffer bufParse(m_pArgSBuffer, nLen, CUtlBuffer::TEXT_BUFFER | CUtlBuffer::READ_ONLY);
	int nArgvBufferSize = 0;
	while (bufParse.IsValid() && (m_nArgc < COMMAND_MAX_ARGC))
	{
		char* pArgvBuf = &m_pArgvBuffer[nArgvBufferSize];
		int nMaxLen = COMMAND_MAX_LENGTH - nArgvBufferSize;
		int nStartGet = bufParse.TellGet();
		int	nSize = bufParse.ParseToken(pBreakSet, pArgvBuf, nMaxLen);
		if (nSize < 0)
			break;

		// Check for overflow condition
		if (nMaxLen == nSize)
		{
			Reset();
			return false;
		}

		if (m_nArgc == 1)
		{
			// Deal with the case where the arguments were quoted
			m_nArgv0Size = bufParse.TellGet();
			bool bFoundEndQuote = m_pArgSBuffer[m_nArgv0Size - 1] == '\"';
			if (bFoundEndQuote)
			{
				--m_nArgv0Size;
			}
			m_nArgv0Size -= nSize;
			Assert(m_nArgv0Size != 0);

			// The StartGet check is to handle this case: "foo"bar
			// which will parse into 2 different args. ArgS should point to bar.
			bool bFoundStartQuote = (m_nArgv0Size > nStartGet) && (m_pArgSBuffer[m_nArgv0Size - 1] == '\"');
			Assert(bFoundEndQuote == bFoundStartQuote);
			if (bFoundStartQuote)
			{
				--m_nArgv0Size;
			}
		}

		m_ppArgv[m_nArgc++] = pArgvBuf;
		if (m_nArgc >= COMMAND_MAX_ARGC)
		{
			Warning(eDLL_T::ENGINE, "%s: Encountered command which overflows the argument buffer.. Clamped!\n", __FUNCTION__);
		}

		nArgvBufferSize += nSize + 1;
		Assert(nArgvBufferSize <= COMMAND_MAX_LENGTH);
	}*/

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: returns argument count
//-----------------------------------------------------------------------------
int64_t CCommand::ArgC(void) const
{
	return m_nArgc;
}

//-----------------------------------------------------------------------------
// Purpose: returns argument vector
//-----------------------------------------------------------------------------
const char** CCommand::ArgV(void) const
{
	return m_nArgc ? (const char**)m_ppArgv : NULL;
}

//-----------------------------------------------------------------------------
// Purpose: returns all args that occur after the 0th arg, in string form
//-----------------------------------------------------------------------------
const char* CCommand::ArgS(void) const
{
	return m_nArgv0Size ? &m_pArgSBuffer[m_nArgv0Size] : "";
}

//-----------------------------------------------------------------------------
// Purpose: returns the entire command in string form, including the 0th arg
//-----------------------------------------------------------------------------
const char* CCommand::GetCommandString(void) const
{
	return m_nArgc ? m_pArgSBuffer : "";
}

//-----------------------------------------------------------------------------
// Purpose: returns argument from index as string
// Input  : nIndex - 
//-----------------------------------------------------------------------------
const char* CCommand::Arg(int nIndex) const
{
	// FIXME: Many command handlers appear to not be particularly careful
	// about checking for valid argc range. For now, we're going to
	// do the extra check and return an empty string if it's out of range
	if (nIndex < 0 || nIndex >= m_nArgc)
	{
		return "";
	}
	return m_ppArgv[nIndex];
}

//-----------------------------------------------------------------------------
// Purpose: gets at arguments
// Input  : nInput - 
//-----------------------------------------------------------------------------
const char* CCommand::operator[](int nIndex) const
{
	return Arg(nIndex);
}

//-----------------------------------------------------------------------------
// Purpose: returns max command length
//-----------------------------------------------------------------------------
int CCommand::MaxCommandLength(void) const
{
	return COMMAND_MAX_LENGTH - 1;
}


//-----------------------------------------------------------------------------
// Purpose: return boolean depending on if the string only has digits in it
// Input  : svString - 
//-----------------------------------------------------------------------------
bool CCommand::HasOnlyDigits(int nIndex) const
{
	const string svString = Arg(nIndex);
	for (const char& character : svString)
	{
		if (std::isdigit(character) == 0)
		{
			return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: reset
//-----------------------------------------------------------------------------
void CCommand::Reset()
{
	m_nArgc = 0;
	m_nArgv0Size = 0;
	m_pArgSBuffer[0] = 0;
	m_nQueuedVal = cmd_source_t::kCommandSrcInvalid;
}

//-----------------------------------------------------------------------------
// Purpose: create
//-----------------------------------------------------------------------------
ConCommand* ConCommand::StaticCreate(const char* pszName, const char* pszHelpString, const char* pszUsageString,
	int nFlags, FnCommandCallback_t pCallback, FnCommandCompletionCallback pCompletionFunc)
{
	ConCommand* pCommand = (ConCommand*)malloc(sizeof(ConCommand));
	*(ConCommandBase**)pCommand = g_pConCommandVFTable;

	pCommand->m_pNext = nullptr;
	pCommand->m_bRegistered = false;

	pCommand->m_pszName = pszName;
	pCommand->m_pszHelpString = pszHelpString;
	pCommand->m_pszUsageString = pszUsageString;
	pCommand->s_pAccessor = nullptr;
	pCommand->m_nFlags = nFlags;

	pCommand->m_nNullCallBack = NullSub;
	pCommand->m_pSubCallback = nullptr;
	pCommand->m_fnCommandCallback = pCallback;
	pCommand->m_bHasCompletionCallback = pCompletionFunc != nullptr ? true : false;
	pCommand->m_bUsingNewCommandCallback = true;
	pCommand->m_bUsingCommandCallbackInterface = false;
	pCommand->m_fnCompletionCallback = pCompletionFunc ? pCompletionFunc : CallbackStub;

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
