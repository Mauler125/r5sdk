//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#ifndef PLAYER_COMMAND_H
#define PLAYER_COMMAND_H

#include "edict.h"
#include "game/shared/usercmd.h"
#include "game/server/player.h"

class IMoveHelper;
class CMoveData;
class CBasePlayer;

//-----------------------------------------------------------------------------
// Purpose: Server side player movement
//-----------------------------------------------------------------------------
class CPlayerMove
{
public:
	//DECLARE_CLASS_NOBASE(CPlayerMove);

	// Construction/destruction
	CPlayerMove(void);
	virtual			~CPlayerMove(void) {}

	// Hook statics:
	static void StaticRunCommand(CPlayerMove* thisp, CPlayer* player, CUserCmd* ucmd, IMoveHelper* moveHelper);

	// Public interfaces:
	// Run a movement command from the player
	virtual void	RunCommand(CPlayer* player, CUserCmd* ucmd, IMoveHelper* moveHelper) = 0;

protected:
	// Prepare for running movement
	virtual void	SetupMove(CPlayer* player, CUserCmd* ucmd, CMoveData* move) = 0;

	// Finish movement
	virtual void	FinishMove(CPlayer* player, CUserCmd* ucmd, CMoveData* move) = 0;

	// Called before and after any movement processing
	virtual void	StartCommand(CPlayer* player, IMoveHelper* pHelper, CUserCmd* cmd) = 0;
};

inline void (*CPlayerMove__RunCommand)(CPlayerMove* thisp, CPlayer* player, CUserCmd* ucmd, IMoveHelper* moveHelper);

///////////////////////////////////////////////////////////////////////////////
class VPlayerMove : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CPlayerMove::RunCommand", CPlayerMove__RunCommand);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 8B C4 55 53 56 57 41 57 48 8D A8 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 44 0F 29 50 ??").GetPtr(CPlayerMove__RunCommand);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // PLAYER_COMMAND_H
