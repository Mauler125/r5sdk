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

#ifndef CVAR_H
#define CVAR_H

#include "vstdlib/concommandhash.h"

/* ==== CCVAR =========================================================================================================================================================== */
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CCvarUtilities
{
public:
	//bool IsCommand(const CCommand& args);

	// Writes lines containing "set variable value" for all variables
	// with the archive flag set to true.
	//void WriteVariables(CUtlBuffer& buff, bool bAllVars);

	// Returns the # of cvars with the server flag set.
	int	CountVariablesWithFlags(int flags);

	// Enable cvars marked with FCVAR_DEVELOPMENTONLY
	void EnableDevCvars();

	// Lists cvars to console
	void CvarList(const CCommand& args);

	// Prints help text for cvar
	void CvarHelp(const CCommand& args);

	// Revert all cvar values
	//void CvarRevert(const CCommand& args);

	// Revert all cvar values
	void CvarDifferences(const CCommand& args);

	// Toggles a cvar on/off, or cycles through a set of values
	//void CvarToggle(const CCommand& args);

	// Finds commands with a specified flag.
	void CvarFindFlags_f(const CCommand& args);


	int CvarFindFlagsCompletionCallback(const char* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);

private:
	// just like Cvar_set, but optimizes out the search
	//void SetDirect(ConVar* var, const char* value);

	//bool IsValidToggleCommand(const char* cmd);
};

extern CCvarUtilities* cv;

class CCvar : public CBaseAppSystem< ICvar >
{ 	// Implementation in engine.
public:
	unordered_map<string, ConCommandBase*> DumpToMap(void);

protected:
	enum ConVarSetType_t
	{
		CONVAR_SET_STRING = 0,
		CONVAR_SET_INT,
		CONVAR_SET_FLOAT,
	};

	struct QueuedConVarSet_t
	{
		ConVar* m_pConVar;
		ConVarSetType_t m_nType;
		int m_nInt;
		float m_flFloat;
		CUtlString m_String;
	};

	class CCVarIteratorInternal : public ICVarIteratorInternal
	{
	public:

		virtual void            SetFirst(void) = 0;
		virtual void            Next(void) = 0;
		virtual	bool            IsValid(void) = 0;
		virtual ConCommandBase* Get(void) = 0;

		virtual ~CCVarIteratorInternal() { }

		CCvar* const m_pOuter = nullptr;
		CConCommandHash* const m_pHash = nullptr;
		CConCommandHash::CCommandHashIterator_t m_hashIter;
	};

	virtual CCVarIteratorInternal* FactoryInternalIterator(void) = 0;

	friend class CCVarIteratorInternal;
	friend class CCvarUtilities;

private:
	CUtlVector< FnChangeCallback_t > m_GlobalChangeCallbacks;
	char pad0[30];           //!TODO:
	int m_nNextDLLIdentifier;
	ConCommandBase* m_pConCommandList;
	CConCommandHash m_CommandHash;
	CUtlVector<void*> m_Unknown;
	char pad2[32];
	void* m_pCallbackStub;
	void* m_pAllocFunc;
	char pad3[16];
	CUtlVector< QueuedConVarSet_t > m_QueuedConVarSets;
	bool m_bMaterialSystemThreadSetAllowed;
};

///////////////////////////////////////////////////////////////////////////////
extern CCvar* g_pCVar;

/* ==== CONVAR ========================================================================================================================================================== */
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
	FORCEINLINE double GetDouble(void) const;
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

	static bool ParseFlagString(const char* pszFlags, int& nFlags, const char* pszConVarName = nullptr);

	struct CVValue_t
	{
		char* m_pszString;
		size_t     m_iStringLength;
		float      m_fValue;
		int        m_nValue;
	};

	ConVar* m_pParent;         //0x0048
	const char* m_pszDefaultValue; //0x0050
	CVValue_t      m_Value;           //0c0058
	bool           m_bHasMin;         //0x0070
	float          m_fMinVal;         //0x0074
	bool           m_bHasMax;         //0x0078
	float          m_fMaxVal;         //0x007C
	CUtlVector<FnChangeCallback_t> m_fnChangeCallbacks; //0x0080
}; //Size: 0x00A0
static_assert(sizeof(ConVar) == 0xA0);

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
// Purpose: Return ConVar value as a double.
// Output : double
//-----------------------------------------------------------------------------
FORCEINLINE double ConVar::GetDouble(void) const
{
	return static_cast<double>(m_pParent->m_Value.m_fValue);
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

///////////////////////////////////////////////////////////////////////////////
// see iconvar.h
static std::map<string, int> s_ConVarFlags = {
	{"NONE", FCVAR_NONE},
	{"DEVELOPMENTONLY", FCVAR_DEVELOPMENTONLY},
	{"GAMEDLL", FCVAR_GAMEDLL},
	{"CLIENTDLL", FCVAR_CLIENTDLL},
	{"HIDDEN", FCVAR_HIDDEN},
	{"PROTECTED", FCVAR_PROTECTED},
	{"SPONLY", FCVAR_SPONLY},
	{"ARCHIVE", FCVAR_ARCHIVE},
	{"NOTIFY", FCVAR_NOTIFY},
	{"USERINFO", FCVAR_USERINFO},
	{"PRINTABLEONLY", FCVAR_PRINTABLEONLY},
	{"GAMEDLL_FOR_REMOTE_CLIENTS", FCVAR_GAMEDLL_FOR_REMOTE_CLIENTS},
	{"UNLOGGED", FCVAR_UNLOGGED},
	{"NEVER_AS_STRING", FCVAR_NEVER_AS_STRING},
	{"REPLICATED", FCVAR_REPLICATED},
	{"CHEAT", FCVAR_CHEAT},
	{"SS", FCVAR_SS},
	{"DEMO", FCVAR_DEMO},
	{"DONTRECORD", FCVAR_DONTRECORD},
	{"SS_ADDED", FCVAR_SS_ADDED},
	{"RELEASE", FCVAR_RELEASE},
	{"RELOAD_MATERIALS", FCVAR_RELOAD_MATERIALS},
	{"RELOAD_TEXTURES", FCVAR_RELOAD_TEXTURES},
	{"NOT_CONNECTED", FCVAR_NOT_CONNECTED},
	{"MATERIAL_SYSTEM_THREAD", FCVAR_MATERIAL_SYSTEM_THREAD},
	{"ARCHIVE_PLAYERPROFILE", FCVAR_ARCHIVE_PLAYERPROFILE},
};

///////////////////////////////////////////////////////////////////////////////
void ConVar_PrintDescription(ConCommandBase* pVar);

/* ==== CONVAR ========================================================================================================================================================== */
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
class VCVar : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogConAdr("ConCommand::`vftable'", reinterpret_cast<uintptr_t>(g_pConCommandVFTable));
		LogConAdr("ConVar::`vbtable'", reinterpret_cast<uintptr_t>(g_pConVarVBTable));
		LogConAdr("ConVar::`vftable'", reinterpret_cast<uintptr_t>(g_pConVarVFTable));
		LogFunAdr("ConVar::Register", p_ConVar_Register.GetPtr());
		LogFunAdr("ConVar::Unregister", p_ConVar_Unregister.GetPtr());
		LogFunAdr("ConVar::IsFlagSet", p_ConVar_IsFlagSet.GetPtr());
		LogFunAdr("ConVar_PrintDescription", p_ConVar_PrintDescription.GetPtr());
		LogVarAdr("g_pCVar", reinterpret_cast<uintptr_t>(g_pCVar));
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
		p_ConVar_PrintDescription           = g_GameDll.FindPatternSIMD("B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 2B E0 48 8B 01 48 89 9C 24 ?? ?? ?? ??");

		v_ConVar_IsFlagSet                  = p_ConVar_IsFlagSet.RCast<bool (*)(ConVar*, int)>();
		v_ConVar_Register                   = p_ConVar_Register.RCast<void* (*)(ConVar*, const char*, const char*, int, const char*, bool, float, bool, float, FnChangeCallback_t, const char*)>();
		v_ConVar_Unregister                 = p_ConVar_Unregister.RCast<void (*)(ConVar*)>();
		v_ConVar_PrintDescription           = p_ConVar_PrintDescription.RCast<void* (*)(ConCommandBase*)>();
	}
	virtual void GetVar(void) const
	{
		g_pCVar = g_GameDll.FindPatternSIMD("48 83 EC 28 48 8B 05 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? 48 85 C0 48 89 15")
			.FindPatternSelf("48 8D 0D", CMemory::Direction::DOWN, 40).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CCvar*>();

		//g_pCVar = g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 48 83 3D ?? ?? ?? ?? ?? 48 8B D9 74 09") // Actual CCvar, above is the vtable ptr.
			//.FindPatternSelf("48 83 3D", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x8).RCast<CCvar*>();
	}
	virtual void GetCon(void) const
	{
		g_pConVarVBTable = g_GameDll.GetVirtualMethodTable(".?AVConVar@@", 0).RCast<ConVar*>();
		g_pConVarVFTable = g_GameDll.GetVirtualMethodTable(".?AVConVar@@", 1).RCast<IConVar*>();
	}
	virtual void Attach(void) const;
	virtual void Detach(void) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // CVAR_H
