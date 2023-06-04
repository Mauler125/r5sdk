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

inline CMemory p_CUserCmd__Reset;
inline auto v_CUserCmd__Reset = p_CUserCmd__Reset.RCast<void(*)(CUserCmd* pUserCmd)>();

inline CMemory p_CUserCmd__Copy;
inline auto v_CUserCmd__Copy = p_CUserCmd__Copy.RCast<CUserCmd*(*)(CUserCmd* pDest, CUserCmd* pSource)>();

inline CMemory p_ReadUserCmd;
inline auto v_ReadUserCmd = p_ReadUserCmd.RCast<int (*)(bf_read* buf, CUserCmd* move, CUserCmd* from)>();

//-------------------------------------------------------------------------------------
// 
//-------------------------------------------------------------------------------------
class CUserCmd
{
public:
	CUserCmd() // Cannot be constructed during DLL init.
	{
		Reset();
	}

	inline CUserCmd* Copy(CUserCmd* pSource) { return v_CUserCmd__Copy(this, pSource); }
	inline void Reset() { v_CUserCmd__Reset(this); }

	int32_t command_number;
	int32_t tick_count;
	float curtime;
	QAngle viewangles;
	float maybe;
	char pad_0x0018[8];
	float forwardmove;
	float sidemove;
	float upmove;
	int32_t buttons;
	char pad_0x0034[336];
	int32_t randomseed;
	char pad_0x0188[8];
	Vector3D headposition;
	float maxpitch;
	char pad_0x01A0[56];
	float frametime;
};

static_assert(sizeof(CUserCmd) == 0x1DC);

///////////////////////////////////////////////////////////////////////////////
class VUserCmd : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CUserCmd::Reset", p_CUserCmd__Reset.GetPtr());
		LogFunAdr("CUserCmd::Copy", p_CUserCmd__Copy.GetPtr());
		LogFunAdr("ReadUserCmd", p_ReadUserCmd.GetPtr());
	}
	virtual void GetFun(void) const
	{
		p_CUserCmd__Reset = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 48 8B DF 66 83 FE FF").FollowNearCallSelf();
		v_CUserCmd__Reset = p_CUserCmd__Reset.RCast<void(*)(CUserCmd*)>();

		p_CUserCmd__Copy = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 4C 8B 9B ?? ?? ?? ??").FollowNearCallSelf();
		v_CUserCmd__Copy = p_CUserCmd__Copy.RCast<CUserCmd* (*)(CUserCmd*, CUserCmd*)>();

		p_ReadUserCmd = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 4C 8B C6 48 81 C6 ?? ?? ?? ??").FollowNearCallSelf();
		v_ReadUserCmd = p_ReadUserCmd.RCast<int (*)(bf_read*, CUserCmd*, CUserCmd*)>();
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const;
	virtual void Detach(void) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // USERCMD_H
