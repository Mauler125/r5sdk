#ifndef CLIENT_INPUT_H
#define CLIENT_INPUT_H
#include "game/client/iinput.h"

class CInput : public IInput
{
public:
	static void SetCustomWeaponActivity(CInput* pInput, int weaponActivity);
private:
};

inline CMemory p_CInput__SetCustomWeaponActivity;
inline void(*v_CInput__SetCustomWeaponActivity)(CInput* pInput, int weaponActivity);

inline IInput* g_pInput_VFTable = nullptr;
inline CInput* g_pInput = nullptr;

///////////////////////////////////////////////////////////////////////////////
class VInput : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogConAdr("CInput::`vftable'", reinterpret_cast<uintptr_t>(g_pInput_VFTable));
		LogFunAdr("CInput::SetCustomWeaponActivity", p_CInput__SetCustomWeaponActivity.GetPtr());
		LogVarAdr("g_Input", reinterpret_cast<uintptr_t>(g_pInput));
	}
	virtual void GetFun(void) const
	{
		p_CInput__SetCustomWeaponActivity = g_GameDll.
			FindPatternSIMD("89 91 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC CC F3 0F 11 89 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC F3 0F 10 81 ?? ?? ?? ??");
		v_CInput__SetCustomWeaponActivity = p_CInput__SetCustomWeaponActivity.RCast<void (*)(CInput*, int)>();
	}
	virtual void GetVar(void) const
	{
		g_pInput = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 48 8B 5D 57").FollowNearCallSelf().
			FindPatternSelf("48 8B 05").ResolveRelativeAddressSelf(0x3, 0x7).RCast<CInput*>();
	}
	virtual void GetCon(void) const
	{
		g_pInput_VFTable = g_GameDll.GetVirtualMethodTable(".?AVCInput@@").RCast<IInput*>();
	}
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // CLIENT_INPUT_H
