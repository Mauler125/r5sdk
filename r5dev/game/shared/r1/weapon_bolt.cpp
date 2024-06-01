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
		//Error(eDLL_T::SERVER, EXIT_FAILURE, "Unable to create bolt for %s", UTIL_GetEntityScriptInfo(player));

		// Amos: The engine code has been modified on assembly level, function
		// 'FireWeaponBolt' now checks the pointer returned by this function,
		// and returns null if null (script should be able to handle this error
		// or cause a soft crash if the entity handle is being used).

		Assert(0);
	}

	return weaponBolt;
}

//-----------------------------------------------------------------------------
void V_Weapon_Bolt::Detour(const bool bAttach) const
{
	DetourSetup(&v_CreateWeaponBolt, &CreateWeaponBolt, bAttach);
}
