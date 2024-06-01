#pragma once
#include "squirrel.h"
#include "sqvm.h"

extern bool g_bSQAuxError;
extern bool g_bSQAuxBadLogic;
extern HSQUIRRELVM g_pErrorVM;

inline SQInteger(*v_sqstd_aux_printerror)(HSQUIRRELVM v);
inline SQInteger(*v_sqstd_aux_badlogic)(HSQUIRRELVM v, __m128i* a2, __m128i* a3);

///////////////////////////////////////////////////////////////////////////////
class VSquirrelAUX : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("sqstd_aux_printerror", v_sqstd_aux_printerror);
		LogFunAdr("sqstd_aux_badlogic", v_sqstd_aux_badlogic);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("40 53 56 57 41 54 41 55 41 56 41 57 48 81 EC ?? ?? ?? ?? FF 05 ?? ?? ?? ??").GetPtr(v_sqstd_aux_printerror);
		g_GameDll.FindPatternSIMD("48 8B C4 55 48 8B EC 48 83 EC 70 41 0F 10 ??").GetPtr(v_sqstd_aux_badlogic);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
