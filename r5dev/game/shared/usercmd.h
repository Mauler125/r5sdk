//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#if !defined( USERCMD_H )
#define USERCMD_H
#ifdef _WIN32
#pragma once
#endif
#include "tier1/bitbuf.h"
#include "mathlib/vector.h"

//-------------------------------------------------------------------------------------
// Forward declarations
//-------------------------------------------------------------------------------------
class CUserCmd;

inline CMemory p_CUserCmd__CUserCmd;
inline CUserCmd*(*v_CUserCmd__CUserCmd)(CUserCmd* pUserCmd);

inline CMemory p_CUserCmd__Reset;
inline void(*v_CUserCmd__Reset)(CUserCmd* pUserCmd);

inline CMemory p_CUserCmd__Copy;
inline CUserCmd*(*v_CUserCmd__Copy)(CUserCmd* pDest, CUserCmd* pSource);

inline CMemory p_ReadUserCmd;
inline int(*v_ReadUserCmd)(bf_read* buf, CUserCmd* move, CUserCmd* from);

//-------------------------------------------------------------------------------------
#pragma pack(push, 1)
//-------------------------------------------------------------------------------------
class CUserCmd
{
public:
	CUserCmd() // Cannot be constructed during DLL init.
	{
		v_CUserCmd__CUserCmd(this);
	}

	inline CUserCmd* Copy(CUserCmd* pSource) { return v_CUserCmd__Copy(this, pSource); }
	inline void Reset() { v_CUserCmd__Reset(this); }

	int32_t command_number;
	int32_t tick_count;
	float_t command_time;
	QAngle viewangles;
	QAngle pitchangles; // Pitch angles? See [r5apex_ds+705D80].
	float_t forwardmove;
	float_t sidemove;
	float_t upmove;
	int32_t buttons;
	byte impulse;
	byte cycleslot;
	byte weaponindex;
	short weaponselect;
	bool bUnk39;
	short weaponactivity;
	int nUnk3C;
	bool controllermode;
	bool fixangles;
	bool setlastcycleslot;
	char pad_0x0034[157];
	QAngle renderangles;                  // IDK what this is used for.
	QAngle renderangles_copy;             // IDK what this is used for.
	float fUnkF8;
	QAngle another_renderangles_copy;     // IDK what this is used for.
	QAngle yet_another_renderangles_copy; // IDK what this is used for.
	char pad_0x00114[112];
	int32_t randomseed;
	byte bUnk188;
	bool bUnk189;
	bool normalizepitch;
	__int16 nUnk18B;
	char pad_0x0188[3];
	Vector3D headposition;
	float_t maxpitch;
	char pad_0x01A0[24];
	bool trace_camera;
	char pad_1B9[3];
	Vector3D camerapos;
	int world_angle_tick;
	int nUnk1D4;
	int predicted_server_event_ack;
	int nUnk1D8;
	float frametime;
};
#pragma pack(pop)

static_assert(sizeof(CUserCmd) == 0x1DC);

int ReadUserCmd(bf_read* buf, CUserCmd* move, CUserCmd* from);

///////////////////////////////////////////////////////////////////////////////
class VUserCmd : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CUserCmd::CUserCmd", p_CUserCmd__CUserCmd.GetPtr());
		LogFunAdr("CUserCmd::Reset", p_CUserCmd__Reset.GetPtr());
		LogFunAdr("CUserCmd::Copy", p_CUserCmd__Copy.GetPtr());
		LogFunAdr("ReadUserCmd", p_ReadUserCmd.GetPtr());
	}
	virtual void GetFun(void) const
	{
		p_CUserCmd__CUserCmd = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 48 81 C3 ?? ?? ?? ?? 48 83 EF 01 75 EB 33 C0").FollowNearCallSelf();
		v_CUserCmd__CUserCmd = p_CUserCmd__CUserCmd.RCast<CUserCmd* (*)(CUserCmd*)>();

		p_CUserCmd__Reset = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 48 8B DF 66 83 FE FF").FollowNearCallSelf();
		v_CUserCmd__Reset = p_CUserCmd__Reset.RCast<void(*)(CUserCmd*)>();

		p_CUserCmd__Copy = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 4C 8B 9B ?? ?? ?? ??").FollowNearCallSelf();
		v_CUserCmd__Copy = p_CUserCmd__Copy.RCast<CUserCmd* (*)(CUserCmd*, CUserCmd*)>();

		p_ReadUserCmd = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 4C 8B C6 48 81 C6 ?? ?? ?? ??").FollowNearCallSelf();
		v_ReadUserCmd = p_ReadUserCmd.RCast<int (*)(bf_read*, CUserCmd*, CUserCmd*)>();
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // USERCMD_H
