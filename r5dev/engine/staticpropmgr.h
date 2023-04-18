#pragma once

struct StaticPropLump_t
{
	Vector3D m_Origin;
	Vector3D m_Angles;
	float m_Scale;
	short m_PropType;
	char m_Solid;
	char m_Flags;
	short m_Skin;
	short m_EnvCubemap;
	float m_FadeDist;
	Vector3D m_LightingOrigin;
	int m_DiffuseModulation;
	char gap_38[4];
	int m_collisionFlagsRemove;
};

inline CMemory p_CStaticProp_Init;
inline auto v_CStaticProp_Init = p_CStaticProp_Init.RCast<void*(*)(int64_t thisptr, int64_t a2, unsigned int idx, unsigned int a4, StaticPropLump_t* lump, int64_t a6, int64_t a7)>();

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
	virtual void Attach(void) const;
	virtual void Detach(void) const;
};
///////////////////////////////////////////////////////////////////////////////
