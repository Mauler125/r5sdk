#pragma once

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class ConCommandBase;

//-----------------------------------------------------------------------------
// Any executable that wants to use ConVars need to implement one of
// these to hook up access to console variables.
//-----------------------------------------------------------------------------
class IConCommandBaseAccessor
{
public:
	// Flags is a combination of FCVAR flags in cvar.h.
	// hOut is filled in with a handle to the variable.
	virtual bool RegisterConCommandBase(ConCommandBase* pVar) = 0;
};

//-----------------------------------------------------------------------------
// Purpose: Command buffer context
//-----------------------------------------------------------------------------
enum class ECommandTarget_t : int
{
	CBUF_FIRST_PLAYER = 0,
	CBUF_LAST_PLAYER = MAX_SPLITSCREEN_CLIENTS - 1,
	CBUF_SERVER = CBUF_LAST_PLAYER + 1,

	CBUF_COUNT,
};

//-----------------------------------------------------------------------------
// Sources of console commands
//-----------------------------------------------------------------------------
enum class cmd_source_t : int
{
	kCommandSrcCode,
	kCommandSrcClientCmd,
	kCommandSrcUserInput,
	kCommandSrcNetClient,
	kCommandSrcNetServer,
	kCommandSrcDemoFile,
	kCommandSrcInvalid = -1
};

//-----------------------------------------------------------------------------
// Purpose: Command tokenizer
//-----------------------------------------------------------------------------
class CCommand
{
private:
	enum
	{
		COMMAND_MAX_ARGC   = 64,
		COMMAND_MAX_LENGTH = 512,
	};

public:
	CCommand() = delete;

	int MaxCommandLength();
	int64_t ArgC(void) const;
	const char** ArgV(void) const;
	const char* ArgS(void) const;
	const char* GetCommandString(void) const;
	const char* Arg(int nIndex) const;
	const char* operator[](int nIndex) const;

	bool HasOnlyDigits(int nIndex) const;

private:
	int          m_nQueuedVal;
	int          m_nArgc;
	int64_t      m_nArgv0Size;
	char         m_pArgSBuffer[COMMAND_MAX_LENGTH];
	char         m_pArgvBuffer[COMMAND_MAX_LENGTH];
	const char*  m_ppArgv[COMMAND_MAX_ARGC];
};

//-----------------------------------------------------------------------------
// Purpose: The base console invoked command/cvar interface
//-----------------------------------------------------------------------------
class ConCommandBase
{
public:
	bool HasFlags(int nFlags);
	void AddFlags(int nFlags);
	void RemoveFlags(int nFlags);

	bool IsCommand(void) const;
	bool IsRegistered(void) const;
	static bool IsFlagSet(ConCommandBase* pCommandBase, int nFlags);

	int GetFlags(void) const;
	ConCommandBase* GetNext(void) const;
	const char* GetName(void) const;
	const char* GetHelpText(void) const;
	const char* GetUsageText(void) const;

	char* CopyString(const char* szFrom) const;

	void*    m_pConCommandBaseVTable;          //0x0000
	ConCommandBase*          m_pNext;          //0x0008
	bool                     m_bRegistered;    //0x0010
	char                     pad_0011[7];      //0x0011
	const char*              m_pszName;        //0x0018
	const char*              m_pszHelpString;  //0x0020
	const char*              m_pszUsageString; //0x0028
	IConCommandBaseAccessor* s_pAccessor;      //0x0030 <-- unused since executable is monolithic.
	int                      m_nFlags;         //0x0038
	char                     pad_003C[4];      //0x003C
}; //Size: 0x0040

//-----------------------------------------------------------------------------
// Purpose: The console invoked command
//-----------------------------------------------------------------------------
class ConCommand : public ConCommandBase
{
	friend class CCVar;
public:
	ConCommand(void) {};
	ConCommand(const char* szName, const char* szHelpString, int nFlags, void* pCallback, void* pCommandCompletionCallback);
	void Init(void);
	void InitShipped(void);
	void PurgeShipped(void) const;
	bool IsCommand(void) const;

	void*          m_nNullCallBack      {}; //0x0040
	char           m_nPad48[8]          {}; //0x0048
	void*          m_pCommandCallback   {}; //0x0050
	void*          m_pCompletionCallback{}; //0x0058
	int            m_nCallbackFlags     {}; //0x0060
	char           m_nPad68[4]          {}; //0x0068
};

/* ==== COMMAND_BUFFER ================================================================================================================================================== */
inline CMemory p_Cbuf_AddText;
inline auto Cbuf_AddText = p_Cbuf_AddText.RCast<void (*)(ECommandTarget_t eTarget, const char* pText, cmd_source_t cmdSource)>();

inline CMemory p_Cbuf_Execute;
inline auto Cbuf_Execute = p_Cbuf_Execute.RCast<void (*)(void)>();

/* ==== CONCOMMAND ====================================================================================================================================================== */
inline CMemory p_ConCommandBase_IsFlagSet;
inline auto ConCommandBase_IsFlagSet = p_ConCommandBase_IsFlagSet.RCast<bool (*)(ConCommandBase* pCommand, int nFlag)>();

inline CMemory p_ConCommand_CMaterialSystemCmdInit;
inline auto ConCommand_CMaterialSystemCmdInit = p_ConCommand_CMaterialSystemCmdInit.RCast<ConCommand* (*)(void)>();

inline CMemory p_ConCommand_RegisterConCommand;
inline auto ConCommand_RegisterConCommand = p_ConCommand_RegisterConCommand.RCast<void* (*)(ConCommand* pCommand)>();

inline CMemory p_NullSub;
inline auto NullSub = p_NullSub.RCast<void(*)(void)>();

inline CMemory p_CallbackStub;
inline auto CallbackStub = p_CallbackStub.RCast<void* (*)(struct _exception* _exc)>();

inline CMemory g_pConCommandVtable;

///////////////////////////////////////////////////////////////////////////////
ECommandTarget_t Cbuf_GetCurrentPlayer(void);

void ConCommand_Attach();
void ConCommand_Detach();

extern ConCommand* g_pConCommand;

///////////////////////////////////////////////////////////////////////////////
class VConCommand : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: Cbuf_AddText                         : {:#18x} |\n", p_Cbuf_AddText.GetPtr());
		spdlog::debug("| FUN: Cbuf_Execute                         : {:#18x} |\n", p_Cbuf_Execute.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
		spdlog::debug("| FUN: ConCommandBase::IsFlagSet            : {:#18x} |\n", p_ConCommandBase_IsFlagSet.GetPtr());
		spdlog::debug("| FUN: ConCommand::CMaterialSystemCmdInit   : {:#18x} |\n", p_ConCommand_CMaterialSystemCmdInit.GetPtr());
		spdlog::debug("| FUN: ConCommand::RegisterConCommand       : {:#18x} |\n", p_ConCommand_RegisterConCommand.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
		spdlog::debug("| FUN: CallbackStub                         : {:#18x} |\n", p_CallbackStub.GetPtr());
		spdlog::debug("| FUN: NullSub                              : {:#18x} |\n", p_NullSub.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
		spdlog::debug("| VAR: g_pConCommandVtable                  : {:#18x} |\n", g_pConCommandVtable.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_Cbuf_AddText                      = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\x48\x63\xD9\x41\x8B\xF8\x48\x8D\x0D\x00\x00\x00\x00\x48\x8B\xF2\xFF\x15\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x41\xB9\x00\x00\x00\x00"), "xxxx?xxxx?xxxxxxxxxxxxxx????xxxxx????xxx????xx????");
		p_Cbuf_Execute                      = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\xFF\x15\x00\x00\x00\x00"), "xxxx?xxxx?xxxx?xxxxxxx????");
		p_ConCommandBase_IsFlagSet          = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x85\x51\x38\x0F\x95\xC0\xC3"), "xxxxxxx");
		p_ConCommand_CMaterialSystemCmdInit = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8B\xEC\x48\x83\xEC\x50\x48\x8B\x15\x00\x00\x00\x00"), "xxxx?xxxx?xxxx?xxxxxxxxxxxxxxxxxxx????");
		p_ConCommand_RegisterConCommand     = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xD1\x48\x8B\x0D\x00\x00\x00\x00\x48\x85\xC9\x74\x06"), "xxxxxx????xxxxx");
		p_NullSub                           = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\xC2\x00\x00\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x40\x53\x48\x83\xEC\x20\x48\x8D\x05\x00\x00\x00\x00"), "xxxxxxxxxxxxxxxxxxxxxxxxx????");
		p_CallbackStub                      = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x33\xC0\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x80\x49\x68\x08"), "xxxxxxxxxxxxxxxxxxxx");

		Cbuf_AddText = p_Cbuf_AddText.RCast<void (*)(ECommandTarget_t eTarget, const char* pText, cmd_source_t cmdSource)>();  /*48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 63 D9 41 8B F8 48 8D 0D ?? ?? ?? ?? 48 8B F2 FF 15 ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 41 B9 ?? ?? ?? ??*/
		Cbuf_Execute = p_Cbuf_Execute.RCast<void (*)(void)>();                                                                 /*48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 FF 15 ?? ?? ?? ??*/
		ConCommandBase_IsFlagSet          = p_ConCommandBase_IsFlagSet.RCast<bool (*)(ConCommandBase* pCommand, int nFlag)>(); /*85 51 38 0F 95 C0 C3*/
		ConCommand_CMaterialSystemCmdInit = p_ConCommand_CMaterialSystemCmdInit.RCast<ConCommand* (*)(void)>();                /*48 89 5C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 55 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC 50 48 8B 15 ?? ?? ?? ??*/
		ConCommand_RegisterConCommand     = p_ConCommand_RegisterConCommand.RCast<void* (*)(ConCommand* pCommand)>();          /*48 8B D1 48 8B 0D ?? ?? ?? ?? 48 85 C9 74 06*/
		NullSub                           = p_NullSub.RCast<void(*)(void)>();                                                  /*C2 00 00 CC CC CC CC CC CC CC CC CC CC CC CC CC 40 53 48 83 EC 20 48 8D 05 ?? ?? ?? ??*/
		CallbackStub                      = p_CallbackStub.RCast<void* (*)(struct _exception* _exc)>();                        /*33 C0 C3 CC CC CC CC CC CC CC CC CC CC CC CC CC 80 49 68 08*/ /*UserMathErrorFunction*/
	}
	virtual void GetVar(void) const
	{
		g_pConCommandVtable = p_ConCommand_CMaterialSystemCmdInit.FindPatternSelf("4C 8D 25", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7);
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VConCommand);
