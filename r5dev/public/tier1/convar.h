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

#include "mathlib/color.h"
#include "public/iconvar.h"
#include "public/iconcommand.h"
#include "tier1/utlvector.h"
#include "tier1/utlstring.h"

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
	virtual ConCommandBase* Create(const char* szName, const char* szHelpString,
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
};
static_assert(sizeof(ConCommandBase) == 0x40);

///////////////////////////////////////////////////////////////////////////////

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

	void* m_nNullCallBack; //0x0040
	void* m_pSubCallback;  //0x0048
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
static_assert(sizeof(ConCommand) == 0x68);

///////////////////////////////////////////////////////////////////////////////

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

	FORCEINLINE bool GetBool(void) const;
	FORCEINLINE float GetFloat(void) const;
	FORCEINLINE int GetInt(void) const;
	FORCEINLINE Color GetColor(void) const;
	FORCEINLINE const char* GetString(void) const;

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
	virtual void CreateInternal(const char* pszName, const char* pszDefaultValue, int nFlags, const char* pszHelpString,
		bool bMin, float fMin, bool bMax, float fMax, FnChangeCallback_t pCallback, const char* pszUsageString) = 0;

	void InstallChangeCallback(FnChangeCallback_t callback, bool bInvoke);
	void RemoveChangeCallback(FnChangeCallback_t callback);

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
};
static_assert(sizeof(ConVar) == 0xA0);

///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a boolean.
// Output : bool
//-----------------------------------------------------------------------------
FORCEINLINE bool ConVar::GetBool(void) const
{
	return !!GetInt();
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a float.
// Output : float
//-----------------------------------------------------------------------------
FORCEINLINE float ConVar::GetFloat(void) const
{
	return m_pParent->m_Value.m_fValue;
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as an integer.
// Output : int
//-----------------------------------------------------------------------------
FORCEINLINE int ConVar::GetInt(void) const
{
	return m_pParent->m_Value.m_nValue;
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a color.
// Output : Color
//-----------------------------------------------------------------------------
FORCEINLINE Color ConVar::GetColor(void) const
{
	unsigned char* pColorElement = (reinterpret_cast<unsigned char*>(&m_pParent->m_Value.m_nValue));
	return Color(pColorElement[0], pColorElement[1], pColorElement[2], pColorElement[3]);
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a string.
// Output : const char *
//-----------------------------------------------------------------------------
FORCEINLINE const char* ConVar::GetString(void) const
{
	if (m_nFlags & FCVAR_NEVER_AS_STRING)
	{
		return "FCVAR_NEVER_AS_STRING";
	}

	char const* str = m_pParent->m_Value.m_pszString;
	return str ? str : "";
}

/* ==== CONVAR ========================================================================================================================================================== */
inline CMemory p_ConVar_Register;
inline void*(*v_ConVar_Register)(ConVar* thisptr, const char* szName, const char* szDefaultValue, int nFlags, const char* szHelpString, bool bMin, float fMin, bool bMax, float fMax, FnChangeCallback_t pCallback, const char* pszUsageString);

inline CMemory p_ConVar_Unregister;
inline void(*v_ConVar_Unregister)(ConVar* thisptr);

inline CMemory p_ConVar_IsFlagSet;
inline bool(*v_ConVar_IsFlagSet)(ConVar* pConVar, int nFlag);

inline ConCommandBase* g_pConCommandBaseVFTable;
inline ConCommand* g_pConCommandVFTable;
inline ConVar* g_pConVarVBTable;
inline IConVar* g_pConVarVFTable;

///////////////////////////////////////////////////////////////////////////////
class VConVar : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogConAdr("ConCommandBase::`vftable'", reinterpret_cast<uintptr_t>(g_pConCommandBaseVFTable));
		LogConAdr("ConCommand::`vftable'", reinterpret_cast<uintptr_t>(g_pConCommandVFTable));
		LogConAdr("ConVar::`vbtable'", reinterpret_cast<uintptr_t>(g_pConVarVBTable));
		LogConAdr("ConVar::`vftable'", reinterpret_cast<uintptr_t>(g_pConVarVFTable));
		LogFunAdr("ConVar::Register", p_ConVar_Register.GetPtr());
		LogFunAdr("ConVar::Unregister", p_ConVar_Unregister.GetPtr());
		LogFunAdr("ConVar::IsFlagSet", p_ConVar_IsFlagSet.GetPtr());
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_ConVar_Register                   = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 56 48 83 EC 30 F3 0F 10 44 24 ??");
		p_ConVar_Unregister                 = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 57 48 83 EC 20 48 8B 59 58 48 8D 05 ?? ?? ?? ??");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_ConVar_Register                   = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC 40 F3 0F 10 84 24 ?? ?? ?? ??");
		p_ConVar_Unregister                 = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 8B 79 58");
#endif
		p_ConVar_IsFlagSet                  = g_GameDll.FindPatternSIMD("48 8B 41 48 85 50 38");

		v_ConVar_IsFlagSet                  = p_ConVar_IsFlagSet.RCast<bool (*)(ConVar*, int)>();
		v_ConVar_Register                   = p_ConVar_Register.RCast<void* (*)(ConVar*, const char*, const char*, int, const char*, bool, float, bool, float, FnChangeCallback_t, const char*)>();
		v_ConVar_Unregister                 = p_ConVar_Unregister.RCast<void (*)(ConVar*)>();
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const
	{
		g_pConCommandBaseVFTable = g_GameDll.GetVirtualMethodTable(".?AVConCommandBase@@").RCast<ConCommandBase*>();
		g_pConCommandVFTable = g_GameDll.GetVirtualMethodTable(".?AVConCommand@@").RCast<ConCommand*>();
		g_pConVarVBTable = g_GameDll.GetVirtualMethodTable(".?AVConVar@@", 0).RCast<ConVar*>();
		g_pConVarVFTable = g_GameDll.GetVirtualMethodTable(".?AVConVar@@", 1).RCast<IConVar*>();
	}
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////

#endif // CONVAR_H
