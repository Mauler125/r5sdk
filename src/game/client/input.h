#ifndef CLIENT_INPUT_H
#define CLIENT_INPUT_H
#include "game/shared/weapon_types.h"
#include "game/shared/shared_activity.h"
#include "game/client/iinput.h"

class CInput : public IInput
{
public:
	virtual void sub_140701C40() = 0;
	virtual void sub_140701D60() = 0;
	virtual void sub_140701D90() = 0;

	virtual int GetMaxAbilityBindingCount( void ) = 0;
	virtual int GetMaxAbilityBindingLength( void ) = 0;

	virtual void sub_140701DC0() = 0;

	virtual void SetJoystickDebounce( int nSlot = -1 ) = 0;
	virtual void ClearJoystickDebounce( int nSlot = -1 ) = 0;

	virtual float		GetLastSwapSelectTime() = 0;
	virtual void		SetLastSwapSelectTime( float lastSelectTime ) = 0;
	virtual float		Unknown_GetFloat20() = 0;
	virtual void		Unknown_SetFloat20( float a2 ) = 0;
	virtual float		Unknown_GetFloat24() = 0;
	virtual void		Unknown_SetFloat24( float a2 ) = 0;

	virtual float		GetLastButtonPressTime( void ) = 0;
	virtual void		SetLastButtonPressTime( float lastPressTime ) = 0;
	virtual void		LinkButtonPair( int button1, int button2, int output ) = 0;

	virtual void sub_140701720() = 0;

	virtual void SetCustomWeaponActivity( sharedactivity_e weaponActivity ) = 0;
	virtual void ActivateOffhandWeaponByIndex( char index /*TODO[ AMOS ]: needs enum, enumVal 6 = INVALID; see Script_ActivateOffhandWeaponByIndex*/ ) = 0;

	virtual void ChangeControllerMode( bool newMode ) = 0;

	virtual void SetZoomScale( float zoomScale ) = 0;      // Used for SetBigMapZoomScale script func
	virtual float GetZoomScale() = 0;                      // Used for GetBigMapZoomScale script func
	virtual void SetZoomAnchor( Vector2D& inAnchor ) = 0;  // Used for SetBigMapZoomAnchor script func
	virtual void GetZoomAnchor( Vector2D& outAnchor ) = 0; // Used for SetBigMapZoomAnchor script func

public: // Hook statics
	static void VSetCustomWeaponActivity( CInput* pInput, sharedactivity_e weaponActivity );
private:
};

inline void(*v_CInput__SetCustomWeaponActivity)(CInput* pInput, sharedactivity_e weaponActivity);

inline IInput* g_pInput_VFTable = nullptr;
inline CInput* g_pInput = nullptr;

///////////////////////////////////////////////////////////////////////////////
class VInput : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogConAdr("CInput::`vftable'", g_pInput_VFTable);
		LogFunAdr("CInput::SetCustomWeaponActivity", v_CInput__SetCustomWeaponActivity);
		LogVarAdr("g_Input", g_pInput);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("89 91 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC CC F3 0F 11 89 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC F3 0F 10 81 ?? ?? ?? ??")
			.GetPtr(v_CInput__SetCustomWeaponActivity);
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
