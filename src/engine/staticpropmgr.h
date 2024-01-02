#pragma once
#include "public/gamebspfile.h"

class CStaticProp
{
public:
	static void* Init(CStaticProp* thisptr, int64_t a2, unsigned int idx, unsigned int a4, StaticPropLump_t* lump, int64_t a6, int64_t a7);

private: // TODO: reverse structure.
};

inline void*(*CStaticProp__Init)(CStaticProp* thisptr, int64_t a2, unsigned int idx, unsigned int a4, StaticPropLump_t* lump, int64_t a6, int64_t a7);

///////////////////////////////////////////////////////////////////////////////
class VStaticPropMgr : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CStaticProp::Init", CStaticProp__Init);

	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 8B C4 44 89 40 18 48 89 50 10 55").GetPtr(CStaticProp__Init);
	}
	virtual void GetVar(void) const
	{

	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
