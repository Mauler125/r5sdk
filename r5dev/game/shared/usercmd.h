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

inline CMemory p_CUserCmd__Copy;
inline auto v_CUserCmd__Copy = p_CUserCmd__Copy.RCast<CUserCmd*(*)(CUserCmd* pDest, CUserCmd* pSource)>();

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

	CUserCmd* Copy(CUserCmd* pSource)
	{
		return v_CUserCmd__Copy(this, pSource);
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
	char pad_0x01A0[60];
};


///////////////////////////////////////////////////////////////////////////////
class VUserCmd : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CUserCmd::Reset", p_CUserCmd__Reset.GetPtr());
		LogFunAdr("CUserCmd::Copy", p_CUserCmd__Copy.GetPtr());
	}
	virtual void GetFun(void) const
	{
		p_CUserCmd__Reset = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 83 FD FF 74 0A").FollowNearCallSelf();
		v_CUserCmd__Reset = p_CUserCmd__Reset.RCast<void(*)(CUserCmd*)>();

		p_CUserCmd__Copy = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 4C 8B 9B ?? ?? ?? ??").FollowNearCallSelf();
		v_CUserCmd__Copy = p_CUserCmd__Copy.RCast<CUserCmd* (*)(CUserCmd*, CUserCmd*)>();
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

#endif // USERCMD_H
