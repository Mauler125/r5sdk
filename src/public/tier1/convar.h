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

	virtual bool IsCommand(void) const;
	virtual bool IsFlagSet(const int nFlags) const;

	virtual void AddFlags(const int nFlags);
	virtual void RemoveFlags(const int nFlags);

	virtual int GetFlags(void) const;
	virtual const char* GetName(void) const;
	virtual const char* GetHelpText(void) const;
	virtual const char* GetUsageText(void) const;

	virtual void SetAccessor(IConCommandBaseAccessor* const pAccessor);
	virtual bool IsRegistered(void) const;

	virtual int GetDLLIdentifier() const;
	virtual ConCommandBase* Create(const char* szName, const char* szHelpString,
		int nFlags, const char* pszUsageString);

	virtual void				Init();
	void						Shutdown();

	ConCommandBase* GetNext(void) const;
	char* CopyString(const char* szFrom) const;

//private:
	// Next ConVar in chain
	// Prior to register, it points to the next convar in the DLL.
	// Once registered, though, m_pNext is reset to point to the next
	// convar in the global list
	ConCommandBase*          m_pNext;          //0x0008

	// Has the cvar been added to the global list?
	bool                     m_bRegistered;    //0x0010

	// Static data.
	const char*              m_pszName;        //0x0018
	const char*              m_pszHelpString;  //0x0020
	const char*              m_pszUsageString; //0x0028

	IConCommandBaseAccessor* m_pAccessor;      //0x0030 <-- unused since executable is monolithic.

	// ConVar flags
	int                      m_nFlags;         //0x0038

	// ConVars add themselves to this list for the executable. 
	// Then ConVar_Register runs through  all the console variables 
	// and registers them into a global list stored in vstdlib.dll
	static ConCommandBase* s_pConCommandBases;

	// ConVars in this executable use this 'global' to access values.
	static IConCommandBaseAccessor* s_pAccessor;
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
	typedef ConCommandBase BaseClass;

	ConCommand(const char* pName, FnCommandCallbackV1_t callback,
		const char* pHelpString = 0, int flags = 0, FnCommandCompletionCallback completionFunc = 0, const char* pszUsageString = 0);
	ConCommand(const char* pName, FnCommandCallback_t callback,
		const char* pHelpString = 0, int flags = 0, FnCommandCompletionCallback completionFunc = 0, const char* pszUsageString = 0);
	ConCommand(const char* pName, ICommandCallback* pCallback,
		const char* pHelpString = 0, int flags = 0, ICommandCompletionCallback* pCommandCompletionCallback = 0, const char* pszUsageString = 0);

	virtual ~ConCommand(void);

	virtual	bool IsCommand(void) const;

	virtual int AutoCompleteSuggest(const char* partial, CUtlVector< CUtlString >& commands);
	virtual bool CanAutoComplete(void) const;

	// Invoke the function
	virtual void Dispatch(const CCommand& command);

//private:
	void* m_nNullCallBack; //0x0040
	void* m_pSubCallback;  //0x0048

	// NOTE: To maintain backward compatibility, we have to be very careful:
	// All public virtual methods must appear in the same order always
	// since engine code will be calling into this code, which *does not match*
	// in the mod code; it's using slightly different, but compatible versions
	// of this class. Also: Be very careful about adding new fields to this class.
	// Those fields will not exist in the version of this class that is instanced
	// in mod code.

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

//-----------------------------------------------------------------------------
// Called by the framework to register ConCommands with the ICVar
//-----------------------------------------------------------------------------
void ConVar_Register(int nCVarFlag = 0, IConCommandBaseAccessor* pAccessor = NULL);
void ConVar_Unregister();

/* ==== CONVAR ========================================================================================================================================================== */
inline void*(*ConVar__Register)(ConVar* thisptr, const char* szName, const char* szDefaultValue, int nFlags, const char* szHelpString, bool bMin, float fMin, bool bMax, float fMax, FnChangeCallback_t pCallback, const char* pszUsageString);
inline void(*ConVar__Unregister)(ConVar* thisptr);
inline bool(*ConVar__IsFlagSet)(ConVar* pConVar, int nFlag);

inline ConCommandBase* g_pConCommandBaseVFTable;
inline ConCommand* g_pConCommandVFTable;
inline ConVar* g_pConVarVBTable;
inline IConVar* g_pConVarVFTable;

///////////////////////////////////////////////////////////////////////////////
class VConVar : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogConAdr("ConCommandBase::`vftable'", g_pConCommandBaseVFTable);
		LogConAdr("ConCommand::`vftable'", g_pConCommandVFTable);
		LogConAdr("ConVar::`vbtable'", g_pConVarVBTable);
		LogConAdr("ConVar::`vftable'", g_pConVarVFTable);
		LogFunAdr("ConVar::Register", ConVar__Register);
		LogFunAdr("ConVar::Unregister", ConVar__Unregister);
		LogFunAdr("ConVar::IsFlagSet", ConVar__IsFlagSet);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC 40 F3 0F 10 84 24 ?? ?? ?? ??").GetPtr(ConVar__Register);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 8B 79 58").GetPtr(ConVar__Unregister);
		g_GameDll.FindPatternSIMD("48 8B 41 48 85 50 38").GetPtr(ConVar__IsFlagSet);
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
