//=============================================================================//
//
// Purpose: Console Commands
//
//=============================================================================//

#include "core/stdafx.h"
#include "tier0/tslist.h"
#include "tier0/memstd.h"
#include "tier0/commandline.h"
#include "tier1/cmd.h"
#include "tier1/cvar.h"
#include "tier1/characterset.h"
#include "tier1/utlstring.h"
#include "public/const.h"
#include "vstdlib/completion.h"
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
	ConCommand* pCommand = MemAllocSingleton()->Alloc<ConCommand>(sizeof(ConCommand));
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
// Purpose: ConCommand registration
//-----------------------------------------------------------------------------
void ConCommand::StaticInit(void)
{
	//-------------------------------------------------------------------------
	// ENGINE DLL                                                             |
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1)
	ConCommand::StaticCreate("bhit", "Bullet-hit trajectory debug.", nullptr, FCVAR_DEVELOPMENTONLY | FCVAR_GAMEDLL, BHit_f, nullptr);
#endif // !GAMEDLL_S0 && !GAMEDLL_S1
#ifndef DEDICATED
	ConCommand::StaticCreate("line", "Draw a debug line.",       nullptr, FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, Line_f, nullptr);
	ConCommand::StaticCreate("sphere", "Draw a debug sphere.",   nullptr, FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, Sphere_f, nullptr);
	ConCommand::StaticCreate("capsule", "Draw a debug capsule.", nullptr, FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, Capsule_f, nullptr);
#endif //!DEDICATED
	ConCommand::StaticCreate("con_help", "Shows the colors and description of each context.", nullptr, FCVAR_RELEASE, CON_Help_f, nullptr);
#ifndef CLIENT_DLL
	ConCommand::StaticCreate("reload_playlists", "Reloads the playlists file.", nullptr, FCVAR_RELEASE, Host_ReloadPlaylists_f, nullptr);
#endif // !CLIENT_DLL
	//-------------------------------------------------------------------------
	// SERVER DLL                                                             |
#ifndef CLIENT_DLL
	ConCommand::StaticCreate("script", "Run input code as SERVER script on the VM.", nullptr, FCVAR_GAMEDLL | FCVAR_CHEAT, SQVM_ServerScript_f, nullptr);
	ConCommand::StaticCreate("sv_kick", "Kick a client from the server by user name.", "sv_kick \"<userId>\"", FCVAR_RELEASE, Host_Kick_f, nullptr);
	ConCommand::StaticCreate("sv_kickid", "Kick a client from the server by handle, nucleus id or ip address.", "sv_kickid \"<handle>\"/\"<nucleusId>/<ipAddress>\"", FCVAR_RELEASE, Host_KickID_f, nullptr);
	ConCommand::StaticCreate("sv_ban", "Bans a client from the server by user name.", "sv_ban <userId>", FCVAR_RELEASE, Host_Ban_f, nullptr);
	ConCommand::StaticCreate("sv_banid", "Bans a client from the server by handle, nucleus id or ip address.", "sv_banid \"<handle>\"/\"<nucleusId>/<ipAddress>\"", FCVAR_RELEASE, Host_BanID_f, nullptr);
	ConCommand::StaticCreate("sv_unban", "Unbans a client from the server by nucleus id or ip address.", "sv_unban \"<nucleusId>\"/\"<ipAddress>\"", FCVAR_RELEASE, Host_Unban_f, nullptr);
	ConCommand::StaticCreate("sv_reloadbanlist", "Reloads the banned list.", nullptr, FCVAR_RELEASE, Host_ReloadBanList_f, nullptr);
	ConCommand::StaticCreate("sv_addbot", "Creates a bot on the server.", nullptr, FCVAR_RELEASE, CC_CreateFakePlayer_f, nullptr);
	ConCommand::StaticCreate("navmesh_hotswap", "Hot swap the NavMesh for all hulls.", nullptr, FCVAR_DEVELOPMENTONLY, Detour_HotSwap_f, nullptr);
#endif // !CLIENT_DLL
#ifndef DEDICATED
	//-------------------------------------------------------------------------
	// CLIENT DLL                                                             |
	ConCommand::StaticCreate("script_client", "Run input code as CLIENT script on the VM.", nullptr, FCVAR_CLIENTDLL | FCVAR_CHEAT, SQVM_ClientScript_f, nullptr);
	ConCommand::StaticCreate("rcon", "Forward RCON query to remote server.", "rcon \"<query>\"", FCVAR_CLIENTDLL | FCVAR_RELEASE, RCON_CmdQuery_f, nullptr);
	ConCommand::StaticCreate("rcon_disconnect", "Disconnect from RCON server.", nullptr, FCVAR_CLIENTDLL | FCVAR_RELEASE, RCON_Disconnect_f, nullptr);

	ConCommand::StaticCreate("con_history", "Shows the developer console submission history.", nullptr, FCVAR_CLIENTDLL | FCVAR_RELEASE, CON_LogHistory_f, nullptr);
	ConCommand::StaticCreate("con_removeline", "Removes a range of lines from the developer console.", nullptr, FCVAR_CLIENTDLL | FCVAR_RELEASE, CON_RemoveLine_f, nullptr);
	ConCommand::StaticCreate("con_clearlines", "Clears all lines from the developer console.", nullptr, FCVAR_CLIENTDLL | FCVAR_RELEASE, CON_ClearLines_f, nullptr);
	ConCommand::StaticCreate("con_clearhistory", "Clears all submissions from the developer console history.", nullptr, FCVAR_CLIENTDLL | FCVAR_RELEASE, CON_ClearHistory_f, nullptr);

	ConCommand::StaticCreate("toggleconsole", "Show/hide the developer console.", nullptr, FCVAR_CLIENTDLL | FCVAR_RELEASE, ToggleConsole_f, nullptr);
	ConCommand::StaticCreate("togglebrowser", "Show/hide the server browser.", nullptr, FCVAR_CLIENTDLL | FCVAR_RELEASE, ToggleBrowser_f, nullptr);
	//-------------------------------------------------------------------------
	// UI DLL                                                                 |
	ConCommand::StaticCreate("script_ui", "Run input code as UI script on the VM.", nullptr, FCVAR_CLIENTDLL | FCVAR_CHEAT, SQVM_UIScript_f, nullptr);
#endif // !DEDICATED
	//-------------------------------------------------------------------------
	// FILESYSTEM API                                                         |
	ConCommand::StaticCreate("fs_vpk_mount",  "Mount a VPK file for FileSystem usage.", nullptr, FCVAR_DEVELOPMENTONLY, VPK_Mount_f, nullptr);
	ConCommand::StaticCreate("fs_vpk_unmount",  "Unmount a VPK file and clear its cache.", nullptr, FCVAR_DEVELOPMENTONLY, VPK_Unmount_f, nullptr);
	ConCommand::StaticCreate("fs_vpk_build",  "Build a VPK file from current workspace.", nullptr, FCVAR_DEVELOPMENTONLY, VPK_Pack_f, nullptr);
	ConCommand::StaticCreate("fs_vpk_unpack", "Unpack all files from a VPK file.", nullptr, FCVAR_DEVELOPMENTONLY, VPK_Unpack_f, nullptr);
	//-------------------------------------------------------------------------
	// RTECH API                                                              |
	ConCommand::StaticCreate("rtech_strtoguid", "Calculates the GUID from input data.", nullptr, FCVAR_DEVELOPMENTONLY, RTech_StringToGUID_f, nullptr);
	ConCommand::StaticCreate("pak_decompress", "Decompresses specified RPAK file.", nullptr, FCVAR_DEVELOPMENTONLY, RTech_Decompress_f, RTech_PakDecompress_f_CompletionFunc);
	ConCommand::StaticCreate("pak_requestload", "Requests asynchronous load for specified RPAK file.", nullptr, FCVAR_DEVELOPMENTONLY, Pak_RequestLoad_f, RTech_PakLoad_f_CompletionFunc);
	ConCommand::StaticCreate("pak_requestunload", "Requests unload for specified RPAK file or ID.", nullptr, FCVAR_DEVELOPMENTONLY, Pak_RequestUnload_f, RTech_PakUnload_f_CompletionFunc);
	ConCommand::StaticCreate("pak_swap", "Requests swap for specified RPAK file or ID", nullptr, FCVAR_DEVELOPMENTONLY, Pak_Swap_f, nullptr);
	ConCommand::StaticCreate("pak_listpaks", "Display a list of the loaded Pak files.", nullptr, FCVAR_RELEASE, Pak_ListPaks_f, nullptr);
	ConCommand::StaticCreate("pak_listtypes", "Display a list of the registered asset types.", nullptr, FCVAR_RELEASE, Pak_ListTypes_f, nullptr);
	//-------------------------------------------------------------------------
	// NETCHANNEL                                                             |
	ConCommand::StaticCreate("net_setkey", "Sets user specified base64 net key.", nullptr, FCVAR_RELEASE, NET_SetKey_f, nullptr);
	ConCommand::StaticCreate("net_generatekey", "Generates and sets a random base64 net key.", nullptr, FCVAR_RELEASE, NET_GenerateKey_f, nullptr);
	//-------------------------------------------------------------------------
	// TIER0                                                                  |
	ConCommand::StaticCreate("sig_getadr", "Logs the sigscan results to the console.", nullptr, FCVAR_DEVELOPMENTONLY | FCVAR_HIDDEN, SIG_GetAdr_f, nullptr);
}

//-----------------------------------------------------------------------------
// Purpose: shipped ConCommand initialization
//-----------------------------------------------------------------------------
void ConCommand::InitShipped(void)
{
	///------------------------------------------------------ [ CALLBACK SWAP ]
	//-------------------------------------------------------------------------
	// ENGINE DLL                                                             |
	ConCommand* changelevel = g_pCVar->FindCommand("changelevel");
	ConCommand* map = g_pCVar->FindCommand("map");
	ConCommand* map_background = g_pCVar->FindCommand("map_background");
	ConCommand* ss_map = g_pCVar->FindCommand("ss_map");
	ConCommand* migrateme = g_pCVar->FindCommand("migrateme");
	ConCommand* help = g_pCVar->FindCommand("help");
	ConCommand* convar_list =  g_pCVar->FindCommand("convar_list");
	ConCommand* convar_differences = g_pCVar->FindCommand("convar_differences");
	ConCommand* convar_findByFlags = g_pCVar->FindCommand("convar_findByFlags");
#ifndef DEDICATED
	//-------------------------------------------------------------------------
	// MATERIAL SYSTEM
	ConCommand* mat_crosshair = g_pCVar->FindCommand("mat_crosshair"); // Patch callback function to working callback.
	//-------------------------------------------------------------------------
	// CLIENT DLL                                                             |
	ConCommand* give = g_pCVar->FindCommand("give");
#endif // !DEDICATED

	help->m_fnCommandCallback = CVHelp_f;
	convar_list->m_fnCommandCallback = CVList_f;
	convar_differences->m_fnCommandCallback = CVDiff_f;
	convar_findByFlags->m_fnCommandCallback = CVFlag_f;
#ifndef CLIENT_DLL
	changelevel->m_fnCommandCallback = Host_Changelevel_f;
#endif // !CLIENT_DLL
	changelevel->m_fnCompletionCallback = Host_Changelevel_f_CompletionFunc;

	map->m_fnCompletionCallback = Host_Map_f_CompletionFunc;
	map_background->m_fnCompletionCallback = Host_Background_f_CompletionFunc;
	ss_map->m_fnCompletionCallback = Host_SSMap_f_CompletionFunc;

#ifndef DEDICATED
	mat_crosshair->m_fnCommandCallback = Mat_CrossHair_f;
	give->m_fnCompletionCallback = Game_Give_f_CompletionFunc;
#endif // !DEDICATED

	/// ------------------------------------------------------ [ FLAG REMOVAL ]
	//-------------------------------------------------------------------------
	if (!CommandLine()->CheckParm("-devsdk"))
	{
		const char* pszMaskedBases[] =
		{
#ifndef DEDICATED
			"connect",
			"connectAsSpectator",
			"connectWithKey",
			"silentconnect",
			"set",
			"ping",
#endif // !DEDICATED
			"launchplaylist",
			"quit",
			"exit",
			"reload",
			"restart",
			"status",
			"version",
		};

		for (size_t i = 0; i < SDK_ARRAYSIZE(pszMaskedBases); i++)
		{
			if (ConCommandBase* pCommandBase = g_pCVar->FindCommandBase(pszMaskedBases[i]))
			{
				pCommandBase->RemoveFlags(FCVAR_DEVELOPMENTONLY);
			}
		}

		convar_list->RemoveFlags(FCVAR_DEVELOPMENTONLY);
		convar_differences->RemoveFlags(FCVAR_DEVELOPMENTONLY);
		convar_findByFlags->RemoveFlags(FCVAR_DEVELOPMENTONLY);
		help->RemoveFlags(FCVAR_DEVELOPMENTONLY);
		migrateme->RemoveFlags(FCVAR_SERVER_CAN_EXECUTE);
		changelevel->RemoveFlags(FCVAR_DEVELOPMENTONLY);
		map->RemoveFlags(FCVAR_DEVELOPMENTONLY|FCVAR_SERVER_CAN_EXECUTE);
		map_background->RemoveFlags(FCVAR_DEVELOPMENTONLY|FCVAR_SERVER_CAN_EXECUTE);
		ss_map->RemoveFlags(FCVAR_DEVELOPMENTONLY|FCVAR_SERVER_CAN_EXECUTE);
	}
}

//-----------------------------------------------------------------------------
// Purpose: unregister extraneous ConCommand's.
//-----------------------------------------------------------------------------
void ConCommand::PurgeShipped(void)
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

	for (size_t i = 0; i < SDK_ARRAYSIZE(pszCommandToRemove); i++)
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
// Purpose: create
//-----------------------------------------------------------------------------
ConVar* ConVar::StaticCreate(const char* pszName, const char* pszDefaultValue, int nFlags, const char* pszHelpString, bool bMin, float fMin, bool bMax, float fMax, FnChangeCallback_t pCallback, const char* pszUsageString)
{
	ConVar* pNewConVar = MemAllocSingleton()->Alloc<ConVar>(sizeof(ConVar)); // Allocate new memory with StdMemAlloc else we crash.

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

	v_ConVar_Register(pNewConVar, pszName, pszDefaultValue, nFlags, pszHelpString, bMin, fMin, bMax, fMax, pCallback, pszUsageString);
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
//		MemAllocSingleton()->Free(m_Value.m_pszString);
//		m_Value.m_pszString = NULL;
//	}
//}

//-----------------------------------------------------------------------------
// Purpose: initialize ConVar's
//-----------------------------------------------------------------------------
void ConVar::Init(void)
{
	//-------------------------------------------------------------------------
	// ENGINE                                                                 |
	hostdesc                       = ConVar::StaticCreate("hostdesc", "", FCVAR_RELEASE, "Host game server description.", false, 0.f, false, 0.f, nullptr, nullptr);
	sdk_fixedframe_tickinterval    = ConVar::StaticCreate("sdk_fixedframe_tickinterval", "0.02", FCVAR_RELEASE, "The tick interval used by the SDK fixed frame.", false, 0.f, false, 0.f, nullptr, nullptr);
	staticProp_defaultBuildFrustum = ConVar::StaticCreate("staticProp_defaultBuildFrustum", "0", FCVAR_DEVELOPMENTONLY, "Use the old solution for building static prop frustum culling.", false, 0.f, false, 0.f, nullptr, nullptr);

	curl_debug      = ConVar::StaticCreate("curl_debug"     , "0" , FCVAR_DEVELOPMENTONLY, "Determines whether or not to enable curl debug logging.", false, 0.f, false, 0.f, nullptr, "1 = curl logs; 0 (zero) = no logs.");
	curl_timeout    = ConVar::StaticCreate("curl_timeout"   , "15", FCVAR_DEVELOPMENTONLY, "Maximum time in seconds a curl transfer operation could take.", false, 0.f, false, 0.f, nullptr, nullptr);
	ssl_verify_peer = ConVar::StaticCreate("ssl_verify_peer", "1" , FCVAR_DEVELOPMENTONLY, "Verify the authenticity of the peer's SSL certificate.", false, 0.f, false, 0.f, nullptr, "1 = curl verifies; 0 (zero) = no verification.");

	rcon_address  = ConVar::StaticCreate("rcon_address",  "localhost", FCVAR_SERVER_CANNOT_QUERY | FCVAR_DONTRECORD | FCVAR_RELEASE, "Remote server access address.", false, 0.f, false, 0.f, nullptr, nullptr);
	rcon_password = ConVar::StaticCreate("rcon_password", ""         , FCVAR_SERVER_CANNOT_QUERY | FCVAR_DONTRECORD | FCVAR_RELEASE, "Remote server access password (rcon is disabled if empty).", false, 0.f, false, 0.f, &RCON_PasswordChanged_f, nullptr);

	r_debug_overlay_nodecay        = ConVar::StaticCreate("r_debug_overlay_nodecay"       , "0", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Keeps all debug overlays alive regardless of their lifetime. Use command 'clear_debug_overlays' to clear everything.", false, 0.f, false, 0.f, nullptr, nullptr);
	r_debug_overlay_invisible      = ConVar::StaticCreate("r_debug_overlay_invisible"     , "1", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Show invisible debug overlays (alpha < 1 = 255).", false, 0.f, false, 0.f, nullptr, nullptr);
	r_debug_overlay_wireframe      = ConVar::StaticCreate("r_debug_overlay_wireframe"     , "1", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Use wireframe in debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	r_debug_draw_depth_test        = ConVar::StaticCreate("r_debug_draw_depth_test"       , "1", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Toggle depth test for other debug draw functionality.", false, 0.f, false, 0.f, nullptr, nullptr);
	r_drawWorldMeshes              = ConVar::StaticCreate("r_drawWorldMeshes"             , "1", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Render world meshes.", false, 0.f, false, 0.f, nullptr, nullptr);
	r_drawWorldMeshesDepthOnly     = ConVar::StaticCreate("r_drawWorldMeshesDepthOnly"    , "1", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Render world meshes (depth only).", false, 0.f, false, 0.f, nullptr, nullptr);
	r_drawWorldMeshesDepthAtTheEnd = ConVar::StaticCreate("r_drawWorldMeshesDepthAtTheEnd", "1", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Render world meshes (depth at the end).", false, 0.f, false, 0.f, nullptr, nullptr);
	//-------------------------------------------------------------------------
	// SERVER                                                                 |
#ifndef CLIENT_DLL
	ai_ainDumpOnLoad             = ConVar::StaticCreate("ai_ainDumpOnLoad"            , "0", FCVAR_DEVELOPMENTONLY, "Dumps AIN data from node graphs loaded from the disk on load.", false, 0.f, false, 0.f, nullptr, nullptr);
	ai_ainDebugConnect           = ConVar::StaticCreate("ai_ainDebugConnect"          , "0", FCVAR_DEVELOPMENTONLY, "Debug AIN node connections.", false, 0.f, false, 0.f, nullptr, nullptr);
	ai_script_nodes_draw_range   = ConVar::StaticCreate("ai_script_nodes_draw_range"  , "0", FCVAR_DEVELOPMENTONLY, "Debug draw AIN script nodes ranging from shift index to this cvar.", false, 0.f, false, 0.f, nullptr, nullptr);
	ai_script_nodes_draw_nearest = ConVar::StaticCreate("ai_script_nodes_draw_nearest", "1", FCVAR_DEVELOPMENTONLY, "Debug draw AIN script node links to nearest node (build order is used if null).", false, 0.f, false, 0.f, nullptr, nullptr);

	navmesh_always_reachable   = ConVar::StaticCreate("navmesh_always_reachable"   , "0"    , FCVAR_DEVELOPMENTONLY, "Marks goal poly from agent poly as reachable regardless of table data ( !slower! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	navmesh_debug_type         = ConVar::StaticCreate("navmesh_debug_type"         , "0"    , FCVAR_DEVELOPMENTONLY, "NavMesh debug draw hull index.", true, 0.f, true, 4.f, nullptr, "0 = small, 1 = med_short, 2 = medium, 3 = large, 4 = extra large");
	navmesh_debug_tile_range   = ConVar::StaticCreate("navmesh_debug_tile_range"   , "0"    , FCVAR_DEVELOPMENTONLY, "NavMesh debug draw tiles ranging from shift index to this cvar.", true, 0.f, false, 0.f, nullptr, nullptr);
	navmesh_debug_camera_range = ConVar::StaticCreate("navmesh_debug_camera_range" , "2000" , FCVAR_DEVELOPMENTONLY, "Only debug draw tiles within this distance from camera origin.", true, 0.f, false, 0.f, nullptr, nullptr);
#ifndef DEDICATED
	navmesh_draw_bvtree            = ConVar::StaticCreate("navmesh_draw_bvtree"            , "-1", FCVAR_DEVELOPMENTONLY, "Draws the BVTree of the NavMesh tiles.", false, 0.f, false, 0.f, nullptr, "Index: > 0 && < mesh->m_tileCount");
	navmesh_draw_portal            = ConVar::StaticCreate("navmesh_draw_portal"            , "-1", FCVAR_DEVELOPMENTONLY, "Draws the portal of the NavMesh tiles.", false, 0.f, false, 0.f, nullptr, "Index: > 0 && < mesh->m_tileCount");
	navmesh_draw_polys             = ConVar::StaticCreate("navmesh_draw_polys"             , "-1", FCVAR_DEVELOPMENTONLY, "Draws the polys of the NavMesh tiles.", false, 0.f, false, 0.f, nullptr, "Index: > 0 && < mesh->m_tileCount");
	navmesh_draw_poly_bounds       = ConVar::StaticCreate("navmesh_draw_poly_bounds"       , "-1", FCVAR_DEVELOPMENTONLY, "Draws the bounds of the NavMesh polys.", false, 0.f, false, 0.f, nullptr, "Index: > 0 && < mesh->m_tileCount");
	navmesh_draw_poly_bounds_inner = ConVar::StaticCreate("navmesh_draw_poly_bounds_inner" , "0" , FCVAR_DEVELOPMENTONLY, "Draws the inner bounds of the NavMesh polys (requires navmesh_draw_poly_bounds).", false, 0.f, false, 0.f, nullptr, "Index: > 0 && < mesh->m_tileCount");
#endif // !DEDICATED
	sv_showconnecting  = ConVar::StaticCreate("sv_showconnecting" , "1", FCVAR_RELEASE, "Logs information about the connecting client to the console.", false, 0.f, false, 0.f, nullptr, nullptr);
	sv_globalBanlist   = ConVar::StaticCreate("sv_globalBanlist"  , "1", FCVAR_RELEASE, "Determines whether or not to use the global banned list.", false, 0.f, false, 0.f, nullptr, "0 = Disable, 1 = Enable.");
	sv_pylonVisibility = ConVar::StaticCreate("sv_pylonVisibility", "0", FCVAR_RELEASE, "Determines the visibility to the Pylon master server.", false, 0.f, false, 0.f, nullptr, "0 = Offline, 1 = Hidden, 2 = Public.");
	sv_pylonRefreshRate   = ConVar::StaticCreate("sv_pylonRefreshRate"  , "5.0", FCVAR_RELEASE, "Pylon host refresh rate (seconds).", true, 2.f, true, 8.f, nullptr, nullptr);
	sv_banlistRefreshRate = ConVar::StaticCreate("sv_banlistRefreshRate", "1.0", FCVAR_RELEASE, "Banned list refresh rate (seconds).", true, 1.f, false, 0.f, nullptr, nullptr);
	sv_statusRefreshRate  = ConVar::StaticCreate("sv_statusRefreshRate" , "0.5", FCVAR_RELEASE, "Server status refresh rate (seconds).", false, 0.f, false, 0.f, nullptr, nullptr);
	sv_autoReloadRate     = ConVar::StaticCreate("sv_autoReloadRate"    , "0"  , FCVAR_RELEASE, "Time in seconds between each server auto-reload (disabled if null). ", true, 0.f, false, 0.f, nullptr, nullptr);
	sv_quota_stringCmdsPerSecond = ConVar::StaticCreate("sv_quota_stringCmdsPerSecond", "16", FCVAR_RELEASE, "How many string commands per second clients are allowed to submit, 0 to disallow all string commands.", true, 0.f, false, 0.f, nullptr, nullptr);
	sv_simulateBots = ConVar::StaticCreate("sv_simulateBots", "1", FCVAR_RELEASE, "Simulate user commands for bots on the server.", true, 0.f, false, 0.f, nullptr, nullptr);

	sv_rcon_debug       = ConVar::StaticCreate("sv_rcon_debug"      , "0" , FCVAR_RELEASE, "Show rcon debug information ( !slower! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	sv_rcon_sendlogs    = ConVar::StaticCreate("sv_rcon_sendlogs"   , "0" , FCVAR_RELEASE, "Network console logs to connected and authenticated sockets.", false, 0.f, false, 0.f, nullptr, nullptr);
	sv_rcon_banpenalty  = ConVar::StaticCreate("sv_rcon_banpenalty" , "10", FCVAR_RELEASE, "Number of minutes to ban users who fail rcon authentication.", false, 0.f, false, 0.f, nullptr, nullptr);
	sv_rcon_maxfailures = ConVar::StaticCreate("sv_rcon_maxfailures", "10", FCVAR_RELEASE, "Max number of times a user can fail rcon authentication before being banned.", true, 1.f, false, 0.f, nullptr, nullptr);
	sv_rcon_maxignores  = ConVar::StaticCreate("sv_rcon_maxignores" , "15", FCVAR_RELEASE, "Max number of times a user can ignore the instruction message before being banned.", true, 1.f, false, 0.f, nullptr, nullptr);
	sv_rcon_maxsockets  = ConVar::StaticCreate("sv_rcon_maxsockets" , "32", FCVAR_RELEASE, "Max number of accepted sockets before the server starts closing redundant sockets.", true, 1.f, false, 0.f, nullptr, nullptr);
	sv_rcon_whitelist_address = ConVar::StaticCreate("sv_rcon_whitelist_address", "", FCVAR_RELEASE, "This address is not considered a 'redundant' socket and will never be banned for failed authentication attempts.", false, 0.f, false, 0.f, nullptr, "Format: '::ffff:127.0.0.1'");
#endif // !CLIENT_DLL
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1)
	bhit_depth_test = ConVar::StaticCreate("bhit_depth_test", "0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Use depth test for bullet ray trace overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	bhit_abs_origin = ConVar::StaticCreate("bhit_abs_origin", "1", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Draw entity's predicted abs origin upon bullet impact for trajectory debugging (requires 'r_visualizetraces' to be set!).", false, 0.f, false, 0.f, nullptr, nullptr);
#endif // !GAMEDLL_S0 && !GAMEDLL_S1
	//-------------------------------------------------------------------------
	// CLIENT                                                                 |
#ifndef DEDICATED
	cl_rcon_request_sendlogs = ConVar::StaticCreate("cl_rcon_request_sendlogs", "1" , FCVAR_RELEASE, "Request the rcon server to send console logs on connect.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_quota_stringCmdsPerSecond = ConVar::StaticCreate("cl_quota_stringCmdsPerSecond", "16" , FCVAR_RELEASE, "How many string commands per second user is allowed to submit, 0 to allow all submissions.", true, 0.f, false, 0.f, nullptr, nullptr);

	cl_notify_invert_x = ConVar::StaticCreate("cl_notify_invert_x", "0", FCVAR_DEVELOPMENTONLY, "Inverts the X offset for console notify debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_notify_invert_y = ConVar::StaticCreate("cl_notify_invert_y", "0", FCVAR_DEVELOPMENTONLY, "Inverts the Y offset for console notify debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_notify_offset_x = ConVar::StaticCreate("cl_notify_offset_x", "10", FCVAR_DEVELOPMENTONLY, "X offset for console notify debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_notify_offset_y = ConVar::StaticCreate("cl_notify_offset_y", "10", FCVAR_DEVELOPMENTONLY, "Y offset for console notify debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);

	cl_showsimstats      = ConVar::StaticCreate("cl_showsimstats"     , "0"  , FCVAR_DEVELOPMENTONLY, "Shows the tick counter for the server/client simulation and the render frame.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_simstats_invert_x = ConVar::StaticCreate("cl_simstats_invert_x", "1"  , FCVAR_DEVELOPMENTONLY, "Inverts the X offset for simulation debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_simstats_invert_y = ConVar::StaticCreate("cl_simstats_invert_y", "1"  , FCVAR_DEVELOPMENTONLY, "Inverts the Y offset for simulation debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_simstats_offset_x = ConVar::StaticCreate("cl_simstats_offset_x", "650", FCVAR_DEVELOPMENTONLY, "X offset for simulation debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_simstats_offset_y = ConVar::StaticCreate("cl_simstats_offset_y", "120", FCVAR_DEVELOPMENTONLY, "Y offset for simulation debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);

	cl_showgpustats      = ConVar::StaticCreate("cl_showgpustats"     , "0", FCVAR_DEVELOPMENTONLY, "Texture streaming debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_gpustats_invert_x = ConVar::StaticCreate("cl_gpustats_invert_x", "1", FCVAR_DEVELOPMENTONLY, "Inverts the X offset for texture streaming debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_gpustats_invert_y = ConVar::StaticCreate("cl_gpustats_invert_y", "1", FCVAR_DEVELOPMENTONLY, "Inverts the Y offset for texture streaming debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_gpustats_offset_x = ConVar::StaticCreate("cl_gpustats_offset_x", "650", FCVAR_DEVELOPMENTONLY, "X offset for texture streaming debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_gpustats_offset_y = ConVar::StaticCreate("cl_gpustats_offset_y", "105", FCVAR_DEVELOPMENTONLY, "Y offset for texture streaming debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);

	cl_showmaterialinfo      = ConVar::StaticCreate("cl_showmaterialinfo"     , "0"  , FCVAR_DEVELOPMENTONLY, "Draw info for the material under the crosshair on screen.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_materialinfo_offset_x = ConVar::StaticCreate("cl_materialinfo_offset_x", "0"  , FCVAR_DEVELOPMENTONLY, "X offset for material debug info overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_materialinfo_offset_y = ConVar::StaticCreate("cl_materialinfo_offset_y", "420", FCVAR_DEVELOPMENTONLY, "Y offset for material debug info overlay.", false, 0.f, false, 0.f, nullptr, nullptr);

	con_drawnotify = ConVar::StaticCreate("con_drawnotify", "0", FCVAR_RELEASE, "Draws the RUI console to the hud.", false, 0.f, false, 0.f, nullptr, nullptr);
	con_notifylines     = ConVar::StaticCreate("con_notifylines"    , "3" , FCVAR_MATERIAL_SYSTEM_THREAD, "Number of console lines to overlay for debugging.", true, 1.f, false, 0.f, nullptr, nullptr);
	con_notifytime      = ConVar::StaticCreate("con_notifytime"     , "6" , FCVAR_MATERIAL_SYSTEM_THREAD, "How long to display recent console text to the upper part of the game window.", false, 1.f, false, 50.f, nullptr, nullptr);

	con_notify_invert_x = ConVar::StaticCreate("con_notify_invert_x", "0" , FCVAR_MATERIAL_SYSTEM_THREAD, "Inverts the X offset for RUI console overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	con_notify_invert_y = ConVar::StaticCreate("con_notify_invert_y", "0" , FCVAR_MATERIAL_SYSTEM_THREAD, "Inverts the Y offset for RUI console overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	con_notify_offset_x = ConVar::StaticCreate("con_notify_offset_x", "10", FCVAR_MATERIAL_SYSTEM_THREAD, "X offset for RUI console overlay.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_offset_y = ConVar::StaticCreate("con_notify_offset_y", "10", FCVAR_MATERIAL_SYSTEM_THREAD, "Y offset for RUI console overlay.", false, 1.f, false, 50.f, nullptr, nullptr);

	con_notify_script_server_clr  = ConVar::StaticCreate("con_notify_script_server_clr", "130 120 245 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Script SERVER VM RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_script_client_clr  = ConVar::StaticCreate("con_notify_script_client_clr", "117 116 139 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Script CLIENT VM RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_script_ui_clr      = ConVar::StaticCreate("con_notify_script_ui_clr"    , "200 110 110 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Script UI VM RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);

	con_notify_native_server_clr = ConVar::StaticCreate("con_notify_native_server_clr", "20 50 248 255"  , FCVAR_MATERIAL_SYSTEM_THREAD, "Native SERVER RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_native_client_clr = ConVar::StaticCreate("con_notify_native_client_clr", "70 70 70 255"   , FCVAR_MATERIAL_SYSTEM_THREAD, "Native CLIENT RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_native_ui_clr     = ConVar::StaticCreate("con_notify_native_ui_clr"    , "200 60 60 255"  , FCVAR_MATERIAL_SYSTEM_THREAD, "Native UI RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_native_engine_clr = ConVar::StaticCreate("con_notify_native_engine_clr", "255 255 255 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Native engine RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_native_fs_clr     = ConVar::StaticCreate("con_notify_native_fs_clr"    , "0 100 225 255"  , FCVAR_MATERIAL_SYSTEM_THREAD, "Native FileSystem RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_native_rtech_clr  = ConVar::StaticCreate("con_notify_native_rtech_clr" , "25 120 20 255"  , FCVAR_MATERIAL_SYSTEM_THREAD, "Native RTech RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_native_ms_clr     = ConVar::StaticCreate("con_notify_native_ms_clr"    , "200 20 180 255" , FCVAR_MATERIAL_SYSTEM_THREAD, "Native MaterialSystem RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_native_audio_clr  = ConVar::StaticCreate("con_notify_native_audio_clr" , "238 43 10 255"  , FCVAR_MATERIAL_SYSTEM_THREAD, "Native AudioSystem RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_native_video_clr  = ConVar::StaticCreate("con_notify_native_video_clr" , "115 0 235 255"  , FCVAR_MATERIAL_SYSTEM_THREAD, "Native VideoSystem RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);

	con_notify_netcon_clr  = ConVar::StaticCreate("con_notify_netcon_clr" , "255 255 255 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Net console RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_common_clr  = ConVar::StaticCreate("con_notify_common_clr" , "255 140 80 255" , FCVAR_MATERIAL_SYSTEM_THREAD, "Common RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);

	con_notify_warning_clr = ConVar::StaticCreate("con_notify_warning_clr", "180 180 20 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Warning RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_error_clr   = ConVar::StaticCreate("con_notify_error_clr"  , "225 20 20 255" , FCVAR_MATERIAL_SYSTEM_THREAD, "Error RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);

	con_max_lines                 = ConVar::StaticCreate("con_max_lines"                , "1024", FCVAR_DEVELOPMENTONLY, "Maximum number of lines in the console before cleanup starts.", true, 1.f, false, 0.f, nullptr, nullptr);
	con_max_history               = ConVar::StaticCreate("con_max_history"              , "512" , FCVAR_DEVELOPMENTONLY, "Maximum number of command submission items before history cleanup starts.", true, 0.f, false, 0.f, nullptr, nullptr);
	con_suggestion_limit          = ConVar::StaticCreate("con_suggestion_limit"         , "128" , FCVAR_DEVELOPMENTONLY, "Maximum number of suggestions the autocomplete window will show for the console.", true, 0.f, false, 0.f, nullptr, nullptr);
	con_suggestion_showhelptext   = ConVar::StaticCreate("con_suggestion_showhelptext"  , "1"   , FCVAR_DEVELOPMENTONLY, "Show CommandBase help text in autocomplete window.", false, 0.f, false, 0.f, nullptr, nullptr);
	con_suggestion_showflags      = ConVar::StaticCreate("con_suggestion_showflags"     , "1"   , FCVAR_DEVELOPMENTONLY, "Show CommandBase flags in autocomplete window.", false, 0.f, false, 0.f, nullptr, nullptr);
	con_suggestion_flags_realtime = ConVar::StaticCreate("con_suggestion_flags_realtime", "1"   , FCVAR_DEVELOPMENTONLY, "Whether to show compile-time or run-time CommandBase flags.", false, 0.f, false, 0.f, nullptr, nullptr);
#endif // !DEDICATED
	//-------------------------------------------------------------------------
	// FILESYSTEM                                                             |
	fs_showWarnings                   = ConVar::StaticCreate("fs_showWarnings"                       , "0", FCVAR_DEVELOPMENTONLY, "Logs the FileSystem warnings to the console, filtered by 'fs_warning_level' ( !slower! ).", true, 0.f, true, 2.f, nullptr, "0 = log to file. 1 = 0 + log to console. 2 = 1 + log to notify.");
	fs_packedstore_entryblock_stats   = ConVar::StaticCreate("fs_packedstore_entryblock_stats"       , "0", FCVAR_DEVELOPMENTONLY, "Logs the stats of each file entry in the VPK during decompression ( !slower! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	fs_packedstore_workspace          = ConVar::StaticCreate("fs_packedstore_workspace" , "platform/ship/", FCVAR_DEVELOPMENTONLY, "Determines the current VPK workspace.", false, 0.f, false, 0.f, nullptr, nullptr);
	fs_packedstore_compression_level  = ConVar::StaticCreate("fs_packedstore_compression_level", "default", FCVAR_DEVELOPMENTONLY, "Determines the VPK compression level.", false, 0.f, false, 0.f, nullptr, "fastest faster default better uber");
	fs_packedstore_max_helper_threads = ConVar::StaticCreate("fs_packedstore_max_helper_threads"    , "-1", FCVAR_DEVELOPMENTONLY, "Max # of additional \"helper\" threads to create during compression.", true, -1, true, LZHAM_MAX_HELPER_THREADS, nullptr, "Must range between [-1,LZHAM_MAX_HELPER_THREADS], where -1=max practical.");
	//-------------------------------------------------------------------------
	// MATERIALSYSTEM                                                         |
#ifndef DEDICATED
	mat_alwaysComplain = ConVar::StaticCreate("mat_alwaysComplain", "0", FCVAR_RELEASE | FCVAR_MATERIAL_SYSTEM_THREAD, "Always complain when a material is missing.", false, 0.f, false, 0.f, nullptr, nullptr);
#endif // !DEDICATED
	//-------------------------------------------------------------------------
	// SQUIRREL                                                               |
	script_show_output      = ConVar::StaticCreate("script_show_output" , "0", FCVAR_RELEASE, "Prints the VM output to the console ( !slower! ).", true, 0.f, true, 2.f, nullptr, "0 = log to file. 1 = 0 + log to console. 2 = 1 + log to notify.");
	script_show_warning     = ConVar::StaticCreate("script_show_warning", "0", FCVAR_RELEASE, "Prints the VM warning output to the console ( !slower! ).", true, 0.f, true, 2.f, nullptr, "0 = log to file. 1 = 0 + log to console. 2 = 1 + log to notify.");
	//-------------------------------------------------------------------------
	// NETCHANNEL                                                             |
	net_tracePayload           = ConVar::StaticCreate("net_tracePayload"          , "0", FCVAR_DEVELOPMENTONLY                    , "Log the payload of the send/recv datagram to a file on the disk.", false, 0.f, false, 0.f, nullptr, nullptr);
	net_encryptionEnable       = ConVar::StaticCreate("net_encryptionEnable"      , "1", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED , "Use AES encryption on game packets.", false, 0.f, false, 0.f, nullptr, nullptr);
	net_useRandomKey           = ConVar::StaticCreate("net_useRandomKey"          , "1"                        , FCVAR_RELEASE    , "Use random AES encryption key for game packets.", false, 0.f, false, 0.f, &NET_UseRandomKeyChanged_f, nullptr);
	net_processTimeBudget      = ConVar::StaticCreate("net_processTimeBudget"     ,"200"                       , FCVAR_RELEASE    , "Net message process time budget in milliseconds (removing netchannel if exceeded).", true, 0.f, false, 0.f, nullptr, "0 = disabled.");
	//-------------------------------------------------------------------------
	// NETWORKSYSTEM                                                          |
	pylon_matchmaking_hostname = ConVar::StaticCreate("pylon_matchmaking_hostname", "ms.r5reloaded.com", FCVAR_RELEASE, "Holds the pylon matchmaking hostname.", false, 0.f, false, 0.f, &MP_HostName_Changed_f, nullptr);
	pylon_host_update_interval = ConVar::StaticCreate("pylon_host_update_interval", "5"                , FCVAR_RELEASE, "Length of time in seconds between each status update interval to master server.", true, 5.f, false, 0.f, nullptr, nullptr);
	pylon_showdebuginfo        = ConVar::StaticCreate("pylon_showdebuginfo"       , "0"                , FCVAR_RELEASE, "Shows debug output for pylon.", false, 0.f, false, 0.f, nullptr, nullptr);
	//-------------------------------------------------------------------------
	// RTECH API                                                              |
	rtech_debug = ConVar::StaticCreate("rtech_debug", "0", FCVAR_DEVELOPMENTONLY, "Shows debug output for the RTech system.", false, 0.f, false, 0.f, nullptr, nullptr);
	//-------------------------------------------------------------------------
	// RUI                                                                    |
#ifndef DEDICATED
	rui_drawEnable = ConVar::StaticCreate("rui_drawEnable", "1", FCVAR_RELEASE, "Draws the RUI if set.", false, 0.f, false, 0.f, nullptr, " 1 = Draw, 0 = No Draw.");
#endif // !DEDICATED
	//-------------------------------------------------------------------------
	// MILES                                                                  |
#ifndef DEDICATED
	miles_debug = ConVar::StaticCreate("miles_debug", "0", FCVAR_RELEASE, "Enables debug prints for the Miles Sound System", false, 0.f, false, 0.f, nullptr, " 1 = Print, 0 = No Print");
#endif // !DEDICATED
	//-------------------------------------------------------------------------
}

//-----------------------------------------------------------------------------
// Purpose: initialize shipped ConVar's
//-----------------------------------------------------------------------------
void ConVar::InitShipped(void)
{
#ifndef CLIENT_DLL
	ai_script_nodes_draw             = g_pCVar->FindVar("ai_script_nodes_draw");
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1)
	bhit_enable                      = g_pCVar->FindVar("bhit_enable");
#endif // !GAMEDLL_S0 && !GAMEDLL_S1
#endif // !CLIENT_DLL
	developer                        = g_pCVar->FindVar("developer");
	fps_max                          = g_pCVar->FindVar("fps_max");
	fs_showAllReads                  = g_pCVar->FindVar("fs_showAllReads");
#ifndef DEDICATED
	cl_threaded_bone_setup           = g_pCVar->FindVar("cl_threaded_bone_setup");
#endif // !DEDICATED
	single_frame_shutdown_for_reload = g_pCVar->FindVar("single_frame_shutdown_for_reload");
	enable_debug_overlays            = g_pCVar->FindVar("enable_debug_overlays");
	debug_draw_box_depth_test        = g_pCVar->FindVar("debug_draw_box_depth_test");
	model_defaultFadeDistScale       = g_pCVar->FindVar("model_defaultFadeDistScale");
	model_defaultFadeDistMin         = g_pCVar->FindVar("model_defaultFadeDistMin");
#ifndef DEDICATED
	miles_language                   = g_pCVar->FindVar("miles_language");
	rui_defaultDebugFontFace         = g_pCVar->FindVar("rui_defaultDebugFontFace");
	r_visualizetraces                = g_pCVar->FindVar("r_visualizetraces");
	r_visualizetraces_duration       = g_pCVar->FindVar("r_visualizetraces_duration");
#endif // !DEDICATED
	staticProp_no_fade_scalar        = g_pCVar->FindVar("staticProp_no_fade_scalar");
	staticProp_gather_size_weight    = g_pCVar->FindVar("staticProp_gather_size_weight");
	stream_overlay                   = g_pCVar->FindVar("stream_overlay");
	stream_overlay_mode              = g_pCVar->FindVar("stream_overlay_mode");
	sv_visualizetraces               = g_pCVar->FindVar("sv_visualizetraces");
	sv_visualizetraces_duration      = g_pCVar->FindVar("sv_visualizetraces_duration");
	old_gather_props                 = g_pCVar->FindVar("old_gather_props");
#ifndef DEDICATED
	origin_disconnectWhenOffline     = g_pCVar->FindVar("origin_disconnectWhenOffline");
#endif // !DEDICATED
	mp_gamemode                      = g_pCVar->FindVar("mp_gamemode");
	hostname                         = g_pCVar->FindVar("hostname");
	hostip                           = g_pCVar->FindVar("hostip");
	hostport                         = g_pCVar->FindVar("hostport");
	host_hasIrreversibleShutdown     = g_pCVar->FindVar("host_hasIrreversibleShutdown");
	net_usesocketsforloopback        = g_pCVar->FindVar("net_usesocketsforloopback");
#ifndef CLIENT_DLL
	sv_stats = g_pCVar->FindVar("sv_stats");

	sv_updaterate_mp = g_pCVar->FindVar("sv_updaterate_mp");
	sv_updaterate_sp = g_pCVar->FindVar("sv_updaterate_sp");

	sv_showhitboxes = g_pCVar->FindVar("sv_showhitboxes");
	sv_forceChatToTeamOnly = g_pCVar->FindVar("sv_forceChatToTeamOnly");

	sv_showhitboxes->SetMin(-1); // Allow user to go over each entity manually without going out of bounds.
	sv_showhitboxes->SetMax(NUM_ENT_ENTRIES - 1);

	sv_forceChatToTeamOnly->RemoveFlags(FCVAR_DEVELOPMENTONLY);
	sv_forceChatToTeamOnly->AddFlags(FCVAR_REPLICATED);

	ai_script_nodes_draw->SetValue(-1);
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1) && !defined (GAMEDLL_S2)
	bhit_enable->SetValue(0);
#endif // !(GAMEDLL_S0) || !(GAMEDLL_S1) || !(GAMEDLL_S2)
#endif // !CLIENT_DLL
#ifndef DEDICATED
	cl_threaded_bone_setup->RemoveFlags(FCVAR_DEVELOPMENTONLY);
	rui_defaultDebugFontFace->RemoveFlags(FCVAR_DEVELOPMENTONLY);
	origin_disconnectWhenOffline->RemoveFlags(FCVAR_DEVELOPMENTONLY);
#endif // !DEDICATED
	mp_gamemode->RemoveFlags(FCVAR_DEVELOPMENTONLY);
	mp_gamemode->RemoveChangeCallback(mp_gamemode->m_fnChangeCallbacks[0]);
	mp_gamemode->InstallChangeCallback(MP_GameMode_Changed_f, false);
}

//-----------------------------------------------------------------------------
// Purpose: unregister/disable extraneous ConVar's.
//-----------------------------------------------------------------------------
void ConVar::PurgeShipped(void)
{
#ifdef DEDICATED
	const char* pszToPurge[] =
	{
		"bink_materials_enabled",
		"communities_enabled",
		"community_frame_run",
		"ime_enabled",
		"origin_igo_mutes_sound_enabled",
		"twitch_shouldQuery",
		"voice_enabled",
	};

	for (size_t i = 0; i < SDK_ARRAYSIZE(pszToPurge); i++)
	{
		if (ConVar* pCVar = g_pCVar->FindVar(pszToPurge[i]))
		{
			pCVar->SetValue(0);
		}
	}
#endif // DEDICATED
}

//-----------------------------------------------------------------------------
// Purpose: clear all hostname ConVar's.
//-----------------------------------------------------------------------------
void ConVar::PurgeHostNames(void)
{
	const char* pszHostNames[] =
	{
		"assetdownloads_hostname",
		"communities_hostname",
		"matchmaking_hostname",
		"party_hostname",
		"persistence_hostname",
		"persistenceDef_hostname",
		"pin_telemetry_hostname",
		"publication_hostname",
		"serverReports_hostname",
		"skill_hostname",
		"speechtotext_hostname",
		"staticfile_hostname",
		"stats_hostname",
		"steamlink_hostname",
		"subscription_hostname",
		"users_hostname"
	};

	for (size_t i = 0; i < SDK_ARRAYSIZE(pszHostNames); i++)
	{
		if (ConVar* pCVar = g_pCVar->FindVar(pszHostNames[i]))
		{
			pCVar->SetValue("0.0.0.0");
		}
	}
}

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
// Purpose: Return ConVar value as a double.
// Output : double
//-----------------------------------------------------------------------------
double ConVar::GetDouble(void) const
{
	return static_cast<double>(m_pParent->m_Value.m_fValue);
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
// Purpose: Return ConVar value as an integer (64-bit).
// Output : int
//-----------------------------------------------------------------------------
int64_t ConVar::GetInt64(void) const
{
	return static_cast<int64_t>(m_pParent->m_Value.m_nValue);
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a size type.
// Output : int
//-----------------------------------------------------------------------------
size_t ConVar::GetSizeT(void) const
{
	return static_cast<size_t>(m_pParent->m_Value.m_nValue);
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a color.
// Output : Color
//-----------------------------------------------------------------------------
Color ConVar::GetColor(void) const
{
	unsigned char* pColorElement = (reinterpret_cast<unsigned char*>(&m_pParent->m_Value.m_nValue));
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
			unsigned char* pColorElement = (reinterpret_cast<unsigned char*>(&m_Value.m_nValue));
			pColorElement[0] = nRGBA[0];
			pColorElement[1] = nRGBA[1];
			pColorElement[2] = nRGBA[2];
			pColorElement[3] = nRGBA[3];

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
			MemAllocSingleton()->Free(m_Value.m_pszString);
		}

		m_Value.m_pszString = MemAllocSingleton()->Alloc<char>(len);
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
		Warning(eDLL_T::ENGINE, "%s: Called with NULL callback; ignoring!!!\n", __FUNCTION__);
		return;
	}

	if (m_pParent->m_fnChangeCallbacks.Find(callback) != m_pParent->m_fnChangeCallbacks.InvalidIndex())
	{
		// Same ptr added twice, sigh...
		Warning(eDLL_T::ENGINE, "%s: Ignoring duplicate change callback!!!\n", __FUNCTION__);
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

//-----------------------------------------------------------------------------
// Purpose: Returns current player calling this function
// Output : ECommandTarget_t - 
//-----------------------------------------------------------------------------
ECommandTarget_t Cbuf_GetCurrentPlayer(void)
{
	// Always returns 'CBUF_FIRST_PLAYER' in Respawn's code.
	return ECommandTarget_t::CBUF_FIRST_PLAYER;
}

//-----------------------------------------------------------------------------
// Purpose: Sends the entire command line over to the server
// Input  : *args - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool Cmd_ForwardToServer(const CCommand* args)
{
#ifndef DEDICATED
	// Client -> Server command throttling.
	static double flForwardedCommandQuotaStartTime = -1;
	static int nForwardedCommandQuotaCount = 0;

	// No command to forward.
	if (args->ArgC() == 0)
		return false;

	double flStartTime = Plat_FloatTime();
	int nCmdQuotaLimit = cl_quota_stringCmdsPerSecond->GetInt();
	const char* pszCmdString = nullptr;

	// Special case: "cmd whatever args..." is forwarded as "whatever args...";
	// in this case we strip "cmd" from the input.
	if (Q_strcasecmp(args->Arg(0), "cmd") == 0)
		pszCmdString = args->ArgS();
	else
		pszCmdString = args->GetCommandString();

	if (nCmdQuotaLimit)
	{
		if (flStartTime - flForwardedCommandQuotaStartTime >= 1.0)
		{
			flForwardedCommandQuotaStartTime = flStartTime;
			nForwardedCommandQuotaCount = 0;
		}
		++nForwardedCommandQuotaCount;

		if (nForwardedCommandQuotaCount > nCmdQuotaLimit)
		{
			// If we are over quota commands per second, dump this on the floor.
			// If we spam the server with too many commands, it will kick us.
			Warning(eDLL_T::CLIENT, "Command '%s' ignored (submission quota of '%d' per second exceeded!)\n", pszCmdString, nCmdQuotaLimit);
			return false;
		}
	}
	return v_Cmd_ForwardToServer(args);
#else // !DEDICATED
	return false; // Client only.
#endif // DEDICATED
}

///////////////////////////////////////////////////////////////////////////////
void VConCommand::Attach() const
{
	DetourAttach((LPVOID*)&v_Cmd_ForwardToServer, &Cmd_ForwardToServer);
	DetourAttach((LPVOID*)&v_ConVar_PrintDescription, &ConVar_PrintDescription);
}
void VConCommand::Detach() const
{
	DetourDetach((LPVOID*)&v_Cmd_ForwardToServer, &Cmd_ForwardToServer);
	DetourDetach((LPVOID*)&v_ConVar_PrintDescription, &ConVar_PrintDescription);
}
