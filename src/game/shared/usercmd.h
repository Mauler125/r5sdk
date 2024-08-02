//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
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

#include "pingcmd.h"

//-------------------------------------------------------------------------------------
// Console variables
//-------------------------------------------------------------------------------------
extern ConVar usercmd_frametime_max;
extern ConVar usercmd_frametime_min;

extern ConVar usercmd_dualwield_enable;

//-------------------------------------------------------------------------------------
// Forward declarations
//-------------------------------------------------------------------------------------
class CUserCmd;
class CUserCmdExtended;

inline CUserCmd*(*CUserCmd__CUserCmd)(CUserCmd* pUserCmd);
inline void(*CUserCmd__Reset)(CUserCmd* pUserCmd);
inline CUserCmd*(*CUserCmd__Copy)(CUserCmd* pDest, CUserCmd* pSource);
inline int(*v_ReadUserCmd)(bf_read* buf, CUserCmd* move, CUserCmd* from);
inline int(*v_ReadUserCmdExtended)(bf_read* buf, CUserCmdExtended* move, CUserCmdExtended* from);

//-------------------------------------------------------------------------------------
#pragma pack(push, 1)
//-------------------------------------------------------------------------------------
class CUserCmd
{
public:
	CUserCmd() // Cannot be constructed during DLL init.
	{
		CUserCmd__CUserCmd(this);
	}

	inline CUserCmd* Copy(CUserCmd* pSource) { return CUserCmd__Copy(this, pSource); }
	inline void Reset() { CUserCmd__Reset(this); }

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
	__unaligned __declspec(align(1)) __int16 weaponselect;

	bool bUnk39;
	short weaponactivity;
	int nUnk3C;
	bool controllermode;
	bool fixangles;
	bool setlastcycleslot;
	char pad_0x0034[5];
	char unkData[144];

	// Zipline vars (see [r5apex_ds+8A6573] for read).
	bool placedZiplineStation;
	char unk[3];
	float fUnkDC;
	Vector3D beginStationOrigin;
	Vector3D stationWorldRelative;
	float fUnkF8;
	Vector3D endStationOrigin;
	QAngle stationWorldAngles;

	PingCommand_s m_pingCommands[NUM_PING_COMMANDS];
	int32_t randomseed;
	byte bUnk188;
	bool bUnk189;
	bool normalizepitch;
	bool linkedButtonPairPress;
	_BYTE gap18C;
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

// The client-side input version of the UserCmd class
class CUserCmdExtended : public CUserCmd
{
	// todo: reverse engineer.
	char unknown_extended[164];
};

int ReadUserCmd(bf_read* buf, CUserCmd* move, CUserCmd* from);
int ReadUserCmdExtended(bf_read* buf, CUserCmdExtended* move, CUserCmdExtended* from);

///////////////////////////////////////////////////////////////////////////////
class VUserCmd : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CUserCmd::CUserCmd", CUserCmd__CUserCmd);
		LogFunAdr("CUserCmd::Reset", CUserCmd__Reset);
		LogFunAdr("CUserCmd::Copy", CUserCmd__Copy);
		LogFunAdr("ReadUserCmd", v_ReadUserCmd);
		LogFunAdr("ReadUserCmdExtended", v_ReadUserCmdExtended);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 48 81 C3 ?? ?? ?? ?? 48 83 EF 01 75 EB 33 C0").FollowNearCallSelf().GetPtr(CUserCmd__CUserCmd);
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 48 8B DF 66 83 FE FF").FollowNearCallSelf().GetPtr(CUserCmd__Reset);
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 4C 8B 9B ?? ?? ?? ??").FollowNearCallSelf().GetPtr(CUserCmd__Copy);
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 4C 8B C6 48 81 C6 ?? ?? ?? ??").FollowNearCallSelf().GetPtr(v_ReadUserCmd);
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 8B 4B ?? 4C 8D 35 ?? ?? ?? ?? 8B 53").FollowNearCallSelf().GetPtr(v_ReadUserCmdExtended);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // USERCMD_H
