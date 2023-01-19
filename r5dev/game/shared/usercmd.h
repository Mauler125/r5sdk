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

#include "public/utility/memaddr.h"
#include "mathlib/vector.h"

//-------------------------------------------------------------------------------------
// Forward declarations
//-------------------------------------------------------------------------------------
class CUserCmd;

inline CMemory p_CUserCmd__Reset;
inline auto v_CUserCmd__Reset = p_CUserCmd__Reset.RCast<void(*)(CUserCmd* pUserCmd)>();

//-------------------------------------------------------------------------------------
// 
//-------------------------------------------------------------------------------------
class CUserCmd
{
public:
	CUserCmd() // Cannot be constructed during DLL init.
	{
		v_CUserCmd__Reset(this);
	}

	int32_t command_number;
	int32_t tick_count;
	float curtime;
	QAngle viewangles;
	char pad_0x0018[12];
	float forwardmove;
	float sidemove;
	float upmove;
	int32_t buttons;
	char pad_0x0034[336];
	int32_t randomseed;
	char pad_0x0188[8];
	Vector3D headposition;
	float maxpitch;
	char pad_0x01A0[224];
};


///////////////////////////////////////////////////////////////////////////////
class VUserCmd : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: CUserCmd::Reset                      : {:#18x} |\n", p_CUserCmd__Reset.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_CUserCmd__Reset = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 83 FD FF 74 0A").FollowNearCallSelf();
		v_CUserCmd__Reset = p_CUserCmd__Reset.RCast<void(*)(CUserCmd*)>();
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VUserCmd);

#endif // USERCMD_H
