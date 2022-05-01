//============ Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A dictionary mapping from symbol to structure 
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#pragma once

template <class T, class I>
class CUtlDict
{
public:
	CUtlDict<T, I>(uintptr_t ptr) : m_Elements(ptr) {};
	T Find(__int64 entry);

private:
	uintptr_t m_Elements;
};

template <class T, class I>
T CUtlDict<T, I>::Find(__int64 entry)
{
	return *(T*)(m_Elements + 24 * entry + 16);
}