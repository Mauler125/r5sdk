#include "weapon_bolt.h"
#include "game/shared/util_shared.h"

//-----------------------------------------------------------------------------
// Purpose: creates a weapon bolt
//-----------------------------------------------------------------------------
CBaseEntity* CreateWeaponBolt(Vector3D* origin, Vector3D* end, __int64 unused, float scale, CPlayer* player,
	int a6, int modelIndex, int a8, unsigned __int8 a9, unsigned int a10, CBaseEntity* weapon)
{
	CBaseEntity* weaponBolt = v_CreateWeaponBolt(origin, end, unused, scale, player, a6, modelIndex, a8, a9, a10, weapon);
	if (!weaponBolt)
	{
		// Code does NOT check for null, and performing inline assembly is kind of a waste.
		// This only happens when 'EntityFactoryDictionary' fails, which only happens when
		// there are no edict slots available anymore (usually a bug in scripts).
		Error(eDLL_T::SERVER, EXIT_FAILURE, "Unable to create bolt for %s", UTIL_GetEntityScriptInfo(player));
	}

	return weaponBolt;
}

//-----------------------------------------------------------------------------
void V_Weapon_Bolt::Attach() const
{
	DetourAttach(&v_CreateWeaponBolt, &CreateWeaponBolt);
}

void V_Weapon_Bolt::Detach() const
{
	DetourDetach(&v_CreateWeaponBolt, &CreateWeaponBolt);
}
