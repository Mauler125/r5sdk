#pragma once
#include "mathlib/vector.h"
#include "mathlib/color.h"
#include "mathlib/vector4d.h"

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
	OVERLAY_SWEPT_BOX,
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
	}
	bool IsDead(void) const;

	OverlayType_t   m_Type         {}; // What type of overlay is it?
	int             m_nCreationTick{}; // Duration -1 means go away after this frame #
	float           m_flEndTime    {}; // When does this box go away
	int             m_nServerCount {}; // Latch server count, too
	OverlayBase_t*  m_pNextOverlay {}; // 16
	int             m_nOverlayTick {}; // 24
	int             m_nFlags{}; // Maybe
};

struct OverlayLine_t : public OverlayBase_t
{
	OverlayLine_t(void) { m_Type = OverlayType_t::OVERLAY_LINE; }

	Vector3D        origin{};
	Vector3D        dest{};
	int             r{};
	int             g{};
	int             b{};
	int             a{};
	bool            noDepthTest{};
};

struct OverlayBox_t : public OverlayBase_t
{
	OverlayBox_t(void) { m_Type = OverlayType_t::OVERLAY_BOX; }

	struct Transforms
	{
		Transforms()
		{
			xmm[0] = _mm_setzero_ps();
			xmm[1] = _mm_setzero_ps();
			xmm[2] = _mm_setzero_ps();
		};
		union
		{
			__m128 xmm[3];
			Vector4D vec[3];
		};
	};

	Transforms transforms;
	Vector3D mins{};
	Vector3D maxs{};
	int             r{};
	int             g{};
	int             b{};
	int             a{};
	bool            noDepthTest{};
};

struct OverlaySphere_t : public OverlayBase_t
{
	OverlaySphere_t(void) { m_Type = OverlayType_t::OVERLAY_SPHERE; }

	Vector3D        vOrigin{};
	float           flRadius{};
	int             nTheta{};
	int             nPhi{};
	int             r{};
	int             g{};
	int             b{};
	int             a{};
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

struct OverlaySweptBox_t : public OverlayBase_t
{
	OverlaySweptBox_t() { m_Type = OverlayType_t::OVERLAY_SWEPT_BOX; }

	Vector3D		start;
	Vector3D		end;
	Vector3D		mins;
	Vector3D		maxs;
	QAngle			angles;
	int				r;
	int				g;
	int				b;
	int				a;
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
void DebugOverlays_Attach();
void DebugOverlays_Detach();

inline CMemory p_DrawAllOverlays;
inline auto v_DrawAllOverlays = p_DrawAllOverlays.RCast<void (*)(bool bDraw)>();

inline CMemory p_RenderLine;
inline auto v_RenderLine = p_RenderLine.RCast<void* (*)(const Vector3D& vOrigin, const Vector3D& vDest, Color color, bool bZBuffer)>();

inline CMemory p_RenderBox;
inline auto v_RenderBox = p_RenderBox.RCast<void* (*)(const OverlayBox_t::Transforms& vTransforms, const Vector3D& vMins, const Vector3D& vMaxs, Color color, bool bZBuffer)>();

inline CMemory p_RenderWireframeSphere;
inline auto v_RenderWireframeSphere = p_RenderWireframeSphere.RCast<void* (*)(const Vector3D& vCenter, float flRadius, int nTheta, int nPhi, Color color, bool bZBuffer)>();

inline CMemory p_DestroyOverlay;
inline auto v_DestroyOverlay = p_DestroyOverlay.RCast<void (*)(OverlayBase_t* pOverlay)>();

inline int* client_debugdraw_int_unk = nullptr;
inline float* client_debugdraw_float_unk = nullptr;

inline OverlayBase_t** s_pOverlays = nullptr;
inline LPCRITICAL_SECTION s_OverlayMutex = nullptr;

inline int* render_tickcount = nullptr;
inline int* overlay_tickcount = nullptr;

///////////////////////////////////////////////////////////////////////////////
class VDebugOverlay : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: DrawAllOverlays                      : {:#18x} |\n", p_DrawAllOverlays.GetPtr());
		spdlog::debug("| FUN: RenderLine                           : {:#18x} |\n", p_RenderLine.GetPtr());
		spdlog::debug("| FUN: RenderBox                            : {:#18x} |\n", p_RenderBox.GetPtr());
		spdlog::debug("| FUN: RenderWireframeSphere                : {:#18x} |\n", p_RenderWireframeSphere.GetPtr());
		spdlog::debug("| FUN: DestroyOverlay                       : {:#18x} |\n", p_DestroyOverlay.GetPtr());
		spdlog::debug("| VAR: s_pOverlays                          : {:#18x} |\n", reinterpret_cast<uintptr_t>(s_pOverlays));
		spdlog::debug("| VAR: s_OverlayMutex                       : {:#18x} |\n", reinterpret_cast<uintptr_t>(s_OverlayMutex));
		spdlog::debug("| VAR: client_debugdraw_int_unk             : {:#18x} |\n", reinterpret_cast<uintptr_t>(client_debugdraw_int_unk));
		spdlog::debug("| VAR: client_debugdraw_float_unk           : {:#18x} |\n", reinterpret_cast<uintptr_t>(client_debugdraw_float_unk));
		spdlog::debug("| VAR: overlay_tickcount                    : {:#18x} |\n", reinterpret_cast<uintptr_t>(overlay_tickcount));
		spdlog::debug("| VAR: render_tickcount                     : {:#18x} |\n", reinterpret_cast<uintptr_t>(render_tickcount));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_DrawAllOverlays = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x55\x48\x83\xEC\x50\x48\x8B\x05\x00\x00\x00\x00"), "xxxxxxxxx????");
		p_RenderWireframeBox = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x44\x89\x4C\x24\x00"), "xxxx?xxxx?xxxx?");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_DrawAllOverlays = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x55\x48\x83\xEC\x30\x48\x8B\x05\x00\x00\x00\x00\x0F\xB6\xE9"), "xxxxxxxxx????xxx");
		p_RenderBox = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x44\x89\x4C\x24\x00"), "xxxx?xxxx?xxxx?");
#endif
		p_DestroyOverlay = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\x48\x8B\xD9\x48\x8D\x0D\x00\x00\x00\x00\xFF\x15\x00\x00\x00\x00\x48\x63\x03"), "xxxxxxxxxxxx????xx????xxx");
		p_RenderWireframeSphere = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x56\x41\x54\x41\x55\x48\x81\xEC\x00\x00\x00\x00"), "xxxxxxxxx????");
		p_RenderLine     = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x74\x24\x00\x44\x89\x44\x24\x00\x57\x41\x56"), "xxxx?xxxx?xxx");

		v_DrawAllOverlays = p_DrawAllOverlays.RCast<void (*)(bool)>();                                                                /*40 55 48 83 EC 30 48 8B 05 ?? ?? ?? ?? 0F B6 E9*/
		v_DestroyOverlay = p_DestroyOverlay.RCast<void (*)(OverlayBase_t*)>();                                                        /*40 53 48 83 EC 20 48 8B D9 48 8D 0D ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 48 63 03 */
		v_RenderBox = p_RenderBox.RCast<void* (*)(const OverlayBox_t::Transforms&, const Vector3D&, const Vector3D&, Color, bool)>(); /*48 89 5C 24 ?? 48 89 6C 24 ?? 44 89 4C 24 ??*/
		v_RenderWireframeSphere = p_RenderWireframeSphere.RCast<void* (*)(const Vector3D&, float, int, int, Color, bool)>();          /*40 56 41 54 41 55 48 81 EC ?? ?? ?? ??*/
		v_RenderLine = p_RenderLine.RCast<void* (*)(const Vector3D&, const Vector3D&, Color, bool)>();                                /*48 89 74 24 ?? 44 89 44 24 ?? 57 41 56*/
	}
	virtual void GetVar(void) const
	{
		client_debugdraw_int_unk = p_DrawAllOverlays.Offset(0xC0).FindPatternSelf("F3 0F 59", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x4, 0x8).RCast<int*>();
		client_debugdraw_float_unk = p_DrawAllOverlays.Offset(0xD0).FindPatternSelf("F3 0F 10", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x4, 0x8).RCast<float*>();

		s_pOverlays = p_DrawAllOverlays.Offset(0x10).FindPatternSelf("48 8B 3D", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7).RCast<OverlayBase_t**>();
		s_OverlayMutex = p_DrawAllOverlays.Offset(0x10).FindPatternSelf("48 8D 0D", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7).RCast<LPCRITICAL_SECTION>();

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		render_tickcount = p_DrawAllOverlays.Offset(0x80).FindPatternSelf("3B 0D", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x6).RCast<int*>();
		overlay_tickcount = p_DrawAllOverlays.Offset(0x70).FindPatternSelf("3B 0D", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x6).RCast<int*>();
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		render_tickcount = p_DrawAllOverlays.Offset(0x50).FindPatternSelf("3B 05", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x6).RCast<int*>();
		overlay_tickcount = p_DrawAllOverlays.Offset(0x70).FindPatternSelf("3B 05", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x6).RCast<int*>();
#endif
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VDebugOverlay);
