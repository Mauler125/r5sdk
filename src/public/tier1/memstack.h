//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: A fast stack memory allocator that uses virtual memory if available
//
//===========================================================================//

#ifndef MEMSTACK_H
#define MEMSTACK_H

#if defined( _WIN32 )
#pragma once
#endif

#include "tier0/imemalloc.h"
#include "tier1/utlvector.h"

#if defined( _WIN32 ) || defined( _PS3 )
#define MEMSTACK_VIRTUAL_MEMORY_AVAILABLE
#endif

//-----------------------------------------------------------------------------

typedef size_t MemoryStackMark_t;

class CMemoryStack //: private IMemoryInfo
{
public:
	CMemoryStack();
	~CMemoryStack();

	bool Init( const char *pszAllocOwner, size_t maxSize = 0, size_t commitIncrement = 0, size_t initialCommit = 0, size_t alignment = 16 );
	void Term();

	size_t GetSize() const;
	size_t GetMaxSize() const ;
	size_t GetUsed() const;
	
	void *Alloc( size_t bytes, bool bClear = false ) RESTRICT;

	MemoryStackMark_t GetCurrentAllocPoint() const;
	void FreeToAllocPoint( MemoryStackMark_t mark, bool bDecommit = true );
	void FreeAll( bool bDecommit = true );
	
	void Access( void **ppRegion, size_t *pBytes );

	void PrintContents() const;

	void *GetBase();
	const void *GetBase() const {  return const_cast<CMemoryStack *>(this)->GetBase(); }

	bool CommitSize( size_t nBytes );

	void SetAllocOwner( const char *pszAllocOwner );

private:
	bool CommitTo( byte * ) RESTRICT;
	void RegisterAllocation();
	void RegisterDeallocation( bool bShouldSpew );

	//const char* GetMemoryName() const OVERRIDE; // User friendly name for this stack or pool
	//size_t GetAllocatedBytes() const OVERRIDE; // Number of bytes currently allocated
	//size_t GetCommittedBytes() const OVERRIDE; // Bytes committed -- may be greater than allocated.
	//size_t GetReservedBytes() const OVERRIDE; // Bytes reserved -- may be greater than committed.
	//size_t GetHighestBytes() const OVERRIDE; // The maximum number of bytes allocated or committed.	

	byte *m_pNextAlloc; // Current alloc point (m_pNextAlloc - m_pBase == allocated bytes)
	byte *m_pCommitLimit; // The current end of the committed memory. On systems without dynamic commit/decommit this is always m_pAllocLimit
	byte *m_pAllocLimit; // The top of the allocated address space (m_pBase + m_maxSize)

	byte *m_pBase;

	// Track the highest alloc limit seen.
	byte* m_pHighestAllocLimit; // This field probably no longer exist, but there is a 64bit type at this offset.
	byte* m_pUnkPtr; // Unknown..


	bool m_bRegisteredAllocation;
	bool m_bPhysical;
	char *m_pszAllocOwner;

	size_t m_unkSize; // Unknown field..
	size_t m_maxSize; // m_maxSize stores how big the stack can grow. It measures the reservation size.
	size_t m_alignment;
#ifdef MEMSTACK_VIRTUAL_MEMORY_AVAILABLE
	size_t m_commitIncrement;
	size_t m_minCommit;
#endif
#if defined( MEMSTACK_VIRTUAL_MEMORY_AVAILABLE ) && defined( _PS3 )
	IVirtualMemorySection *m_pVirtualMemorySection;
#endif

private:
	// Make the assignment operator and copy constructor private and unimplemented.
	CMemoryStack& operator=( const CMemoryStack& );
	CMemoryStack( const CMemoryStack& );
};

//-------------------------------------

FORCEINLINE void *CMemoryStack::Alloc( size_t bytes, bool bClear ) RESTRICT
{
	sizeof(CMemoryStack);
	Assert( m_pBase );

	bytes = MAX( bytes, m_alignment );
	bytes = AlignValue( bytes, m_alignment );

	void *pResult = m_pNextAlloc;
	byte *pNextAlloc = m_pNextAlloc + bytes;

	if ( pNextAlloc > m_pCommitLimit )
	{
		if ( !CommitTo( pNextAlloc ) )
		{
			return nullptr;
		}
	}

	if ( bClear )
	{
		memset( pResult, 0, bytes );
	}

	m_pNextAlloc = pNextAlloc;
	m_pHighestAllocLimit = Max( m_pNextAlloc, m_pHighestAllocLimit );

	return pResult;
}

//-------------------------------------

inline bool CMemoryStack::CommitSize( size_t nBytes )
{
	if ( GetSize() != nBytes )
	{
		return CommitTo( m_pBase + nBytes );
	}
	return true;
}

//-------------------------------------

// How big can this memory stack grow? This is equivalent to how many
// bytes are reserved.
inline size_t CMemoryStack::GetMaxSize() const
{ 
	return m_maxSize;
}

//-------------------------------------

inline size_t CMemoryStack::GetUsed() const
{ 
	return ( m_pNextAlloc - m_pBase ); 
}

//-------------------------------------

inline void *CMemoryStack::GetBase()
{
	return m_pBase;
}

//-------------------------------------

inline MemoryStackMark_t CMemoryStack::GetCurrentAllocPoint() const
{
	return ( m_pNextAlloc - m_pBase );
}


//-----------------------------------------------------------------------------
// The CUtlMemoryStack class:
// A fixed memory class
//-----------------------------------------------------------------------------
template< typename T, typename I, size_t MAX_SIZE, size_t COMMIT_SIZE = 0, size_t INITIAL_COMMIT = 0 >
class CUtlMemoryStack
{
public:
	// constructor, destructor
	CUtlMemoryStack( int nGrowSize = 0, int nInitSize = 0 )	{ m_MemoryStack.Init( "CUtlMemoryStack", MAX_SIZE * sizeof(T), COMMIT_SIZE * sizeof(T), INITIAL_COMMIT * sizeof(T), 4 ); COMPILE_TIME_ASSERT( sizeof(T) % 4 == 0 );	}
	CUtlMemoryStack( T* pMemory, int numElements )			{ Assert( 0 ); 										}

	// Can we use this index?
	bool IsIdxValid( I i ) const							{ long x=i; return (x >= 0) && (x < m_nAllocated); }

	// Specify the invalid ('null') index that we'll only return on failure
	static const I INVALID_INDEX = ( I )-1; // For use with COMPILE_TIME_ASSERT
	static I InvalidIndex() { return INVALID_INDEX; }

	class Iterator_t
	{
		Iterator_t( I i ) : index( i ) {}
		I index;
		friend class CUtlMemoryStack<T,I,MAX_SIZE, COMMIT_SIZE, INITIAL_COMMIT>;
	public:
		bool operator==( const Iterator_t it ) const		{ return index == it.index; }
		bool operator!=( const Iterator_t it ) const		{ return index != it.index; }
	};
	Iterator_t First() const								{ return Iterator_t( m_nAllocated ? 0 : InvalidIndex() ); }
	Iterator_t Next( const Iterator_t &it ) const			{ return Iterator_t( it.index < m_nAllocated ? it.index + 1 : InvalidIndex() ); }
	I GetIndex( const Iterator_t &it ) const				{ return it.index; }
	bool IsIdxAfter( I i, const Iterator_t &it ) const		{ return i > it.index; }
	bool IsValidIterator( const Iterator_t &it ) const		{ long x=it.index; return x >= 0 && x < m_nAllocated; }
	Iterator_t InvalidIterator() const						{ return Iterator_t( InvalidIndex() ); }

	// Gets the base address
	T* Base()												{ return (T*)m_MemoryStack.GetBase(); }
	const T* Base() const									{ return (const T*)m_MemoryStack.GetBase(); }

	// element access
	T& operator[]( I i )									{ Assert( IsIdxValid(i) ); return Base()[i];	}
	const T& operator[]( I i ) const						{ Assert( IsIdxValid(i) ); return Base()[i];	}
	T& Element( I i )										{ Assert( IsIdxValid(i) ); return Base()[i];	}
	const T& Element( I i ) const							{ Assert( IsIdxValid(i) ); return Base()[i];	}

	// Attaches the buffer to external memory....
	void SetExternalBuffer( T* pMemory, int numElements )	{ Assert( 0 ); }

	// Size
	int NumAllocated() const								{ return m_nAllocated; }
	int Count() const										{ return m_nAllocated; }

	// Grows the memory, so that at least allocated + num elements are allocated
	void Grow( int num = 1 )								{ Assert( num > 0 ); m_nAllocated += num; m_MemoryStack.Alloc( num * sizeof(T) ); }

	// Makes sure we've got at least this much memory
	void EnsureCapacity( int num )							{ Assert( num <= MAX_SIZE ); if ( m_nAllocated < num ) Grow( num - m_nAllocated ); }

	// Memory deallocation
	void Purge()											{ m_MemoryStack.FreeAll(); m_nAllocated = 0; }

	// is the memory externally allocated?
	bool IsExternallyAllocated() const						{ return false; }

	// Set the size by which the memory grows
	void SetGrowSize( int size )							{ Assert( 0 ); }

	// Identify the owner of this memory stack's memory
	void SetAllocOwner( const char *pszAllocOwner )			{ m_MemoryStack.SetAllocOwner( pszAllocOwner ); }

private:
	CMemoryStack m_MemoryStack;
	int m_nAllocated;
};

#endif // MEMSTACK_H
