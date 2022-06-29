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

template< class T, class A = CUtlMemory<T> >
class CUtlVector
{
	typedef A CAllocator;
public:
	typedef T ElemType_t;
protected:
	CAllocator m_Memory;
	int m_Size;
};
