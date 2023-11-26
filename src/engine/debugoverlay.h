#pragma once
#include "mathlib/vector.h"
#include "mathlib/vector4d.h"
#include "mathlib/color.h"
#include "mathlib/ssemath.h"

// Something has to be hardcoded..
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1) || defined (GAMEDLL_S2)

constexpr auto MATERIALSYSTEM_VCALL_OFF_0 = 0x3F8;
constexpr auto CMATQUEUEDRENDERCONTEXT_VCALL_OFS_0 = 0x278;
constexpr auto CMATQUEUEDRENDERCONTEXT_VCALL_OFS_1 = 0x280;

#elif defined (GAMEDLL_S3)

constexpr auto MATERIALSYSTEM_VCALL_OFF_0 = 0x3F0;
constexpr auto CMATQUEUEDRENDERCONTEXT_VCALL_OFS_0 = 0x288;
constexpr auto CMATQUEUEDRENDERCONTEXT_VCALL_OFS_1 = 0x290;

#endif
constexpr auto CMATQUEUEDRENDERCONTEXT_VCALL_OFS_2 = 0x8;
constexpr auto NDEBUG_PERSIST_TILL_NEXT_SERVER = (0.01023f);

enum class OverlayType_t
{
	OVERLAY_BOX = 0,
	OVERLAY_SPHERE,
	OVERLAY_LINE,
	OVERLAY_TRIANGLE,
	OVERLAY_LASER_LINE,
	OVERLAY_BOX2,
	OVERLAY_CAPSULE,
	OVERLAY_UNK0,
	OVERLAY_UNK1
};

struct OverlayBase_t
{
	OverlayBase_t(void)
	{
		m_Type          = OverlayType_t::OVERLAY_BOX;
		m_nCreationTick = -1;
		m_flEndTime     = 0.0f;
		m_nServerCount  = -1;
		m_pNextOverlay  = NULL;
		m_nOverlayTick  = NULL;
		m_nFlags        = NULL;
	}
	bool IsDead(void) const;

	OverlayType_t   m_Type;          // What type of overlay is it?
	int             m_nCreationTick; // Duration -1 means go away after this frame #
	float           m_flEndTime;     // When does this box go away
	int             m_nServerCount;  // Latch server count, too
	OverlayBase_t*  m_pNextOverlay;  // 16
	int             m_nOverlayTick;  // 24
	int             m_nFlags;        // Maybe
};

struct OverlayBox_t : public OverlayBase_t
{
	OverlayBox_t(void) { m_Type = OverlayType_t::OVERLAY_BOX; }

	struct Transforms
	{
		Transforms()
		{
			xmm[0] = LoadZeroSIMD();
			xmm[1] = LoadZeroSIMD();
			xmm[2] = LoadZeroSIMD();
		};
		union
		{
			fltx4 xmm[3];
			matrix3x4a_t mat;
		};
	};

	Transforms transforms;
	Vector3D mins;
	Vector3D maxs;
	int             r;
	int             g;
	int             b;
	int             a;
	bool            noDepthTest;
};

struct OverlaySphere_t : public OverlayBase_t
{
	OverlaySphere_t(void) { m_Type = OverlayType_t::OVERLAY_SPHERE; }

	Vector3D        vOrigin;
	float           flRadius;
	int             nTheta;
	int             nPhi;
	int             r;
	int             g;
	int             b;
	int             a;
};

struct OverlayLine_t : public OverlayBase_t
{
	OverlayLine_t(void) { m_Type = OverlayType_t::OVERLAY_LINE; }

	Vector3D        origin;
	Vector3D        dest;
	int             r;
	int             g;
	int             b;
	int             a;
	bool            noDepthTest;
};

struct OverlayTriangle_t : public OverlayBase_t
{
	OverlayTriangle_t() { m_Type = OverlayType_t::OVERLAY_TRIANGLE; }

	Vector3D		p1;
	Vector3D		p2;
	Vector3D		p3;
	int				r;
	int				g;
	int				b;
	int				a;
	bool			noDepthTest;
};

struct OverlayLaserLine_t : public OverlayBase_t
{
	OverlayLaserLine_t() { m_Type = OverlayType_t::OVERLAY_LASER_LINE; }

	Vector3D		start;
	Vector3D		end;
	int				r;
	int				g;
	int				b;
	int				a;
	bool			noDepthTest;
};

struct OverlayCapsule_t : public OverlayBase_t
{
	OverlayCapsule_t() { m_Type = OverlayType_t::OVERLAY_CAPSULE; }

	Vector3D start;
	Vector3D end;
	Vector3D radius;
	Vector3D top;
	Vector3D bottom;
	int r;
	int g;
	int b;
	int a;
	bool m_bWireframe;
};

void DestroyOverlay(OverlayBase_t* pOverlay);
void DrawOverlay(OverlayBase_t* pOverlay);

inline CMemory p_DrawAllOverlays;
inline void(*v_DrawAllOverlays)(bool bDraw);

inline CMemory p_DestroyOverlay;
inline void(*v_DestroyOverlay)(OverlayBase_t* pOverlay);

inline CMemory p_RenderLine;
inline void*(*v_RenderLine)(const Vector3D& vOrigin, const Vector3D& vDest, Color color, bool bZBuffer);

inline CMemory p_RenderBox;
inline void*(*v_RenderBox)(const matrix3x4_t& vTransforms, const Vector3D& vMins, const Vector3D& vMaxs, Color color, bool bZBuffer);

inline CMemory p_RenderWireframeSphere;
inline void*(*v_RenderWireframeSphere)(const Vector3D& vCenter, float flRadius, int nTheta, int nPhi, Color color, bool bZBuffer);

inline OverlayBase_t** s_pOverlays = nullptr;
inline LPCRITICAL_SECTION s_OverlayMutex = nullptr;

inline int* g_nRenderTickCount = nullptr;
inline int* g_nOverlayTickCount = nullptr;

///////////////////////////////////////////////////////////////////////////////
class VDebugOverlay : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("DrawAllOverlays", p_DrawAllOverlays.GetPtr());
		LogFunAdr("DestroyOverlay", p_DestroyOverlay.GetPtr());
		LogFunAdr("RenderLine", p_RenderLine.GetPtr());
		LogFunAdr("RenderBox", p_RenderBox.GetPtr());
		LogFunAdr("RenderWireframeSphere", p_RenderWireframeSphere.GetPtr());
		LogVarAdr("s_Overlays", reinterpret_cast<uintptr_t>(s_pOverlays));
		LogVarAdr("s_OverlayMutex", reinterpret_cast<uintptr_t>(s_OverlayMutex));
		LogVarAdr("g_nOverlayTickCount", reinterpret_cast<uintptr_t>(g_nOverlayTickCount));
		LogVarAdr("g_nRenderTickCount", reinterpret_cast<uintptr_t>(g_nRenderTickCount));
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_DrawAllOverlays = g_GameDll.FindPatternSIMD("40 55 48 83 EC 50 48 8B 05 ?? ?? ?? ??");
		p_RenderBox = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 44 89 4C 24 ?? 55 41 56");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_DrawAllOverlays = g_GameDll.FindPatternSIMD("40 55 48 83 EC 30 48 8B 05 ?? ?? ?? ?? 0F B6 E9");
		p_RenderBox = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 44 89 4C 24 ??");
#endif
		p_DestroyOverlay = g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 48 8B D9 48 8D 0D ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 48 63 03");
		p_RenderWireframeSphere = g_GameDll.FindPatternSIMD("40 56 41 54 41 55 48 81 EC ?? ?? ?? ??");
		p_RenderLine     = g_GameDll.FindPatternSIMD("48 89 74 24 ?? 44 89 44 24 ?? 57 41 56");

		v_DrawAllOverlays = p_DrawAllOverlays.RCast<void (*)(bool)>();                                                                /*40 55 48 83 EC 30 48 8B 05 ?? ?? ?? ?? 0F B6 E9*/
		v_DestroyOverlay = p_DestroyOverlay.RCast<void (*)(OverlayBase_t*)>();                                                        /*40 53 48 83 EC 20 48 8B D9 48 8D 0D ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 48 63 03 */
		v_RenderBox = p_RenderBox.RCast<void* (*)(const matrix3x4_t&, const Vector3D&, const Vector3D&, Color, bool)>();              /*48 89 5C 24 ?? 48 89 6C 24 ?? 44 89 4C 24 ??*/
		v_RenderWireframeSphere = p_RenderWireframeSphere.RCast<void* (*)(const Vector3D&, float, int, int, Color, bool)>();          /*40 56 41 54 41 55 48 81 EC ?? ?? ?? ??*/
		v_RenderLine = p_RenderLine.RCast<void* (*)(const Vector3D&, const Vector3D&, Color, bool)>();                                /*48 89 74 24 ?? 44 89 44 24 ?? 57 41 56*/
	}
	virtual void GetVar(void) const
	{
		s_pOverlays = p_DrawAllOverlays.Offset(0x10).FindPatternSelf("48 8B 3D", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7).RCast<OverlayBase_t**>();
		s_OverlayMutex = p_DrawAllOverlays.Offset(0x10).FindPatternSelf("48 8D 0D", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7).RCast<LPCRITICAL_SECTION>();

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		g_nRenderTickCount = p_DrawAllOverlays.Offset(0x80).FindPatternSelf("3B 0D", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x6).RCast<int*>();
		g_nOverlayTickCount = p_DrawAllOverlays.Offset(0x70).FindPatternSelf("3B 0D", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x6).RCast<int*>();
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		g_nRenderTickCount = p_DrawAllOverlays.Offset(0x50).FindPatternSelf("3B 05", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x6).RCast<int*>();
		g_nOverlayTickCount = p_DrawAllOverlays.Offset(0x70).FindPatternSelf("3B 05", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x6).RCast<int*>();
#endif
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
