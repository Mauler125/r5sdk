#ifndef CL_VIEW_H
#define CL_VIEW_H

const Vector3D& MainViewOrigin();
const QAngle& MainViewAngles();

inline Vector3D* g_vecRenderOrigin = nullptr;
inline QAngle* g_vecRenderAngles = nullptr;

///////////////////////////////////////////////////////////////////////////////
class V_View : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| VAR: g_vecRenderOrigin                    : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_vecRenderOrigin));
		spdlog::debug("| VAR: g_vecRenderAngles                    : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_vecRenderAngles));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
		CMemory base = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x30\xF3\x0F\x10\x05\x00\x00\x00\x00\x00\x8B\x00"), "xxxx?xxxxxxxxx?????x?");

		g_vecRenderOrigin = base.Offset(0x00).FindPatternSelf("F3 0F 10 05").ResolveRelativeAddressSelf(0x4, 0x8).RCast<Vector3D*>();
		g_vecRenderAngles = base.Offset(0x30).FindPatternSelf("F3 0F 10 0D").ResolveRelativeAddressSelf(0x4, 0x8).RCast<QAngle*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(V_View);
#endif // CL_VIEW_H