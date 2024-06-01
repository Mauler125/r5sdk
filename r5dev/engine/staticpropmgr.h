#pragma once
#include "public/gamebspfile.h"

struct GatherProps_t
{
	// TODO: reverse structure.
	int field_0;
	int field_4;
	int field_8;
	int field_C;
	_BYTE gap10[8];
	int field_18;
	int field_1C;
	int field_20;
	int field_24;
	int field_28;
	__int64 field_30;
	int field_38;
	int field_3C;
	__int64 field_40;
};

class CStaticProp
{
public:
	static void* Init(CStaticProp* thisptr, int64_t a2, unsigned int idx, unsigned int a4, StaticPropLump_t* lump, int64_t a6, int64_t a7);

private: // TODO: reverse structure.
};

inline void*(*CStaticProp__Init)(CStaticProp* thisptr, int64_t a2, unsigned int idx, unsigned int a4, StaticPropLump_t* lump, int64_t a6, int64_t a7);
inline void*(*v_GatherStaticPropsSecondPass_PreInit)(GatherProps_t* gather);
inline void* (*v_GatherStaticPropsSecondPass_PostInit)(GatherProps_t* gather);

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
		g_GameDll.FindPatternSIMD("48 89 4C 24 ?? 55 56 57 41 54 41 55 41 56 41 57 B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 2B E0 48 8D 6C 24 ?? 48 8B 05 ?? ?? ?? ?? 4C 8B F9").GetPtr(v_GatherStaticPropsSecondPass_PreInit);
		g_GameDll.FindPatternSIMD("48 89 4C 24 ?? 55 56 57 41 54 41 55 41 56 41 57 B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 2B E0 48 8D 6C 24 ?? 48 8B 05 ?? ?? ?? ?? 4C 8B F1").GetPtr(v_GatherStaticPropsSecondPass_PostInit);
	}
	virtual void GetVar(void) const
	{

	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
