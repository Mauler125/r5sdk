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
#include "public/icvar.h"
#include "public/iconvar.h"
#include "tier1/utlmap.h"
#include "tier1/utlvector.h"
#include "tier1/utlstring.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class ConCommandBase;

//-----------------------------------------------------------------------------
// Purpose: Interface to ConVars/ConCommands
//-----------------------------------------------------------------------------
class CCvar : public CBaseAppSystem< ICvar >
{ 	// Implementation in engine.
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
		virtual bool            IsValid(void) = 0;
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

extern CCvar* g_pCVar;
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// Purpose: ConVar tools
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
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// Purpose: Console variable flags container for tools
//-----------------------------------------------------------------------------
class ConVarFlags
{
public:
	ConVarFlags();
	void SetFlag(const int nFlag, const char* szDesc, const char* szShortDesc);

	struct FlagDesc_t
	{
		int			bit;
		const char* desc;
		const char* shortdesc;
	};

	CUtlMap<const char*, int> m_StringToFlags;
	FlagDesc_t m_FlagsToDesc[33];
	int m_Count;
};

extern ConVarFlags g_ConVarFlags;

///////////////////////////////////////////////////////////////////////////////
bool ConVar_ParseFlagString(const char* pszFlags, int& nFlags, const char* pszConVarName = "<<unspecified>>");
void ConVar_PrintDescription(ConCommandBase* pVar);

inline CMemory p_ConVar_PrintDescription;
inline void (*v_ConVar_PrintDescription)(ConCommandBase* pVar);

///////////////////////////////////////////////////////////////////////////////
class VCVar : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("ConVar_PrintDescription", p_ConVar_PrintDescription.GetPtr());
		LogVarAdr("g_pCVar", reinterpret_cast<uintptr_t>(g_pCVar));
	}
	virtual void GetFun(void) const 
	{
		p_ConVar_PrintDescription = g_GameDll.FindPatternSIMD("B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 2B E0 48 8B 01 48 89 9C 24 ?? ?? ?? ??");
		v_ConVar_PrintDescription = p_ConVar_PrintDescription.RCast<void (*)(ConCommandBase*)>();
	}
	virtual void GetVar(void) const
	{
		g_pCVar = g_GameDll.FindPatternSIMD("48 83 EC 28 48 8B 05 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? 48 85 C0 48 89 15")
			.FindPatternSelf("48 8D 0D", CMemory::Direction::DOWN, 40).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CCvar*>();

		//g_pCVar = g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 48 83 3D ?? ?? ?? ?? ?? 48 8B D9 74 09") // Actual CCvar, above is the vtable ptr.
			//.FindPatternSelf("48 83 3D", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x8).RCast<CCvar*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // CVAR_H
