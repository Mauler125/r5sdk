//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $NoKeywords: $
//===========================================================================//

#ifndef TIER1_CMD_H
#define TIER1_CMD_H

#include "tier1/utlvector.h"
#include "tier1/utlstring.h"
#include "tier1/characterset.h"
#include "public/iconvar.h"
#include "public/iconcommand.h"
#include "mathlib/color.h"

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
	virtual ~ConCommandBase(void) { };

	virtual bool IsCommand(void) const = 0;
	virtual bool IsFlagSet(int nFlags) const = 0;

	virtual void AddFlags(int nFlags) = 0;
	virtual void RemoveFlags(int nFlags) = 0;

	virtual int GetFlags(void) const = 0;
	virtual const char* GetName(void) const = 0;
	virtual const char* GetHelpText(void) const = 0;
	virtual const char* GetUsageText(void) const = 0;

	virtual void SetAccessor(IConCommandBaseAccessor* pAccessor) = 0;
	virtual bool IsRegistered(void) const = 0;

	virtual int GetDLLIdentifier() const = 0;
	virtual ConCommandBase* Create (const char* szName, const char* szHelpString,
		int nFlags, const char* pszUsageString) = 0;

	virtual void				Init() = 0;

	bool HasFlags(int nFlags) const;

	ConCommandBase* GetNext(void) const;
	char* CopyString(const char* szFrom) const;

	ConCommandBase*          m_pNext;          //0x0008
	bool                     m_bRegistered;    //0x0010
	const char*              m_pszName;        //0x0018
	const char*              m_pszHelpString;  //0x0020
	const char*              m_pszUsageString; //0x0028
	IConCommandBaseAccessor* s_pAccessor;      //0x0030 <-- unused since executable is monolithic.
	int                      m_nFlags;         //0x0038
}; //Size: 0x0040
static_assert(sizeof(ConCommandBase) == 0x40);

//-----------------------------------------------------------------------------
// Purpose: The console invoked command
//-----------------------------------------------------------------------------
class ConCommand : public ConCommandBase
{
	friend class CCvar;
public:
	ConCommand(void);

	static ConCommand* StaticCreate(const char* szName, const char* szHelpString, const char* pszUsageString,
		int nFlags, FnCommandCallback_t pCallback, FnCommandCompletionCallback pCommandCompletionCallback);

	virtual int AutoCompleteSuggest(const char* partial, CUtlVector< CUtlString >& commands) = 0;
	virtual bool CanAutoComplete(void) const = 0;

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
		FnCommandCompletionCallback m_fnCompletionCallback;
		ICommandCompletionCallback* m_pCommandCompletionCallback;
	};

	bool m_bHasCompletionCallback : 1;
	bool m_bUsingNewCommandCallback : 1;
	bool m_bUsingCommandCallbackInterface : 1;
};

/* ==== CONCOMMAND ====================================================================================================================================================== */
inline CMemory p_ConCommand_AutoCompleteSuggest;
inline bool(*ConCommand_AutoCompleteSuggest)(ConCommand* pCommand, const char* partial, CUtlVector< CUtlString >& commands);

inline CMemory p_ConCommandBase_IsFlagSet;
inline bool(*ConCommandBase_IsFlagSet)(ConCommandBase* pCommand, int nFlag);

inline CMemory p_NullSub;
inline void(*NullSub)(void);

inline CMemory p_CallbackStub;
inline FnCommandCompletionCallback CallbackStub = p_CallbackStub.RCast<FnCommandCompletionCallback>();

inline ConCommandBase* g_pConCommandVFTable;

///////////////////////////////////////////////////////////////////////////////
ECommandTarget_t Cbuf_GetCurrentPlayer(void);

///////////////////////////////////////////////////////////////////////////////
class VConCommand : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("ConCommandBase::IsFlagSet", p_ConCommandBase_IsFlagSet.GetPtr());
		LogFunAdr("ConCommand::AutoCompleteSuggest", p_ConCommand_AutoCompleteSuggest.GetPtr());
		LogFunAdr("CallbackStub", p_CallbackStub.GetPtr());
		LogFunAdr("NullSub", p_NullSub.GetPtr());
	}
	virtual void GetFun(void) const
	{
		p_ConCommand_AutoCompleteSuggest    = g_GameDll.FindPatternSIMD("40 ?? B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 2B E0 F6 41 60 04");
		p_ConCommandBase_IsFlagSet          = g_GameDll.FindPatternSIMD("85 51 38 0F 95 C0 C3");

		p_NullSub                           = g_GameDll.FindPatternSIMD("C2 ?? ?? CC CC CC CC CC CC CC CC CC CC CC CC CC 40 53 48 83 EC 20 48 8D 05 ?? ?? ?? ??");
		p_CallbackStub                      = g_GameDll.FindPatternSIMD("33 C0 C3 CC CC CC CC CC CC CC CC CC CC CC CC CC 80 49 68 08");

		ConCommandBase_IsFlagSet            = p_ConCommandBase_IsFlagSet.RCast<bool (*)(ConCommandBase*, int)>(); /*85 51 38 0F 95 C0 C3*/
		ConCommand_AutoCompleteSuggest      = p_ConCommand_AutoCompleteSuggest.RCast<bool (*)(ConCommand*, const char*, CUtlVector< CUtlString >&)>();

		NullSub                             = p_NullSub.RCast<void(*)(void)>();                                   /*C2 00 00 CC CC CC CC CC CC CC CC CC CC CC CC CC 40 53 48 83 EC 20 48 8D 05 ?? ?? ?? ??*/
		CallbackStub                        = p_CallbackStub.RCast<FnCommandCompletionCallback>();                /*33 C0 C3 CC CC CC CC CC CC CC CC CC CC CC CC CC 80 49 68 08*/ /*UserMathErrorFunction*/
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const 
	{
		g_pConCommandVFTable = g_GameDll.GetVirtualMethodTable(".?AVConCommand@@").RCast<ConCommandBase*>();
	}
	virtual void Attach(void) const { };
	virtual void Detach(void) const { };
};
///////////////////////////////////////////////////////////////////////////////

#endif // TIER1_CMD_H
