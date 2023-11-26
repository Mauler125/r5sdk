#pragma once
#include "squirrel.h"
#include "sqvm.h"

extern bool g_bSQAuxError;
extern bool g_bSQAuxBadLogic;
extern HSQUIRRELVM g_pErrorVM;

inline CMemory p_sqstd_aux_printerror;
inline SQInteger(*v_sqstd_aux_printerror)(HSQUIRRELVM v);

inline CMemory p_sqstd_aux_badlogic;
inline SQInteger(*v_sqstd_aux_badlogic)(HSQUIRRELVM v, __m128i* a2, __m128i* a3);

///////////////////////////////////////////////////////////////////////////////
class VSquirrelAUX : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("sqstd_aux_printerror", p_sqstd_aux_printerror.GetPtr());
		LogFunAdr("sqstd_aux_badlogic", p_sqstd_aux_badlogic.GetPtr());
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_sqstd_aux_printerror = g_GameDll.FindPatternSIMD("40 53 55 56 57 41 54 41 55 41 57 48 81 EC ?? ?? ?? ??");
		v_sqstd_aux_printerror = p_sqstd_aux_printerror.RCast<SQInteger(*)(HSQUIRRELVM)>(); /*40 53 55 56 57 41 54 41 55 41 57 48 81 EC ?? ?? ?? ??*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_sqstd_aux_printerror = g_GameDll.FindPatternSIMD("40 53 56 57 41 54 41 55 41 56 41 57 48 81 EC ?? ?? ?? ?? FF 05 ?? ?? ?? ??");
		v_sqstd_aux_printerror = p_sqstd_aux_printerror.RCast<SQInteger(*)(HSQUIRRELVM)>(); /*40 53 56 57 41 54 41 55 41 56 41 57 48 81 EC ?? ?? ?? ?? FF 05 ?? ?? ?? ??*/
#endif
		p_sqstd_aux_badlogic = g_GameDll.FindPatternSIMD("48 8B C4 55 48 8B EC 48 83 EC 70 41 0F 10 ??");
		v_sqstd_aux_badlogic = p_sqstd_aux_badlogic.RCast<SQInteger(*)(HSQUIRRELVM, __m128i*, __m128i*)>(); /*48 8B C4 55 48 8B EC 48 83 EC 70 41 0F 10 00*/
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
