#include "core/stdafx.h"
#include "tier0/cmd.h"
#include "tier0/cvar.h"
#include "tier0/completion.h"
#include "client/client.h"
#include "engine/sys_utils.h"

//-----------------------------------------------------------------------------
// Purpose: returns max command lenght
//-----------------------------------------------------------------------------
int CCommand::MaxCommandLength(void)
{
	return COMMAND_MAX_LENGTH - 1;
}

//-----------------------------------------------------------------------------
// Purpose: returns argument count
//-----------------------------------------------------------------------------
std::int64_t CCommand::ArgC(void) const
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
// Purpose: construct/allocate
//-----------------------------------------------------------------------------
ConCommand::ConCommand(const char* pszName, const char* pszHelpString, int nFlags, void* pCallback, void* pCommandCompletionCallback)
{
	ConCommand* pCommand = reinterpret_cast<ConCommand*>(MemAlloc_Wrapper(sizeof(ConCommand))); // Allocate new memory with StdMemAlloc else we crash.
	memset(pCommand, '\0', sizeof(ConCommand)); // Set all to null.

	pCommand->m_ConCommandBase.m_pConCommandBaseVTable = g_pConCommandVtable.RCast<void*>();
	pCommand->m_ConCommandBase.m_pszName       = pszName;
	pCommand->m_ConCommandBase.m_pszHelpString = pszHelpString;
	pCommand->m_ConCommandBase.m_nFlags        = nFlags;
	pCommand->m_nNullCallBack                  = NullSub;
	pCommand->m_pCommandCallback               = pCallback;
	pCommand->m_nCallbackFlags                 = 2;
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
	// SERVER DLL                                                             |
	ConCommand* sv_kick          = new ConCommand("sv_kick", "Kick a client from the server by name. | Usage: kick \"<name>\".", FCVAR_RELEASE, _Kick_f_CompletionFunc, nullptr);
	ConCommand* sv_kickid        = new ConCommand("sv_kickid", "Kick a client from the server by UserID or OriginID | Usage: kickid \"<OriginID>\"/\"<UserID>\".", FCVAR_RELEASE, _KickID_f_CompletionFunc, nullptr);
	ConCommand* sv_ban           = new ConCommand("sv_ban", "Bans a client from the server by name. | Usage: ban <name>.", FCVAR_RELEASE, _Ban_f_CompletionFunc, nullptr);
	ConCommand* sv_banid         = new ConCommand("sv_banid", "Bans a client from the server by OriginID, UserID or IPAddress | Usage: banid \"<OriginID>\"/\"<IPAddress>/<UserID>\".", FCVAR_RELEASE, _BanID_f_CompletionFunc, nullptr);
	ConCommand* sv_unban         = new ConCommand("sv_unban", "Unbans a client from the Server by IPAddress or OriginID | Usage: unban \"<OriginID>\"/\"<IPAddress>\".", FCVAR_RELEASE, _Unban_f_CompletionFunc, nullptr);
	ConCommand* sv_reloadbanlist = new ConCommand("sv_reloadbanlist", "Reloads the ban list from the disk.", FCVAR_RELEASE, _ReloadBanList_f_CompletionFunc, nullptr);
#ifndef DEDICATED
	//-------------------------------------------------------------------------
	// CLIENT DLL                                                             |
	ConCommand* cl_showconsole = new ConCommand("cl_showconsole", "Opens the game console.", FCVAR_CLIENTDLL | FCVAR_RELEASE, _CGameConsole_f_CompletionFunc, nullptr);
	ConCommand* cl_showbrowser = new ConCommand("cl_showbrowser", "Opens the server browser.", FCVAR_CLIENTDLL | FCVAR_RELEASE, _CCompanion_f_CompletionFunc, nullptr);
	ConCommand* rcon = new ConCommand("rcon", "Forward RCON query to remote server. | Usage: rcon \"<query>\".", FCVAR_CLIENTDLL | FCVAR_RELEASE, _RCON_CmdQuery_f_CompletionFunc, nullptr);
#endif // !DEDICATED
	//-------------------------------------------------------------------------
	// FILESYSTEM API                                                         |
	ConCommand* fs_decompress_pak = new ConCommand("fs_decompress_pak", "Decompresses user specified 'vpk_dir' file.", FCVAR_DEVELOPMENTONLY, _VPK_Decompress_f_CompletionFunc, nullptr);
	//-------------------------------------------------------------------------
	// RTECH API                                                              |
	ConCommand* rtech_strtoguid  = new ConCommand("rtech_strtoguid", "Calculates the GUID from input data.", FCVAR_DEVELOPMENTONLY, _RTech_StringToGUID_f_CompletionFunc, nullptr);
	ConCommand* rtech_asyncload  = new ConCommand("rtech_asyncload", "Loads user specified 'RPak' file.", FCVAR_DEVELOPMENTONLY, _RTech_AsyncLoad_f_CompletionFunc, nullptr);
	ConCommand* rtech_decompress = new ConCommand("rtech_decompress", "Decompresses user specified 'RPak' file.", FCVAR_DEVELOPMENTONLY, _RTech_Decompress_f_CompletionFunc, nullptr);
	//-------------------------------------------------------------------------
	// NETCHANNEL                                                             |
	ConCommand* net_toggletrace = new ConCommand("net_toggletrace", "Logs the sending and receiving datagram to a file on the disk.", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, _NET_TraceNetChan_f_CompletionFunc, nullptr);
	ConCommand* net_setkey      = new ConCommand("net_setkey", "Sets user specified base64 net key.", FCVAR_RELEASE, _NET_SetKey_f_CompletionFunc, nullptr);
	ConCommand* net_generatekey = new ConCommand("net_generatekey", "Generates and sets a random base64 net key.", FCVAR_RELEASE, _NET_GenerateKey_f_CompletionFunc, nullptr);
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
	return true;
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
	if (cm_return_false_cmdquery_cheats->GetBool())
	{
		pCommandBase->RemoveFlags(FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT);
	}
	else // Mask off FCVAR_DEVELOPMENTONLY.
	{
		pCommandBase->RemoveFlags(FCVAR_DEVELOPMENTONLY);
	}
	if (cm_debug_cmdquery->GetBool())
	{
		printf(" Masked: %08X\n", pCommandBase->m_nFlags);
		printf(" Verify: %08X\n", nFlags);
		printf("--------------------------------------------------\n");
	}
	if (nFlags & FCVAR_RELEASE && !cm_return_false_cmdquery_all->GetBool())
	{
		// Default retail behaviour.
		return ConCommandBase_IsFlagSet(pCommandBase, nFlags);
	}
	if (cm_return_false_cmdquery_all->GetBool())
	{
		// Returning false on all queries may cause problems.
		return false;
	}
	// Return false on every FCVAR_DEVELOPMENTONLY || FCVAR_CHEAT query.
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
// Purpose: Returns the ConCommandBase help text.
// Output : const char*
//-----------------------------------------------------------------------------
const char* ConCommandBase::GetHelpText(void) const
{
	return m_pszHelpString;
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
