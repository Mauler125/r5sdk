#ifndef GAME_WEAPON_BOLT_H
#define GAME_WEAPON_BOLT_H

#include <game/shared/shared_classnames.h>
#ifndef CLIENT_DLL
#include <game/server/baseentity.h>
#include <game/server/player.h>
#else
#include <game/client/c_baseentity.h>
#include <game/client/c_player.h>
#endif

CBaseEntity* CreateWeaponBolt(Vector3D* origin, Vector3D* end, __int64 unused, float scale, CPlayer* unkEnt,
	int a6, int modelindex, int a8, unsigned __int8 a9, unsigned int a10, CBaseEntity* weaponEnt);

inline CBaseEntity*(*v_CreateWeaponBolt)(
	Vector3D* origin, Vector3D* end, __int64 unused, float scale, CPlayer* player,
	int a6, int modelindex, int a8, unsigned __int8 a9, unsigned int a10, CBaseEntity* weapon);


///////////////////////////////////////////////////////////////////////////////
class V_Weapon_Bolt : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CreateWeaponBolt", v_CreateWeaponBolt);
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 41 56 41 57 48 83 EC 70").GetPtr(v_CreateWeaponBolt);
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // GAME_WEAPON_BOLT_H
