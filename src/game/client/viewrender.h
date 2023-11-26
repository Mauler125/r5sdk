#pragma once

//-------------------------------------------------------------------------------------
// Forward declarations
//-------------------------------------------------------------------------------------
class VMatrix;

class CViewRender
{
public:
	VMatrix* GetWorldMatrixForView(int8_t slot);
};

///////////////////////////////////////////////////////////////////////////////
const Vector3D& MainViewOrigin();
const QAngle& MainViewAngles();

inline CMemory p_CViewRender_GetWorldMatrixForView;
inline VMatrix*(*CViewRender_GetWorldMatrixForView)(CViewRender*, int8_t);

inline Vector3D* g_vecRenderOrigin = nullptr;
inline QAngle* g_vecRenderAngles = nullptr;

inline CViewRender* g_pViewRender = nullptr;
inline CMemory g_pViewRender_VFTable;

///////////////////////////////////////////////////////////////////////////////
class V_ViewRender : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogConAdr("CViewRender::`vftable'", g_pViewRender_VFTable.GetPtr());
		LogFunAdr("CViewRender::GetWorldMatrixForView", p_CViewRender_GetWorldMatrixForView.GetPtr());
		LogVarAdr("g_ViewRender", reinterpret_cast<uintptr_t>(g_pViewRender));
		LogVarAdr("g_vecRenderOrigin", reinterpret_cast<uintptr_t>(g_vecRenderOrigin));
		LogVarAdr("g_vecRenderAngles", reinterpret_cast<uintptr_t>(g_vecRenderAngles));
	}
	virtual void GetFun(void) const
	{
		p_CViewRender_GetWorldMatrixForView = g_pViewRender_VFTable.WalkVTable(16).Deref(); // 16th vfunc.
		CViewRender_GetWorldMatrixForView = p_CViewRender_GetWorldMatrixForView.RCast<VMatrix* (*)(CViewRender*, int8_t)>();
	}
	virtual void GetVar(void) const
	{
		CMemory base = g_GameDll.FindPatternSIMD("48 89 74 24 ?? 57 48 83 EC 30 F3 0F 10 05 ?? ?? ?? ?? ?? 8B ??");

		g_vecRenderOrigin = base.Offset(0x00).FindPatternSelf("F3 0F 10 05").ResolveRelativeAddressSelf(0x4, 0x8).RCast<Vector3D*>();
		g_vecRenderAngles = base.Offset(0x30).FindPatternSelf("F3 0F 10 0D").ResolveRelativeAddressSelf(0x4, 0x8).RCast<QAngle*>();

		g_pViewRender = g_GameDll.FindPatternSIMD("48 8D 05 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC CC CC 48 8B C4").ResolveRelativeAddressSelf(0x3, 0x7).RCast<CViewRender*>(); /*48 8D 05 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC CC CC 48 8B C4*/
	}
	virtual void GetCon(void) const
	{
		g_pViewRender_VFTable = g_GameDll.GetVirtualMethodTable(".?AVCViewRender@@");
	}
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////
