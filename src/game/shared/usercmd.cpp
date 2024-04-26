//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "usercmd.h"
#include "tier1/utlbuffer.h"
#include "game/shared/in_buttons.h"
#include "game/shared/weapon_types.h"

ConVar usercmd_frametime_max("usercmd_frametime_max", "0.100"   , FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "The largest amount of simulation seconds a UserCmd can have." );
ConVar usercmd_frametime_min("usercmd_frametime_min", "0.002857", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "The smallest amount of simulation seconds a UserCmd can have.");

ConVar usercmd_dualwield_enable("usercmd_dualwield_enable", "0", FCVAR_REPLICATED | FCVAR_RELEASE, "Allows setting dual wield cycle slots, and activating multiple inventory weapons from UserCmd.");

//-----------------------------------------------------------------------------
// Purpose: Read in a delta compressed usercommand.
// Input  : *buf - 
//			*move - 
//			*from - 
// Output : random seed
//-----------------------------------------------------------------------------
int ReadUserCmd(bf_read* buf, CUserCmd* move, CUserCmd* from)
{
	const int seed = v_ReadUserCmd(buf, move, from);

	// Initialize the camera position as <0,0,0>, this should at least avoid
	// crash and meme behaviors.
	if (!move->camerapos.IsValid())
		move->camerapos.Init();

	// Viewangles must be normalized; applying invalid angles on the client
	// will result in undefined behavior, or a crash.
	move->viewangles.Normalize();
	move->pitchangles.Normalize();

	// Some players abused a feature of the engine which allows you to perform
	// custom weapon activities. After some research, it appears that only the
	// 'ACT_VM_WEAPON_INSPECT' was supposed to work for standard games, all the
	// other activities appears to be for development or testing, therefore, we
	// should only allow 'ACT_VM_WEAPON_INSPECT' if cheats are disabled.
	if (!sv_cheats->GetBool() && move->weaponactivity != ACT_VM_WEAPON_INSPECT)
		move->weaponactivity = ACT_NONE;

	// On the client, the frame time must be within 'usercmd_frametime_min'
	// and 'usercmd_frametime_max'. Testing revealed that speed hacking could
	// be achieved by sending bogus frame times. Clamp the networked frame
	// time to the exact values that the client should be using to make sure
	// it couldn't be circumvented by busting out the client side clamps.
	if (host_timescale->GetFloat() == 1.0f)
		move->frametime = clamp(move->frametime,
			usercmd_frametime_min.GetFloat(),
			usercmd_frametime_max.GetFloat());

	// Checks are only required if cycleslot is valid; see 'CPlayer::UpdateWeaponSlots'.
	if (move->cycleslot != WEAPON_INVENTORY_SLOT_INVALID)
	{
		const bool dualWieldEnabled = usercmd_dualwield_enable.GetBool();

		// Client could instruct the server to switch cycle slots for inventory
		// weapons, however, the client could also cycle to the dual wield slots.
		// dual wield weapons should be explicitly enabled to prevent exploitation.
		if (!dualWieldEnabled && move->cycleslot > WEAPON_INVENTORY_SLOT_ANTI_TITAN)
			move->cycleslot = WEAPON_INVENTORY_SLOT_ANTI_TITAN;

		// Array member 'CPlayer::m_selectedWeapons' is of size 2, values
		// (up to 254) result in arbitrary memory reads on the server.
		// Clamp the value to make sure it never reads out of bounds. Also,
		// if dual wield is disabled on the server, reset the weapon index
		// value as it would otherwise allow the client to enable several
		// equipped weapons at the same time.
		if (move->weaponindex >= WEAPON_INVENTORY_SLOT_PRIMARY_1)
			dualWieldEnabled
			? move->weaponindex = WEAPON_INVENTORY_SLOT_PRIMARY_1
			: move->weaponindex = WEAPON_INVENTORY_SLOT_PRIMARY_0;
	}

	return seed;
}

//-----------------------------------------------------------------------------
void VUserCmd::Detour(const bool bAttach) const
{
	DetourSetup(&v_ReadUserCmd, &ReadUserCmd, bAttach);
}
