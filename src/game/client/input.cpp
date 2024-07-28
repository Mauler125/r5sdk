//=============================================================================//
//
// Purpose: 
//
//=============================================================================//
#include "input.h"
#include "common/global.h"
#include "game/shared/weapon_types.h"
#include "game/shared/shared_activity.h"

void CInput::VSetCustomWeaponActivity(CInput* pInput, sharedactivity_e weaponActivity)
{
	// Server only allows other custom weapon activities if cheats are enabled,
	// don't bother simulating it on the client without the cheats cvar, as
	// otherwise visual glitches occur.
	if (!sv_cheats->GetBool() && weaponActivity != ACT_VM_WEAPON_INSPECT)
		weaponActivity = ACT_NONE;

	v_CInput__SetCustomWeaponActivity(pInput, weaponActivity);
}

void VInput::Detour(const bool bAttach) const
{
	DetourSetup(&v_CInput__SetCustomWeaponActivity, CInput::VSetCustomWeaponActivity, bAttach);
}
