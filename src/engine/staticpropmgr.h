#pragma once
#include "public/gamebspfile.h"

inline CMemory p_CStaticProp_Init;
inline void*(*v_CStaticProp_Init)(int64_t thisptr, int64_t a2, unsigned int idx, unsigned int a4, StaticPropLump_t* lump, int64_t a6, int64_t a7);

void* __fastcall CStaticProp_Init(int64_t thisptr, int64_t a2, unsigned int idx, unsigned int a4, StaticPropLump_t* lump, int64_t a6, int64_t a7);

///////////////////////////////////////////////////////////////////////////////
class VStaticPropMgr : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CStaticProp::Init", p_CStaticProp_Init.GetPtr());

	}
	virtual void GetFun(void) const
	{
		p_CStaticProp_Init = g_GameDll.FindPatternSIMD("48 8B C4 44 89 40 18 48 89 50 10 55"); /*48 8B C4 44 89 40 18 48 89 50 10 55*/
		v_CStaticProp_Init = p_CStaticProp_Init.RCast<void*(*)(int64_t, int64_t, unsigned int, unsigned int, StaticPropLump_t*, int64_t, int64_t)>();
	}
	virtual void GetVar(void) const
	{

	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
