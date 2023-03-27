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

#ifndef CONVAR_H
#define CONVAR_H

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
	// Official name 'GetName', couldn't be used due to name ambiguity
	// with the 'GetName' function in the IConVar class.
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

	static void StaticInit(void);
	static void InitShipped(void);
	static void PurgeShipped(void);

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

//-----------------------------------------------------------------------------
// Purpose: A console variable
//-----------------------------------------------------------------------------
class ConVar : public ConCommandBase, public IConVar
{
	friend class CCvar;
	friend class ConVarRef;

public:
	static ConVar* StaticCreate(const char* pszName, const char* pszDefaultValue, int nFlags, const char* pszHelpString,
		bool bMin, float fMin, bool bMax, float fMax, FnChangeCallback_t pCallback, const char* pszUsageString);
	void Destroy(void);

	ConVar(void);
	virtual ~ConVar(void) { };

	static void Init(void);
	static void InitShipped(void);

	static void PurgeShipped(void);
	static void PurgeHostNames(void);

	bool GetBool(void) const;
	float GetFloat(void) const;
	double GetDouble(void) const;
	int GetInt(void) const;
	int64_t GetInt64(void) const;
	size_t GetSizeT(void) const;
	Color GetColor(void) const;
	const char* GetString(void) const;

	void SetMax(float flMaxValue);
	void SetMin(float flMinValue);
	bool GetMin(float& flMinValue) const;
	bool GetMax(float& flMaxValue) const;
	float GetMinValue(void) const;
	float GetMaxValue(void) const;
	bool HasMin(void) const;
	bool HasMax(void) const;

	void SetValue(int nValue);
	void SetValue(float flValue);
	void SetValue(const char* pszValue);
	void SetValue(Color clValue);

	virtual void InternalSetValue(const char* pszValue) = 0;
	virtual void InternalSetFloatValue(float flValue) = 0;
	virtual void InternalSetIntValue(int nValue) = 0;
	void InternalSetColorValue(Color value);

	virtual __int64 Unknown0(unsigned int a2) = 0;
	virtual __int64 Unknown1(const char* a2) = 0;

	void Revert(void);
	virtual bool ClampValue(float& flValue) = 0;

	const char* GetDefault(void) const;
	void SetDefault(const char* pszDefault);
	bool SetColorFromString(const char* pszValue);

	virtual void ChangeStringValue(const char* pszTempValue) = 0;
	virtual void Create(const char* pszName, const char* pszDefaultValue, int nFlags, const char* pszHelpString,
		bool bMin, float fMin, bool bMax, float fMax, FnChangeCallback_t pCallback, const char* pszUsageString) = 0;

	void InstallChangeCallback(FnChangeCallback_t callback, bool bInvoke);
	void RemoveChangeCallback(FnChangeCallback_t callback);

	virtual bool IsFlagSet(int nFlags) { return (nFlags & m_pParent->m_nFlags) ? true : false; };
	virtual const char* GetName(void) const { return m_pParent->m_pszName; };

	struct CVValue_t
	{
		char*      m_pszString;
		size_t     m_iStringLength;
		float      m_fValue;
		int        m_nValue;
	};

	ConVar*        m_pParent;         //0x0048
	const char*    m_pszDefaultValue; //0x0050
	CVValue_t      m_Value;           //0c0058
	bool           m_bHasMin;         //0x0070
	float          m_fMinVal;         //0x0074
	bool           m_bHasMax;         //0x0078
	float          m_fMaxVal;         //0x007C
	CUtlVector<FnChangeCallback_t> m_fnChangeCallbacks; //0x0080
}; //Size: 0x00A0
static_assert(sizeof(ConVar) == 0xA0);

///////////////////////////////////////////////////////////////////////////////
void ConVar_PrintDescription(ConCommandBase* pVar);


/* ==== COMMAND_BUFFER ================================================================================================================================================== */
inline CMemory p_Cbuf_AddText;
inline auto Cbuf_AddText = p_Cbuf_AddText.RCast<void (*)(ECommandTarget_t eTarget, const char* pText, cmd_source_t cmdSource)>();

inline CMemory p_Cbuf_Execute;
inline auto Cbuf_Execute = p_Cbuf_Execute.RCast<void (*)(void)>();

inline CMemory p_Cmd_ForwardToServer;
inline auto v_Cmd_ForwardToServer = p_Cmd_ForwardToServer.RCast<bool (*)(const CCommand* args)>();

/* ==== CONCOMMAND ====================================================================================================================================================== */
inline CMemory p_ConCommand_AutoCompleteSuggest;
inline auto ConCommand_AutoCompleteSuggest = p_ConCommand_AutoCompleteSuggest.RCast<bool (*)(ConCommand* pCommand, const char* partial, CUtlVector< CUtlString >& commands)>();

inline CMemory p_ConCommandBase_IsFlagSet;
inline auto ConCommandBase_IsFlagSet = p_ConCommandBase_IsFlagSet.RCast<bool (*)(ConCommandBase* pCommand, int nFlag)>();

inline CMemory p_NullSub;
inline auto NullSub = p_NullSub.RCast<void(*)(void)>();

inline CMemory p_CallbackStub;
inline FnCommandCompletionCallback CallbackStub = p_CallbackStub.RCast<FnCommandCompletionCallback>();

inline ConCommandBase* g_pConCommandVFTable;

/* ==== ICONVAR ========================================================================================================================================================= */
inline CMemory p_ConVar_Register;
inline auto v_ConVar_Register = p_ConVar_Register.RCast<void* (*)(ConVar* thisptr, const char* szName, const char* szDefaultValue, int nFlags, const char* szHelpString, bool bMin, float fMin, bool bMax, float fMax, FnChangeCallback_t pCallback, const char* pszUsageString)>();

inline CMemory p_ConVar_Unregister;
inline auto v_ConVar_Unregister = p_ConVar_Unregister.RCast<void (*)(ConVar* thisptr)>();

inline CMemory p_ConVar_IsFlagSet;
inline auto v_ConVar_IsFlagSet = p_ConVar_IsFlagSet.RCast<bool (*)(ConVar* pConVar, int nFlag)>();

inline CMemory p_ConVar_PrintDescription;
inline auto v_ConVar_PrintDescription = p_ConVar_PrintDescription.RCast<void* (*)(ConCommandBase* pVar)>();

inline ConVar* g_pConVarVBTable;
inline IConVar* g_pConVarVFTable;

///////////////////////////////////////////////////////////////////////////////
ECommandTarget_t Cbuf_GetCurrentPlayer(void);

///////////////////////////////////////////////////////////////////////////////
class VConCommand : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogConAdr("ConCommand::`vftable'", reinterpret_cast<uintptr_t>(g_pConCommandVFTable));
		LogConAdr("ConVar::`vbtable'", reinterpret_cast<uintptr_t>(g_pConVarVBTable));
		LogConAdr("ConVar::`vftable'", reinterpret_cast<uintptr_t>(g_pConVarVFTable));
		LogFunAdr("ConCommandBase::IsFlagSet", p_ConCommandBase_IsFlagSet.GetPtr());
		LogConAdr("ConCommand::AutoCompleteSuggest", p_ConCommand_AutoCompleteSuggest.GetPtr());
		LogFunAdr("ConVar::Register", p_ConVar_Register.GetPtr());
		LogFunAdr("ConVar::Unregister", p_ConVar_Unregister.GetPtr());
		LogFunAdr("ConVar::IsFlagSet", p_ConVar_IsFlagSet.GetPtr());
		LogFunAdr("ConVar_PrintDescription", p_ConVar_PrintDescription.GetPtr());
		LogFunAdr("Cbuf_AddText", p_Cbuf_AddText.GetPtr());
		LogFunAdr("Cbuf_Execute", p_Cbuf_Execute.GetPtr());
		LogFunAdr("Cmd_ForwardToServer", p_Cmd_ForwardToServer.GetPtr());
		LogFunAdr("CallbackStub", p_CallbackStub.GetPtr());
		LogFunAdr("NullSub", p_NullSub.GetPtr());
	}
	virtual void GetFun(void) const
	{
		p_ConCommand_AutoCompleteSuggest    = g_GameDll.FindPatternSIMD("40 ?? B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 2B E0 F6 41 60 04");
		p_ConCommandBase_IsFlagSet          = g_GameDll.FindPatternSIMD("85 51 38 0F 95 C0 C3");

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_ConVar_Register                   = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 56 48 83 EC 30 F3 0F 10 44 24 ??");
		p_ConVar_Unregister                 = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 57 48 83 EC 20 48 8B 59 58 48 8D 05 ?? ?? ?? ??");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_ConVar_Register                   = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC 40 F3 0F 10 84 24 ?? ?? ?? ??");
		p_ConVar_Unregister                 = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 8B 79 58");
#endif
		p_ConVar_IsFlagSet                  = g_GameDll.FindPatternSIMD("48 8B 41 48 85 50 38");
		p_ConVar_PrintDescription           = g_GameDll.FindPatternSIMD("B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 2B E0 48 8B 01 48 89 9C 24 ?? ?? ?? ??");

		p_Cbuf_AddText                      = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 63 D9 41 8B F8 48 8D 0D ?? ?? ?? ?? 48 8B F2 FF 15 ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 41 B9 ?? ?? ?? ??");
		p_Cbuf_Execute                      = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 FF 15 ?? ?? ?? ??");
		p_Cmd_ForwardToServer               = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 81 EC ?? ?? ?? ?? 44 8B 59 04");
		p_NullSub                           = g_GameDll.FindPatternSIMD("C2 ?? ?? CC CC CC CC CC CC CC CC CC CC CC CC CC 40 53 48 83 EC 20 48 8D 05 ?? ?? ?? ??");
		p_CallbackStub                      = g_GameDll.FindPatternSIMD("33 C0 C3 CC CC CC CC CC CC CC CC CC CC CC CC CC 80 49 68 08");

		ConCommandBase_IsFlagSet            = p_ConCommandBase_IsFlagSet.RCast<bool (*)(ConCommandBase*, int)>(); /*85 51 38 0F 95 C0 C3*/
		ConCommand_AutoCompleteSuggest      = p_ConCommand_AutoCompleteSuggest.RCast<bool (*)(ConCommand*, const char*, CUtlVector< CUtlString >&)>();

		v_ConVar_IsFlagSet                  = p_ConVar_IsFlagSet.RCast<bool (*)(ConVar*, int)>();
		v_ConVar_Register                   = p_ConVar_Register.RCast<void* (*)(ConVar*, const char*, const char*, int, const char*, bool, float, bool, float, FnChangeCallback_t, const char*)>();
		v_ConVar_Unregister                 = p_ConVar_Unregister.RCast<void (*)(ConVar*)>();
		v_ConVar_PrintDescription           = p_ConVar_PrintDescription.RCast<void* (*)(ConCommandBase*)>();

		Cbuf_AddText                        = p_Cbuf_AddText.RCast<void (*)(ECommandTarget_t, const char*, cmd_source_t)>(); /*48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 63 D9 41 8B F8 48 8D 0D ?? ?? ?? ?? 48 8B F2 FF 15 ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 41 B9 ?? ?? ?? ??*/
		Cbuf_Execute                        = p_Cbuf_Execute.RCast<void (*)(void)>();                                        /*48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 FF 15 ?? ?? ?? ??*/
		v_Cmd_ForwardToServer               = p_Cmd_ForwardToServer.RCast<bool (*)(const CCommand*)>();           /*48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 81 EC ?? ?? ?? ?? 44 8B 59 04*/
		NullSub                             = p_NullSub.RCast<void(*)(void)>();                                   /*C2 00 00 CC CC CC CC CC CC CC CC CC CC CC CC CC 40 53 48 83 EC 20 48 8D 05 ?? ?? ?? ??*/
		CallbackStub                        = p_CallbackStub.RCast<FnCommandCompletionCallback>();                /*33 C0 C3 CC CC CC CC CC CC CC CC CC CC CC CC CC 80 49 68 08*/ /*UserMathErrorFunction*/
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const 
	{
		g_pConCommandVFTable = g_GameDll.GetVirtualMethodTable(".?AVConCommand@@").RCast<ConCommandBase*>();
		g_pConVarVBTable = g_GameDll.GetVirtualMethodTable(".?AVConVar@@", 0).RCast<ConVar*>();
		g_pConVarVFTable = g_GameDll.GetVirtualMethodTable(".?AVConVar@@", 1).RCast<IConVar*>();
	}
	virtual void Attach(void) const;
	virtual void Detach(void) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // CONVAR_H
