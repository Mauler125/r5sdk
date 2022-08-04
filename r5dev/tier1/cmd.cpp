//=============================================================================//
//
// Purpose: Console Commands
//
//=============================================================================//

#include "core/stdafx.h"
#include "tier0/tslist.h"
#include "tier0/memstd.h"
#include "tier1/cmd.h"
#include "tier1/cvar.h"
#include "tier1/characterset.h"
#include "vstdlib/callback.h"

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
		int nLen = strlen(ppArgV[i]);
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
// Purpose: returns max command lenght
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
	string svString = Arg(nIndex);
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
// Purpose: construct/allocate
//-----------------------------------------------------------------------------
ConCommand::ConCommand(const char* pszName, const char* pszHelpString, int nFlags, void* pCallback, void* pCommandCompletionCallback)
{
	ConCommand* pCommand = MemAllocSingleton()->Alloc<ConCommand>(sizeof(ConCommand));
	memset(pCommand, '\0', sizeof(ConCommand));

	pCommand->m_pConCommandBaseVFTable = g_pConCommandVtable.RCast<IConCommandBase*>();
	pCommand->m_pszName          = pszName;
	pCommand->m_pszHelpString    = pszHelpString;
	pCommand->m_nFlags           = nFlags;
	pCommand->m_nNullCallBack    = NullSub;
	pCommand->m_pCommandCallback = pCallback;
	pCommand->m_nCallbackFlags   = 2;
	if (pCommandCompletionCallback)
	{
		pCommand->m_pCompletionCallback = pCommandCompletionCallback;
	}
	else
	{
		pCommand->m_pCompletionCallback = CallbackStub;
	}
	ConCommand_RegisterConCommand(pCommand);
	*this = *pCommand;
}

//-----------------------------------------------------------------------------
// Purpose: ConCommand registration
//-----------------------------------------------------------------------------
void ConCommand::Init(void)
{
	//-------------------------------------------------------------------------
	// ENGINE DLL                                                             |
	new ConCommand("bhit", "Bullet-hit trajectory debug.", FCVAR_DEVELOPMENTONLY | FCVAR_GAMEDLL, BHit_f, nullptr);
#ifndef DEDICATED
	new ConCommand("line", "Draw a debug line.", FCVAR_GAMEDLL | FCVAR_CHEAT, Line_f, nullptr);
	new ConCommand("sphere", "Draw a debug sphere.", FCVAR_GAMEDLL | FCVAR_CHEAT, Sphere_f, nullptr);
	new ConCommand("capsule", "Draw a debug capsule.", FCVAR_GAMEDLL | FCVAR_CHEAT, Capsule_f, nullptr);
#endif //!DEDICATED
	//-------------------------------------------------------------------------
	// SERVER DLL                                                             |
#ifndef CLIENT_DLL
	new ConCommand("script", "Run input code as SERVER script on the VM.", FCVAR_GAMEDLL | FCVAR_CHEAT, SQVM_ServerScript_f, nullptr);
	new ConCommand("sv_kick", "Kick a client from the server by name. | Usage: kick \"<name>\".", FCVAR_RELEASE, Host_Kick_f, nullptr);
	new ConCommand("sv_kickid", "Kick a client from the server by UserID or OriginID | Usage: kickid \"<UserID>\"/\"<OriginID>\".", FCVAR_RELEASE, Host_KickID_f, nullptr);
	new ConCommand("sv_ban", "Bans a client from the server by name. | Usage: ban <name>.", FCVAR_RELEASE, Host_Ban_f, nullptr);
	new ConCommand("sv_banid", "Bans a client from the server by UserID, OriginID or IPAddress | Usage: banid \"<UserID>\"/\"<OriginID>/<IPAddress>\".", FCVAR_RELEASE, Host_BanID_f, nullptr);
	new ConCommand("sv_unban", "Unbans a client from the server by OriginID or IPAddress | Usage: unban \"<OriginID>\"/\"<IPAddress>\".", FCVAR_RELEASE, Host_Unban_f, nullptr);
	new ConCommand("sv_reloadbanlist", "Reloads the ban list from the disk.", FCVAR_RELEASE, Host_ReloadBanList_f, nullptr);
#endif // !CLIENT_DLL
#ifndef DEDICATED
	//-------------------------------------------------------------------------
	// CLIENT DLL                                                             |
	new ConCommand("script_client", "Run input code as CLIENT script on the VM.", FCVAR_CLIENTDLL | FCVAR_CHEAT, SQVM_ClientScript_f, nullptr);
	new ConCommand("cl_showconsole", "Opens the game console.", FCVAR_CLIENTDLL | FCVAR_RELEASE, GameConsole_Invoke_f, nullptr);
	new ConCommand("cl_showbrowser", "Opens the server browser.", FCVAR_CLIENTDLL | FCVAR_RELEASE, ServerBrowser_Invoke_f, nullptr);
	new ConCommand("rcon", "Forward RCON query to remote server. | Usage: rcon \"<query>\".", FCVAR_CLIENTDLL | FCVAR_RELEASE, RCON_CmdQuery_f, nullptr);
	new ConCommand("rcon_disconnect", "Disconnect from RCON server.", FCVAR_CLIENTDLL | FCVAR_RELEASE, RCON_Disconnect_f, nullptr);
	//-------------------------------------------------------------------------
	// UI DLL                                                                 |
	new ConCommand("script_ui", "Run input code as UI script on the VM.", FCVAR_CLIENTDLL | FCVAR_CHEAT, SQVM_UIScript_f, nullptr);
#endif // !DEDICATED
	//-------------------------------------------------------------------------
	// FILESYSTEM API                                                         |
	new ConCommand("fs_vpk_mount",  "Mounts a VPK file for FileSystem usage.", FCVAR_DEVELOPMENTONLY, VPK_Mount_f, nullptr);
	new ConCommand("fs_vpk_build",  "Builds a VPK file from current workspace.", FCVAR_DEVELOPMENTONLY, VPK_Pack_f, nullptr);
	new ConCommand("fs_vpk_unpack", "Unpacks all files from a VPK file.", FCVAR_DEVELOPMENTONLY, VPK_Unpack_f, nullptr);
	//-------------------------------------------------------------------------
	// RTECH API                                                              |
	new ConCommand("rtech_strtoguid", "Calculates the GUID from input data.", FCVAR_DEVELOPMENTONLY, RTech_StringToGUID_f, nullptr);
	new ConCommand("rtech_decompress", "Decompresses the specified RPAK file.", FCVAR_DEVELOPMENTONLY, RTech_Decompress_f, nullptr);
	new ConCommand("pak_requestload", "Requests asynchronous load for specified RPAK file.", FCVAR_DEVELOPMENTONLY, Pak_RequestLoad_f, nullptr);
	new ConCommand("pak_requestunload", "Requests unload for specified RPAK file or ID.", FCVAR_DEVELOPMENTONLY, Pak_RequestUnload_f, nullptr);
	new ConCommand("pak_swap", "Requests swap for specified RPAK file or ID", FCVAR_DEVELOPMENTONLY, Pak_Swap_f, nullptr);
	new ConCommand("pak_listpaks", "Display a list of the loaded Pak files.", FCVAR_DEVELOPMENTONLY, Pak_ListPaks_f, nullptr);
	//-------------------------------------------------------------------------
	// NETCHANNEL                                                             |
	new ConCommand("net_setkey", "Sets user specified base64 net key.", FCVAR_RELEASE, NET_SetKey_f, nullptr);
	new ConCommand("net_generatekey", "Generates and sets a random base64 net key.", FCVAR_RELEASE, NET_GenerateKey_f, nullptr);
}

//-----------------------------------------------------------------------------
// Purpose: shipped ConCommand initialization
//-----------------------------------------------------------------------------
void ConCommand::InitShipped(void)
{
#ifndef DEDICATED
	//-------------------------------------------------------------------------
	// MATERIAL SYSTEM
	g_pCVar->FindCommand("mat_crosshair")->m_pCommandCallback = Mat_CrossHair_f; // Patch callback function to working callback.
#endif // !DEDICATED
}

//-----------------------------------------------------------------------------
// Purpose: unregister extraneous ConCommand's.
//-----------------------------------------------------------------------------
void ConCommand::PurgeShipped(void) const
{
#ifdef DEDICATED
	const char* pszCommandToRemove[] =
	{
		"bind",
		"bind_held",
		"bind_list",
		"bind_list_abilities",
		"bind_US_standard",
		"bind_held_US_standard",
		"unbind",
		"unbind_US_standard",
		"unbindall",
		"unbind_all_gamepad",
		"unbindall_ignoreGamepad",
		"unbind_batch",
		"unbind_held",
		"unbind_held_US_standard",
		"uiscript_reset",
		"getpos_bind",
		"connect",
		"silent_connect",
		"ping",
		"gameui_activate",
		"gameui_hide",
		"weaponSelectOrdnance",
		"weaponSelectPrimary0",
		"weaponSelectPrimary1",
		"weaponSelectPrimary2",
		"+scriptCommand1",
		"-scriptCommand1",
		"+scriptCommand2",
		"-scriptCommand2",
		"+scriptCommand3",
		"-scriptCommand3",
		"+scriptCommand4",
		"-scriptCommand4",
		"+scriptCommand5",
		"-scriptCommand5",
		"+scriptCommand6",
		"-scriptCommand6",
		"+scriptCommand7",
		"-scriptCommand7",
		"+scriptCommand8",
		"-scriptCommand8",
		"+scriptCommand9",
		"-scriptCommand9",
	};

	for (int i = 0; i < (&pszCommandToRemove)[1] - pszCommandToRemove; i++)
	{
		ConCommandBase* pCommandBase = g_pCVar->FindCommandBase(pszCommandToRemove[i]);

		if (pCommandBase)
		{
			g_pCVar->UnregisterConCommand(pCommandBase);
		}
	}
#endif // DEDICATED
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if this is a command 
// Output : bool
//-----------------------------------------------------------------------------
bool ConCommand::IsCommand(void) const
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if this is a command 
// Output : bool
//-----------------------------------------------------------------------------
bool ConCommandBase::IsCommand(void) const
{
	return m_pConCommandBaseVFTable != g_pConVarVFTable.RCast<void*>();
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
// Purpose: Test each ConCommand query before execution.
// Input  : *pCommandBase - nFlags 
// Output : False if execution is permitted, true if not.
//-----------------------------------------------------------------------------
bool ConCommandBase::IsFlagSet(ConCommandBase* pCommandBase, int nFlags)
{
	if (cm_debug_cmdquery->GetBool())
	{
		printf("--------------------------------------------------\n");
		printf(" Flaged: %08X\n", pCommandBase->m_nFlags);
	}
	// Mask off FCVAR_CHEATS and FCVAR_DEVELOPMENTONLY.
	if (cm_unset_cheat_cmdquery->GetBool())
	{
		pCommandBase->RemoveFlags(FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT);
	}
	else if (cm_unset_dev_cmdquery->GetBool())// Mask off FCVAR_DEVELOPMENTONLY.
	{
		pCommandBase->RemoveFlags(FCVAR_DEVELOPMENTONLY);
	}
	if (cm_debug_cmdquery->GetBool())
	{
		printf(" Masked: %08X\n", pCommandBase->m_nFlags);
		printf(" Verify: %08X\n", nFlags);
		printf("--------------------------------------------------\n");
	}
	if (nFlags & FCVAR_RELEASE && !cm_unset_all_cmdquery->GetBool())
	{
		// Default retail behaviour.
		return ConCommandBase_IsFlagSet(pCommandBase, nFlags);
	}
	if (cm_unset_all_cmdquery->GetBool())
	{
		// Returning false on all queries may cause problems.
		return false;
	}
	// Default behavior.
	return pCommandBase->HasFlags(nFlags) != 0;
}

//-----------------------------------------------------------------------------
// Purpose: Checks if ConCommand has requested flags.
// Input  : nFlags - 
// Output : True if ConCommand has nFlags.
//-----------------------------------------------------------------------------
bool ConCommandBase::HasFlags(int nFlags)
{
	return m_nFlags & nFlags;
}

//-----------------------------------------------------------------------------
// Purpose: Add's flags to ConCommand.
// Input  : nFlags - 
//-----------------------------------------------------------------------------
void ConCommandBase::AddFlags(int nFlags)
{
	m_nFlags |= nFlags;
}

//-----------------------------------------------------------------------------
// Purpose: Removes flags from ConCommand.
// Input  : nFlags - 
//-----------------------------------------------------------------------------
void ConCommandBase::RemoveFlags(int nFlags)
{
	m_nFlags &= ~nFlags;
}

//-----------------------------------------------------------------------------
// Purpose: Returns current flags.
// Output : int
//-----------------------------------------------------------------------------
int ConCommandBase::GetFlags(void) const
{
	return m_nFlags;
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
// Purpose: Returns the ConCommandBase name.
// Output : const char*
//-----------------------------------------------------------------------------
const char* ConCommandBase::GetName(void) const
{
	return m_pszName;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the ConCommandBase help text.
// Output : const char*
//-----------------------------------------------------------------------------
const char* ConCommandBase::GetHelpText(void) const
{
	return m_pszHelpString;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the ConCommandBase usage text.
// Output : const char*
//-----------------------------------------------------------------------------
const char* ConCommandBase::GetUsageText(void) const
{
	return m_pszUsageString;
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
// Purpose: Returns current player calling this function
// Output : ECommandTarget_t - 
//-----------------------------------------------------------------------------
ECommandTarget_t Cbuf_GetCurrentPlayer(void)
{
	// Always returns 'CBUF_FIRST_PLAYER' in Respawn's code.
	return ECommandTarget_t::CBUF_FIRST_PLAYER;
}

///////////////////////////////////////////////////////////////////////////////
void ConCommand_Attach()
{
	DetourAttach((LPVOID*)&ConCommandBase_IsFlagSet, &ConCommandBase::IsFlagSet);
}
void ConCommand_Detach()
{
	DetourDetach((LPVOID*)&ConCommandBase_IsFlagSet, &ConCommandBase::IsFlagSet);
}
ConCommand* g_pConCommand = new ConCommand();
