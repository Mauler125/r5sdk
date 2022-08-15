#pragma once
#include "tier1/characterset.h"
#include "public/iconvar.h"
#include "public/iconcommand.h"

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
	CCommand();
	CCommand(int nArgC, const char** ppArgV, cmd_source_t source);
	bool Tokenize(const char* pCommand, cmd_source_t source, characterset_t* pBreakSet);

	int64_t ArgC(void) const;
	const char** ArgV(void) const;
	const char* ArgS(void) const;
	const char* GetCommandString(void) const;
	const char* Arg(int nIndex) const;
	const char* operator[](int nIndex) const;

	void Reset();
	int MaxCommandLength(void) const;
	bool HasOnlyDigits(int nIndex) const;

private:
	cmd_source_t m_nQueuedVal;
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
	bool HasFlags(int nFlags) const;
	void AddFlags(int nFlags);
	void RemoveFlags(int nFlags);

	bool IsCommand(void) const;
	bool IsRegistered(void) const;

	bool IsFlagSet(int nFlags) const { return IsFlagSetInternal(this, nFlags); };
	static bool IsFlagSetInternal(const ConCommandBase* pCommandBase, int nFlags);

	int GetFlags(void) const;
	ConCommandBase* GetNext(void) const;
	const char* GetName(void) const;
	const char* GetHelpText(void) const;
	const char* GetUsageText(void) const;

	char* CopyString(const char* szFrom) const;

	IConCommandBase* m_pConCommandBaseVFTable; //0x0000
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
	friend class CCvar;
public:
	static ConCommand* Create(const char* szName, const char* szHelpString, int nFlags, FnCommandCallback_t pCallback, FnCommandCompletionCallback pCommandCompletionCallback);

	ConCommand(void);
	~ConCommand(void);

	void Init(void);
	void InitShipped(void);
	void PurgeShipped(void) const;
	bool IsCommand(void) const;

	void*          m_nNullCallBack; //0x0040
	void*          m_pSubCallback;  //0x0048
	// Call this function when executing the command
	union
	{
		FnCommandCallbackV1_t m_fnCommandCallbackV1;
		FnCommandCallback_t m_fnCommandCallback;
		ICommandCallback* m_pCommandCallback;
	};

	union
	{
		FnCommandCompletionCallback	m_fnCompletionCallback;
		ICommandCompletionCallback* m_pCommandCompletionCallback;
	};

	bool m_bHasCompletionCallback : 1;
	bool m_bUsingNewCommandCallback : 1;
	bool m_bUsingCommandCallbackInterface : 1;
};

/* ==== COMMAND_BUFFER ================================================================================================================================================== */
inline CMemory p_Cbuf_AddText;
inline auto Cbuf_AddText = p_Cbuf_AddText.RCast<void (*)(ECommandTarget_t eTarget, const char* pText, cmd_source_t cmdSource)>();

inline CMemory p_Cbuf_Execute;
inline auto Cbuf_Execute = p_Cbuf_Execute.RCast<void (*)(void)>();

/* ==== CONCOMMAND ====================================================================================================================================================== */
inline CMemory p_ConCommandBase_IsFlagSet;
inline auto ConCommandBase_IsFlagSet = p_ConCommandBase_IsFlagSet.RCast<bool (*)(ConCommandBase* pCommand, int nFlag)>();

inline CMemory p_NullSub;
inline auto NullSub = p_NullSub.RCast<void(*)(void)>();

inline CMemory p_CallbackStub;
inline FnCommandCompletionCallback CallbackStub = p_CallbackStub.RCast<FnCommandCompletionCallback>();

inline CMemory g_pConCommandVFTable;

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
		spdlog::debug("+----------------------------------------------------------------+\n");
		spdlog::debug("| FUN: CallbackStub                         : {:#18x} |\n", p_CallbackStub.GetPtr());
		spdlog::debug("| FUN: NullSub                              : {:#18x} |\n", p_NullSub.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
		spdlog::debug("| VAR: g_pConCommandVFTable                 : {:#18x} |\n", g_pConCommandVFTable.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_Cbuf_AddText                      = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\x48\x63\xD9\x41\x8B\xF8\x48\x8D\x0D\x00\x00\x00\x00\x48\x8B\xF2\xFF\x15\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x41\xB9\x00\x00\x00\x00"), "xxxx?xxxx?xxxxxxxxxxxxxx????xxxxx????xxx????xx????");
		p_Cbuf_Execute                      = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\xFF\x15\x00\x00\x00\x00"), "xxxx?xxxx?xxxx?xxxxxxx????");
		p_ConCommandBase_IsFlagSet          = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x85\x51\x38\x0F\x95\xC0\xC3"), "xxxxxxx");
		p_NullSub                           = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\xC2\x00\x00\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x40\x53\x48\x83\xEC\x20\x48\x8D\x05\x00\x00\x00\x00"), "xxxxxxxxxxxxxxxxxxxxxxxxx????");
		p_CallbackStub                      = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x33\xC0\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x80\x49\x68\x08"), "xxxxxxxxxxxxxxxxxxxx");

		Cbuf_AddText = p_Cbuf_AddText.RCast<void (*)(ECommandTarget_t, const char*, cmd_source_t)>();           /*48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 63 D9 41 8B F8 48 8D 0D ?? ?? ?? ?? 48 8B F2 FF 15 ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 41 B9 ?? ?? ?? ??*/
		Cbuf_Execute = p_Cbuf_Execute.RCast<void (*)(void)>();                                                  /*48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 FF 15 ?? ?? ?? ??*/
		ConCommandBase_IsFlagSet          = p_ConCommandBase_IsFlagSet.RCast<bool (*)(ConCommandBase*, int)>(); /*85 51 38 0F 95 C0 C3*/
		NullSub                           = p_NullSub.RCast<void(*)(void)>();                                   /*C2 00 00 CC CC CC CC CC CC CC CC CC CC CC CC CC 40 53 48 83 EC 20 48 8D 05 ?? ?? ?? ??*/
		CallbackStub                      = p_CallbackStub.RCast<FnCommandCompletionCallback>();                /*33 C0 C3 CC CC CC CC CC CC CC CC CC CC CC CC CC 80 49 68 08*/ /*UserMathErrorFunction*/
	}
	virtual void GetVar(void) const
	{
		g_pConCommandVFTable = g_GameDll.GetVirtualMethodTable(".?AVConCommand@@");
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VConCommand);
