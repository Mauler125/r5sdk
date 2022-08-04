#pragma once
#include "tier1/cmd.h"
#include "mathlib/color.h"
#include "public/include/iconvar.h"
#include "tier1/utlvector.h"

//-----------------------------------------------------------------------------
// Purpose: A console variable
//-----------------------------------------------------------------------------
class ConVar : public ConCommandBase
{
public:
	ConVar(void){};
	ConVar(const char* pszName, const char* pszDefaultValue, int nFlags, const char*pszHelpString,
		bool bMin, float fMin, bool bMax, float fMax, FnChangeCallback_t pCallback, const char* pszUsageString);
	~ConVar(void);

	void Init(void) const;
	void InitShipped(void) const;

	void PurgeShipped(void) const;
	void PurgeHostNames(void) const;

	void AddFlags(int nFlags);
	void RemoveFlags(int nFlags);

	const char* GetBaseName(void) const;
	const char* GetHelpText(void) const;
	const char* GetUsageText(void) const;

	bool GetBool(void) const;
	float GetFloat(void) const;
	double GetDouble(void) const;
	int GetInt(void) const;
	int64_t GetInt64(void) const;
	size_t GetSizeT(void) const;
	Color GetColor(void) const;
	const char* GetString(void) const;

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

	void InternalSetValue(const char* pszValue);
	void InternalSetIntValue(int nValue);
	void InternalSetFloatValue(float flValue);
	void InternalSetColorValue(Color value);

	void Revert(void);
	bool ClampValue(float& flValue);

	const char* GetDefault(void) const;
	void SetDefault(const char* pszDefault);
	bool SetColorFromString(const char* pszValue);

	void ChangeStringValue(const char* pszTempValue);
	void ChangeStringValueUnsafe(const char* pszNewValue);

	void InstallChangeCallback(FnChangeCallback_t callback, bool bInvoke);
	void RemoveChangeCallback(FnChangeCallback_t callback);

	bool IsRegistered(void) const;
	bool IsCommand(void) const;
	static bool IsFlagSet(ConVar* pConVar, int nFlags);

	struct CVValue_t
	{
		char*      m_pszString;
		size_t     m_iStringLength;
		float      m_fValue;
		int        m_nValue;
	};

	IConVar*       m_pIConVarVFTable{}; //0x0040
	ConVar*        m_pParent        {}; //0x0048
	const char*    m_pszDefaultValue{}; //0x0050
	CVValue_t      m_Value          {}; //0c0058
	bool           m_bHasMin        {}; //0x0070
	float          m_fMinVal        {}; //0x0074
	bool           m_bHasMax        {}; //0x0078
	float          m_fMaxVal        {}; //0x007C
	CUtlVector<FnChangeCallback_t> m_fnChangeCallbacks; //0x0080
}; //Size: 0x00A0
static_assert(sizeof(ConVar) == 0xA0);

/* ==== ICONVAR ========================================================================================================================================================= */
inline CMemory p_IConVar_IsFlagSet;
inline auto IConVar_IsFlagSet = p_IConVar_IsFlagSet.RCast<bool (*)(ConVar* pConVar, int nFlag)>();

inline CMemory p_ConVar_SetInfo;
inline auto ConVar_SetInfo = p_ConVar_SetInfo.RCast<void* (*)(ConVar* thisptr, int a2, int a3, int a4, void* a5)>();

inline CMemory p_ConVar_Register;
inline auto ConVar_Register = p_ConVar_Register.RCast<void* (*)(ConVar* thisptr, const char* szName, const char* szDefaultValue, int nFlags, const char* szHelpString, bool bMin, float fMin, bool bMax, float fMax, FnChangeCallback_t pCallback, const char* pszUsageString)>();

inline CMemory g_pConVarVFTable;
inline CMemory g_pIConVarVFTable;

///////////////////////////////////////////////////////////////////////////////
void IConVar_Attach();
void IConVar_Detach();

extern ConVar* g_pConVar;

///////////////////////////////////////////////////////////////////////////////
class VConVar : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: ConVar::IsFlagSet                    : {:#18x} |\n", p_IConVar_IsFlagSet.GetPtr());
		spdlog::debug("| FUN: ConVar::SetInfo                      : {:#18x} |\n", p_ConVar_SetInfo.GetPtr());
		spdlog::debug("| FUN: ConVar::Register                     : {:#18x} |\n", p_ConVar_Register.GetPtr());
		spdlog::debug("| VAR: g_pConVarVtable                      : {:#18x} |\n", g_pConVarVFTable.GetPtr());
		spdlog::debug("| VAR: g_pIConVarVtable                     : {:#18x} |\n", g_pIConVarVFTable.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_IConVar_IsFlagSet = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\x41\x48\x85\x50\x38"), "xxxxxxx");
		p_ConVar_SetInfo = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x60\x48\x8B\xD9\xC6\x41\x10\x00\x33\xC9\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x4C\x24\x00\x0F\x57\xC0\x48\x89\x4C\x24\x00\x48\x89\x03\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x43\x40"), "xxxxxxxxxxxxxxxxxx????xxxx?xxxxxxx?xxxxxx????xxxx");
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_ConVar_Register = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x41\x56\x48\x83\xEC\x30\xF3\x0F\x10\x44\x24\x00"), "xxxx?xxxx?xxxx?xxxx?xxxxxxxxxxx?");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_ConVar_Register = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x40\xF3\x0F\x10\x84\x24\x00\x00\x00\x00"), "xxxx?xxxx?xxxx?xxxxxxxxxx????");
#endif
		IConVar_IsFlagSet = p_IConVar_IsFlagSet.RCast<bool (*)(ConVar*, int)>();             /*48 8B 41 48 85 50 38*/
		ConVar_SetInfo = p_ConVar_SetInfo.RCast<void* (*)(ConVar*, int, int, int, void*)>(); /*40 53 48 83 EC 60 48 8B D9 C6 41 10 00 33 C9 48 8D 05 ? ? ? ? 48 89 4C 24 ? 0F 57 C0 48 89 4C 24 ? 48 89 03 48 8D 05 ? ? ? ? 48 89 43 40*/
		ConVar_Register = p_ConVar_Register.RCast<void* (*)(ConVar*, const char*, const char*, int, const char*, bool, float, bool, float, FnChangeCallback_t, const char*)>(); /*48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 40 F3 0F 10 84 24 ? ? ? ?*/
	}
	virtual void GetVar(void) const
	{
		g_pConVarVFTable  = p_ConVar_SetInfo.Offset(0x00).FindPatternSelf("48 8D 05", CMemory::Direction::DOWN, 100).ResolveRelativeAddressSelf(0x3, 0x7).GetPtr(); // Get vtable ptr for ConVar table.
		g_pIConVarVFTable = p_ConVar_SetInfo.Offset(0x16).FindPatternSelf("48 8D 05", CMemory::Direction::DOWN, 100).ResolveRelativeAddressSelf(0x3, 0x7).GetPtr(); // Get vtable ptr for ICvar table.
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VConVar);
