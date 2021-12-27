#include "core/stdafx.h"
#include "tier0/cvar.h"
#include "tier0/completion.h"
#include "tier0/ConCommand.h"
#include "client/client.h"
#include "engine/sys_utils.h"

//-----------------------------------------------------------------------------
// Purpose: test each ConCommand query before execution
// Input  : *cmd - flag
// Output : true if execution is not permitted, false if permitted
//-----------------------------------------------------------------------------
bool HConCommand_IsFlagSet(ConCommandBase* pCommandBase, int nFlag)
{
	if (cm_return_false_cmdquery_cheats->m_pParent->m_iValue > 0)
	{
		if (cm_debug_cmdquery->m_pParent->m_iValue > 0)
		{
			printf("--------------------------------------------------\n");
			printf(" Flaged: %08X\n", pCommandBase->m_nFlags);
		}
		// Mask off FCVAR_CHEATS and FCVAR_DEVELOPMENTONLY.
		pCommandBase->m_nFlags &= ~(FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT);
		if (cm_debug_cmdquery->m_pParent->m_iValue > 0)
		{
			printf(" Masked: %08X\n", pCommandBase->m_nFlags);
			printf(" Verify: %08X\n", nFlag);
			printf("--------------------------------------------------\n");
		}
		if (nFlag & FCVAR_RELEASE && cm_return_false_cmdquery_all->m_pParent->m_iValue <= 0)
		{
			// Default retail behaviour.
			return ConCommand_IsFlagSet(pCommandBase, nFlag);
		}
		if (cm_return_false_cmdquery_all->m_pParent->m_iValue > 0)
		{
			// Returning false on all queries may cause problems.
			return false;
		}
		// Return false on every FCVAR_DEVELOPMENTONLY || FCVAR_CHEAT query.
		return (pCommandBase->m_nFlags & nFlag) != 0;
	}
	else
	{
		if (cm_debug_cmdquery->m_pParent->m_iValue > 0)
		{
			printf("--------------------------------------------------\n");
			printf(" Flaged: %08X\n", pCommandBase->m_nFlags);
		}
		// Mask off FCVAR_DEVELOPMENTONLY.
		pCommandBase->m_nFlags &= ~(FCVAR_DEVELOPMENTONLY);
		if (cm_debug_cmdquery->m_pParent->m_iValue > 0)
		{
			printf(" Masked: %08X\n", pCommandBase->m_nFlags);
			printf(" Verify: %08X\n", nFlag);
			printf("--------------------------------------------------\n");
		}
		if (nFlag & FCVAR_RELEASE && cm_return_false_cmdquery_all->m_pParent->m_iValue <= 0)
		{
			// Default retail behaviour.
			return ConCommand_IsFlagSet(pCommandBase, nFlag);
		}
		if (cm_return_false_cmdquery_all->m_pParent->m_iValue > 0)
		{
			// Returning false on all queries may cause problems.
			return false;
		}
		// Return false on every FCVAR_DEVELOPMENTONLY query.
		return (pCommandBase->m_nFlags & nFlag) != 0;
	}
	// Default behaviour.
	return ConCommand_IsFlagSet(pCommandBase, nFlag);
}

//-----------------------------------------------------------------------------
// Purpose: register ConCommand's
//-----------------------------------------------------------------------------
void* ConCommand_RegisterCommand(const char* szName, const char* szHelpString, int nFlags, void* pCallback, void* pCommandCompletionCallback)
{
	void* pCommand = reinterpret_cast<void*>(MemAlloc_Wrapper(0x68));         // Allocate new memory with StdMemAlloc else we crash.
	memset(pCommand, 0, 0x68);                                                // Set all to null.
	std::uintptr_t pCommandBase = reinterpret_cast<std::uintptr_t>(pCommand); // To ptr.

	*(void**)pCommandBase                 = g_pConCommandVtable.RCast<void*>();  // 0x00 to ConCommand vtable.
	*(const char**)(pCommandBase + 0x18)  = szName;                              // 0x18 to ConCommand Name.
	*(const char**)(pCommandBase + 0x20)  = szHelpString;                        // 0x20 to ConCommand help string.
	*(std::int32_t*)(pCommandBase + 0x38) = nFlags;                              // 0x38 to ConCommand Flags.
	*(void**)(pCommandBase + 0x40)        = p_ConCommand_NullSub.RCast<void*>(); // 0x40 Nullsub since every concommand has it.
	*(void**)(pCommandBase + 0x50)        = pCallback;                           // 0x50 has function callback.
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

	return pCommand;
}

//-----------------------------------------------------------------------------
// Purpose: ConCommand definitions to be registered
//-----------------------------------------------------------------------------
void ConCommand_InitConCommand()
{
	//-------------------------------------------------------------------------
	// SERVER DLL                                                             |
	void* sv_kick          = ConCommand_RegisterCommand("sv_kick", "Kick a client from the server by name. | Usage: kick <name>.", 0, _Kick_f_CompletionFunc, nullptr);
	void* sv_kickid        = ConCommand_RegisterCommand("sv_kickid", "Kick a client from the server by UserID or OriginID | Usage: kickid <OriginID>/<UserID>.", 0, _KickID_f_CompletionFunc, nullptr);
	void* sv_ban           = ConCommand_RegisterCommand("sv_ban", "Bans a client from the server by name. | Usage: ban <name>.", 0, _Ban_f_CompletionFunc, nullptr);
	void* sv_banid         = ConCommand_RegisterCommand("sv_banid", "Bans a client from the server by OriginID, UserID or IPAddress | Usage: banid <OriginID>/<IPAddress>/<UserID>.", 0, _BanID_f_CompletionFunc, nullptr);
	void* sv_unban         = ConCommand_RegisterCommand("sv_unban", "Unbans a client from the Server by IPAddress or OriginID | Usage: unban <OriginID>/<IPAddress>.", 0, _Unban_f_CompletionFunc, nullptr);
	void* sv_reloadbanlist = ConCommand_RegisterCommand("sv_reloadbanlist", "Reloads the ban list from the disk.", 0, _ReloadBanList_f_CompletionFunc, nullptr);
#ifndef DEDICATED
	//-------------------------------------------------------------------------
	// CLIENT DLL                                                             |
	void* cl_showconsole = ConCommand_RegisterCommand("cl_showconsole", "Opens the game console.", 0, _CGameConsole_f_CompletionFunc, nullptr);
	void* cl_showbrowser = ConCommand_RegisterCommand("cl_showbrowser", "Opens the server browser.", 0, _CCompanion_f_CompletionFunc, nullptr);
#endif // !DEDICATED
	//-------------------------------------------------------------------------
	// FILESYSTEM API                                                         |
	void* fs_decompress_pak = ConCommand_RegisterCommand("fs_decompress_pak", "Decompresses user specified 'vpk_dir' file.", 0, _VPK_Decompress_f_CompletionFunc, nullptr);
	//-------------------------------------------------------------------------
	// RTECH API                                                              |
	void* rtech_strtoguid  = ConCommand_RegisterCommand("rtech_strtoguid", "Calculates the GUID from input data.", 0, _RTech_StringToGUID_f_CompletionFunc, nullptr);
	void* rtech_asyncload  = ConCommand_RegisterCommand("rtech_asyncload", "Loads user specified 'RPak' file.", 0, _RTech_AsyncLoad_f_CompletionFunc, nullptr);
	void* rtech_decompress = ConCommand_RegisterCommand("rtech_decompress", "Decompresses user specified 'RPak' file.", 0, _RTech_Decompress_f_CompletionFunc, nullptr);
	//-------------------------------------------------------------------------
	// NETCHANNEL                                                             |
	void* net_toggletrace = ConCommand_RegisterCommand("net_toggletrace", "Logs the sending and receiving datagram to a file on the disk.", 0, _NET_TraceNetChan_f_CompletionFunc, nullptr);
	void* net_setkey      = ConCommand_RegisterCommand("net_setkey", "Sets user specified base64 net key.", 0, _NET_SetKey_f_CompletionFunc, nullptr);
	void* net_generatekey = ConCommand_RegisterCommand("net_generatekey", "Generates and sets a random base64 net key.", 0, _NET_GenerateKey_f_CompletionFunc, nullptr);
}

void ConCommand_Attach()
{
	DetourAttach((LPVOID*)&ConCommand_IsFlagSet, &HConCommand_IsFlagSet);
}

void ConCommand_Detach()
{
	DetourDetach((LPVOID*)&ConCommand_IsFlagSet, &HConCommand_IsFlagSet);
}
