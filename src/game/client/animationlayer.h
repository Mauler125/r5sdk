//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//
#ifndef ANIMATIONLAYER_H
#define ANIMATIONLAYER_H

#include "studio.h"

struct C_AnimationLayer
{
	void* _vftable;
	CStudioHdr* m_pDispatchedStudioHdr;
	int m_nDispatchedSrc;
	int m_nDispatchedDst;
};

#endif // ANIMATIONLAYER_H
