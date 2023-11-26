#ifndef GAME_WEAPON_BOLT_H
#define GAME_WEAPON_BOLT_H

#include <game/shared/shared_classnames.h>
#ifndef CLIENT_DLL
#include <game/server/baseentity.h>
#include <game/server/player.h>
#else
#include <game/client/c_baseentity.h>
#include <game/client/c_baseplayer.h>
#endif

CBaseEntity* CreateWeaponBolt(Vector3D* origin, Vector3D* end, __int64 unused, float scale, CPlayer* unkEnt,
	int a6, int modelindex, int a8, unsigned __int8 a9, unsigned int a10, CBaseEntity* weaponEnt);

inline CMemory p_CreateWeaponBolt;
inline CBaseEntity*(*v_CreateWeaponBolt)(
	Vector3D* origin, Vector3D* end, __int64 unused, float scale, CPlayer* player,
	int a6, int modelindex, int a8, unsigned __int8 a9, unsigned int a10, CBaseEntity* weapon);


///////////////////////////////////////////////////////////////////////////////
class V_Weapon_Bolt : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CreateWeaponBolt", p_CreateWeaponBolt.GetPtr());
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CreateWeaponBolt = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 56 57 41 54 41 55 41 56 48 81 EC ?? ?? ?? ??");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CreateWeaponBolt = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 41 56 41 57 48 83 EC 70");
#endif
		v_CreateWeaponBolt = p_CreateWeaponBolt.RCast<CBaseEntity* (*)(
			Vector3D*, Vector3D*, __int64, float, CPlayer*,
			int, int, int, unsigned __int8, unsigned int, CBaseEntity*)>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // GAME_WEAPON_BOLT_H
