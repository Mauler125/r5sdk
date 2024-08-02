#ifndef WEAPON_TYPES_H
#define WEAPON_TYPES_H

//-----------------------------------------------------------------------------
// Weapon inventory slot enumerants; used as constants for scripts.
//-----------------------------------------------------------------------------
enum WeaponInventorySlot_t
{
	WEAPON_INVENTORY_SLOT_INVALID = 0xFF, // Promote to 32bit INT_MAX for script constants.
	WEAPON_INVENTORY_SLOT_ANY = 0xFE,     // promote to 32bit INT_MAX for script constants.
	WEAPON_INVENTORY_SLOT_PRIMARY_0 = 0,
	WEAPON_INVENTORY_SLOT_PRIMARY_1 = 1,
	WEAPON_INVENTORY_SLOT_PRIMARY_2 = 2,
	WEAPON_INVENTORY_SLOT_PRIMARY_3 = 3,
	WEAPON_INVENTORY_SLOT_ANTI_TITAN = 4,
	WEAPON_INVENTORY_SLOT_DUALPRIMARY_0 = 5,
	WEAPON_INVENTORY_SLOT_DUALPRIMARY_1 = 6,
	WEAPON_INVENTORY_SLOT_DUALPRIMARY_2 = 7,
	WEAPON_INVENTORY_SLOT_DUALPRIMARY_3 = 8,
};

#endif // WEAPON_TYPES_H
