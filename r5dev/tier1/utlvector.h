//============ Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
// A growable array class that maintains a free list and keeps elements
// in the same location
//===========================================================================//
#pragma once

#include "tier1/utlmemory.h"

struct __declspec(align(4)) CUtlVector
{
	void* vtable;
	CUtlMemory m_Memory;
	int m_Size;
};


