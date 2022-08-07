//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Random number generator
//
// $Workfile: $
// $NoKeywords: $
//===========================================================================//

#include "core/stdafx.h"
#include "threadtools.h"

LONG ThreadInterlockedCompareExchange64(LONG volatile* pDest, int64 value, int64 comperand)
{
	return _InterlockedCompareExchange(pDest, comperand, value);
}

bool ThreadInterlockedAssignIf(LONG volatile* p, int32 value, int32 comperand)
{
	Assert((size_t)p % 4 == 0);
	return _InterlockedCompareExchange(p, comperand, value);
}

int64 ThreadInterlockedCompareExchange64(int64 volatile* pDest, int64 value, int64 comperand)
{
	return _InterlockedCompareExchange64(pDest, comperand, value);
}

bool ThreadInterlockedAssignIf64(int64 volatile* pDest, int64 value, int64 comperand)
{
	return _InterlockedCompareExchange64(pDest, comperand, value);
}