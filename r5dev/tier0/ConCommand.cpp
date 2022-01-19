#include "core/stdafx.h"
#include "tier0/cvar.h"
#include "tier0/completion.h"
#include "tier0/ConCommand.h"
#include "client/client.h"
#include "engine/sys_utils.h"

//-----------------------------------------------------------------------------
// purpose: construct/allocate
//-----------------------------------------------------------------------------
ConCommand::ConCommand(const char* pszName, const char* pszHelpString, int nFlags, void* pCallback, void* pCommandCompletionCallback)
{
	ConCommand* pCommand = reinterpret_cast<ConCommand*>(MemAlloc_Wrapper(0x68)); // Allocate new memory with StdMemAlloc else we crash.
	memset(pCommand, 0, 0x68);                                                    // Set all to null.
	std::uintptr_t pCommandBase = reinterpret_cast<std::uintptr_t>(pCommand);     // To ptr.

	*(void**)pCommandBase = g_pConCommandVtable.RCast<void*>();  // 0x00 to ConCommand vtable.
	*(const char**)(pCommandBase + 0x18) = pszName;                       // 0x18 to ConCommand Name.
	*(const char**)(pCommandBase + 0x20) = pszHelpString;                 // 0x20 to ConCommand help string.
	*(std::int32_t*)(pCommandBase + 0x38) = nFlags;                       // 0x38 to ConCommand Flags.
	*(void**)(pCommandBase + 0x40) = p_ConCommand_NullSub.RCast<void*>(); // 0x40 Nullsub since every concommand has it.
	*(void**)(pCommandBase + 0x50) = pCallback;                           // 0x50 has function callback.
	*(std::int32_t*)(pCommandBase + 0x60) = 2; // 0x60 Set to use callback and newcommand callback.

	if (pCommandCompletionCallback) // callback after execution desired?
	{
		*(void**)(pCommandBase + 0x58) = pCommandCompletionCallback; // 0x58 to our callback after execution.
	}
	else
	{
		*(void**)(pCommandBase + 0x58) = p_ConCommand_CallbackCompletion.RCast<void*>(); // 0x58 nullsub.
	}

	p_ConCommand_RegisterConCommand.RCast<void(*)(void*)>()((void*)pCommandBase); // Register command in ConVarAccessor.

	*this = *pCommand;
}

//-----------------------------------------------------------------------------
// Purpose: ConCommand registration
//-----------------------------------------------------------------------------
void ConCommand::Init(void)
{
	//-------------------------------------------------------------------------
	// SERVER DLL                                                             |
	ConCommand* sv_kick          = new ConCommand("sv_kick", "Kick a client from the server by name. | Usage: kick <name>.", FCVAR_GAMEDLL, _Kick_f_CompletionFunc, nullptr);
	ConCommand* sv_kickid        = new ConCommand("sv_kickid", "Kick a client from the server by UserID or OriginID | Usage: kickid <OriginID>/<UserID>.", FCVAR_GAMEDLL, _KickID_f_CompletionFunc, nullptr);
	ConCommand* sv_ban           = new ConCommand("sv_ban", "Bans a client from the server by name. | Usage: ban <name>.", FCVAR_GAMEDLL, _Ban_f_CompletionFunc, nullptr);
	ConCommand* sv_banid         = new ConCommand("sv_banid", "Bans a client from the server by OriginID, UserID or IPAddress | Usage: banid <OriginID>/<IPAddress>/<UserID>.", FCVAR_GAMEDLL, _BanID_f_CompletionFunc, nullptr);
	ConCommand* sv_unban         = new ConCommand("sv_unban", "Unbans a client from the Server by IPAddress or OriginID | Usage: unban <OriginID>/<IPAddress>.", FCVAR_GAMEDLL, _Unban_f_CompletionFunc, nullptr);
	ConCommand* sv_reloadbanlist = new ConCommand("sv_reloadbanlist", "Reloads the ban list from the disk.", FCVAR_GAMEDLL, _ReloadBanList_f_CompletionFunc, nullptr);
#ifndef DEDICATED
	//-------------------------------------------------------------------------
	// CLIENT DLL                                                             |
	ConCommand* cl_showconsole = new ConCommand("cl_showconsole", "Opens the game console.", FCVAR_CLIENTDLL, _CGameConsole_f_CompletionFunc, nullptr);
	ConCommand* cl_showbrowser = new ConCommand("cl_showbrowser", "Opens the server browser.", FCVAR_CLIENTDLL, _CCompanion_f_CompletionFunc, nullptr);
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
// Purpose: Checks if ConCommand has requested flags.
// Input  : nFlags - 
// Output : True if ConCommand has nFlags.
//-----------------------------------------------------------------------------
bool ConCommandBase::HasFlags(int nFlags)
{
	return m_nFlags & nFlags;
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

void ConCommand_Attach()
{
	DetourAttach((LPVOID*)&ConCommandBase_IsFlagSet, &ConCommandBase::IsFlagSet);
}

void ConCommand_Detach()
{
	DetourDetach((LPVOID*)&ConCommandBase_IsFlagSet, &ConCommandBase::IsFlagSet);
}
ConCommand* g_pConCommand = new ConCommand();
