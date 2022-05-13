#pragma once


extern bool g_bSQAuxError;

inline CMemory p_sqstd_aux_printerror;
inline auto v_sqstd_aux_printerror = p_sqstd_aux_printerror.RCast<SQInteger(*)(HSQUIRRELVM v)>();


void SQAUX_Attach();
void SQAUX_Detach();
///////////////////////////////////////////////////////////////////////////////
class VSqStdAux : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: sqstd_aux_printerror                 : {:#18x} |\n", p_sqstd_aux_printerror.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_sqstd_aux_printerror = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x55\x56\x57\x41\x54\x41\x55\x41\x57\x48\x81\xEC\x00\x00\x00\x00"), "xxxxxxxxxxxxxx????");
		v_sqstd_aux_printerror = p_sqstd_aux_printerror.RCast<SQInteger(*)(HSQUIRRELVM)>(); /*40 53 55 56 57 41 54 41 55 41 57 48 81 EC ?? ?? ?? ??*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_sqstd_aux_printerror = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00\xFF\x05\x00\x00\x00\x00"), "xxxxxxxxxxxxxxx????xx????");
		v_sqstd_aux_printerror = p_sqstd_aux_printerror.RCast<SQInteger(*)(HSQUIRRELVM)>(); /*40 53 56 57 41 54 41 55 41 56 41 57 48 81 EC ?? ?? ?? ?? FF 05 ?? ?? ?? ??*/
#endif
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VSqStdAux);
