//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "usercmd.h"

//-----------------------------------------------------------------------------
// Purpose: Read in a delta compressed usercommand.
// Input  : *buf - 
//			*move - 
//			*from - 
// Output : random seed
//-----------------------------------------------------------------------------
int ReadUserCmd(bf_read* buf, CUserCmd* move, CUserCmd* from)
{
	const int seed = v_ReadUserCmd(buf, move, from);

	// On the client, the frame time must be within 'usercmd_frametime_min'
	// and 'usercmd_frametime_max'. Testing revealed that speed hacking could
	// be achieved by sending bogus frametimes. Clamp the networked frame time
	// to the exact values that the client should be using to make sure it
	// couldn't be circumvented by busting out the client side clamps.
	if (host_timescale->GetFloat() == 1.0f)
		move->frametime = clamp(move->frametime,
			usercmd_frametime_min->GetFloat(),
			usercmd_frametime_max->GetFloat());

	return seed;
}

//-----------------------------------------------------------------------------
void VUserCmd::Attach() const
{
	DetourAttach(&v_ReadUserCmd, &ReadUserCmd);
}

void VUserCmd::Detach() const
{
	DetourDetach(&v_ReadUserCmd, &ReadUserCmd);
}
