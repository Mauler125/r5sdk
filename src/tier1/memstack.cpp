//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose:
//
//=============================================================================//

#include "tier0/dbg.h"
#include "tier0/memstd.h"
#include "tier1/memstack.h"
#include "tier1/utlmap.h"
#include "tier0/memdbgon.h"

#ifdef _WIN32
#pragma warning(disable:4073)
#pragma init_seg(lib)
#endif

#define VA_COMMIT_FLAGS MEM_COMMIT
#define VA_RESERVE_FLAGS MEM_RESERVE

static volatile bool bSpewAllocations = false; // TODO: Register CMemoryStacks with g_pMemAlloc, so it can spew a summary

//-----------------------------------------------------------------------------

MEMALLOC_DEFINE_EXTERNAL_TRACKING(CMemoryStack);

//-----------------------------------------------------------------------------

void PrintStatus( void* p )
{
	CMemoryStack* pMemoryStack = (CMemoryStack*)p;

	pMemoryStack->PrintContents();
}

CMemoryStack::CMemoryStack()
 : 	m_pNextAlloc( nullptr )
	, m_pCommitLimit( nullptr )
	, m_pAllocLimit( nullptr )
	, m_pHighestAllocLimit( nullptr )
	, m_pBase( nullptr )
	, m_pUnkPtr( nullptr )
	, m_bRegisteredAllocation( false )
 	, m_maxSize( 0 )
	, m_alignment( 16 )
#ifdef MEMSTACK_VIRTUAL_MEMORY_AVAILABLE
 	, m_commitIncrement( 0 )
	, m_minCommit( 0 )
	#ifdef _PS3
		, m_pVirtualMemorySection( NULL )
	#endif
#endif
{
	//AddMemoryInfoCallback( this );
	m_pszAllocOwner = strdup( "CMemoryStack unattributed" );
}
	
//-------------------------------------

CMemoryStack::~CMemoryStack()
{
	if ( m_pBase )
		Term();

	//RemoveMemoryInfoCallback( this );
	free( m_pszAllocOwner );
}

//-------------------------------------

bool CMemoryStack::Init( const char *pszAllocOwner, size_t maxSize, size_t commitIncrement, size_t initialCommit, size_t alignment )
{
	Assert( !m_pBase );

	m_bPhysical = false;
	m_unkSize = 0x100000; // NOTE: always set to this value in this place for R5 (see [r5apex.exe+478B96]).

	m_maxSize = maxSize;
	m_alignment = AlignValue( alignment, 4 );

	Assert( m_alignment == alignment );
	Assert( m_maxSize > 0 );

	SetAllocOwner( pszAllocOwner );

#ifdef MEMSTACK_VIRTUAL_MEMORY_AVAILABLE

#ifdef _PS3
	// Memory can only be committed in page-size increments on PS3
	static const unsigned PS3_PAGE_SIZE = 64*1024;
	if ( commitSize < PS3_PAGE_SIZE )
		commitSize = PS3_PAGE_SIZE;
#endif

	if ( commitIncrement != 0 )
	{
		m_commitIncrement = commitIncrement;
	}

	uint64 pageSize;

#ifdef _PS3
	pageSize = PS3_PAGE_SIZE;
#elif defined( _X360 )
	pageSize = 64 * 1024;
#else
	SYSTEM_INFO sysInfo;
	GetSystemInfo( &sysInfo );
	Assert( !( sysInfo.dwPageSize & (sysInfo.dwPageSize-1)) );
	pageSize = sysInfo.dwPageSize;
#endif

	if ( m_commitIncrement == 0 )
	{
		m_commitIncrement = pageSize;
	}
	else
	{
		m_commitIncrement = AlignValue( m_commitIncrement, pageSize );
	}

	m_maxSize = AlignValue( m_maxSize, m_commitIncrement );
	
	Assert( m_maxSize % pageSize == 0 && m_commitIncrement % pageSize == 0 && m_commitIncrement <= m_maxSize );

#ifdef _WIN32
	m_pBase = (unsigned char *)VirtualAlloc( NULL, m_maxSize, VA_RESERVE_FLAGS, PAGE_NOACCESS );
#else
	m_pVirtualMemorySection = g_pMemAlloc->AllocateVirtualMemorySection( m_maxSize );
	if ( !m_pVirtualMemorySection )
	{
		Warning( "AllocateVirtualMemorySection failed( size=%d )\n", m_maxSize );
		Assert( 0 );
		m_pBase = NULL;
	}
	else
	{
		m_pBase = ( byte* ) m_pVirtualMemorySection->GetBaseAddress();
	}
#endif
	if ( !m_pBase )
	{
#if !defined( NO_MALLOC_OVERRIDE )
		g_pMemAlloc->OutOfMemory();
#endif
		return false;
	}
	m_pCommitLimit = m_pNextAlloc = m_pBase;

	if ( initialCommit )
	{
		initialCommit = AlignValue( initialCommit, m_commitIncrement );
		Assert( initialCommit <= m_maxSize );
		bool bInitialCommitSucceeded = false;
#ifdef _WIN32
		bInitialCommitSucceeded = !!VirtualAlloc( m_pCommitLimit, initialCommit, VA_COMMIT_FLAGS, PAGE_READWRITE );
#else
		m_pVirtualMemorySection->CommitPages( m_pCommitLimit, initialCommit );
		bInitialCommitSucceeded = true;
#endif
		if ( !bInitialCommitSucceeded )
		{
#if !defined( NO_MALLOC_OVERRIDE )
			g_pMemAlloc->OutOfMemory( initialCommit );
#endif
			return false;
		}
		m_minCommit = initialCommit;
		m_pCommitLimit += initialCommit;
		RegisterAllocation();
	}

#else
	m_pBase = (byte*)MemAlloc_AllocAligned( m_maxSize, alignment ? alignment : 1 );
	m_pNextAlloc = m_pBase;
	m_pCommitLimit = m_pBase + m_maxSize;
#endif

	m_pHighestAllocLimit = m_pNextAlloc;

	m_pAllocLimit = m_pBase + m_maxSize;

	return ( m_pBase != nullptr );
}

//-------------------------------------

#ifdef _GAMECONSOLE
bool CMemoryStack::InitPhysical( const char *pszAllocOwner, size_t size, size_t nBaseAddrAlignment, size_t alignment, uint32 nFlags )
{
	m_bPhysical = true;

	m_maxSize = m_commitIncrement = size;
	m_alignment = AlignValue( alignment, 4 );

	SetAllocOwner( pszAllocOwner );

#ifdef _X360
	int flags = PAGE_READWRITE | nFlags;
	if ( size >= 16*1024*1024 )
	{
		flags |= MEM_16MB_PAGES;
	}
	else
	{
		flags |= MEM_LARGE_PAGES;
	}
	m_pBase = (unsigned char *)XPhysicalAlloc( m_maxSize, MAXULONG_PTR, nBaseAddrAlignment, flags );
#elif defined (_PS3)
	m_pBase = (byte*)nFlags;
	m_pBase = (byte*)AlignValue( (uintp)m_pBase, m_alignment );
#else
#pragma error
#endif

	Assert( m_pBase );
	m_pNextAlloc = m_pBase;
	m_pCommitLimit = m_pBase + m_maxSize;
	m_pAllocLimit = m_pBase + m_maxSize;
	m_pHighestAllocLimit = m_pNextAlloc;

	RegisterAllocation();
	return ( m_pBase != NULL );
}
#endif

//-------------------------------------

void CMemoryStack::Term()
{
	FreeAll();
	if ( m_pBase )
	{
#ifdef _GAMECONSOLE
		if ( m_bPhysical )
		{
#if defined( _X360 )
			XPhysicalFree( m_pBase );
#elif defined( _PS3 )
#else
#pragma error
#endif
			m_pCommitLimit = m_pBase = NULL;
			m_maxSize = 0;
			RegisterDeallocation(true);
			m_bPhysical = false;
			return;
		}
#endif // _GAMECONSOLE

#ifdef MEMSTACK_VIRTUAL_MEMORY_AVAILABLE
#if defined(_WIN32)
		VirtualFree( m_pBase, 0, MEM_RELEASE );
#else
		m_pVirtualMemorySection->Release();
		m_pVirtualMemorySection = NULL;
#endif
#else
		MemAlloc_FreeAligned( m_pBase );
#endif
		m_pBase = nullptr;
		// Zero these variables to avoid getting misleading mem_dump
		// results when m_pBase is NULL.
		m_pNextAlloc = nullptr;
		m_pCommitLimit = nullptr;
		m_pHighestAllocLimit = nullptr;
		m_pUnkPtr = nullptr;
		m_maxSize = 0;
		RegisterDeallocation(true);
	}
}

//-------------------------------------

uint64 CMemoryStack::GetSize() const
{ 
	if ( m_bPhysical )
		return m_maxSize;

#ifdef MEMSTACK_VIRTUAL_MEMORY_AVAILABLE
	return m_pCommitLimit - m_pBase; 
#else
	return m_maxSize;
#endif
}


//-------------------------------------

bool CMemoryStack::CommitTo( byte *pNextAlloc ) RESTRICT
{
	if ( m_bPhysical )
	{
		return false;
	}

#ifdef MEMSTACK_VIRTUAL_MEMORY_AVAILABLE
	unsigned char *	pNewCommitLimit = AlignValue( pNextAlloc, m_commitIncrement );
	ptrdiff_t commitIncrement 		= pNewCommitLimit - m_pCommitLimit;
	
	if( m_pCommitLimit + commitIncrement > m_pAllocLimit )
	{
#if !defined( NO_MALLOC_OVERRIDE )
		g_pMemAlloc->OutOfMemory( commitIncrement );
#endif
		return false;
	}

	if ( pNewCommitLimit > m_pCommitLimit )
	{
		RegisterDeallocation(false);
		bool bAllocationSucceeded = false;
#ifdef _WIN32
		bAllocationSucceeded = !!VirtualAlloc( m_pCommitLimit, commitIncrement, VA_COMMIT_FLAGS, PAGE_READWRITE );
#else
		bAllocationSucceeded = m_pVirtualMemorySection->CommitPages( m_pCommitLimit, commitIncrement );
#endif
		if ( !bAllocationSucceeded )
		{
#if !defined( NO_MALLOC_OVERRIDE )
			g_pMemAlloc->OutOfMemory( commitIncrement );
#endif
			return false;
		}
		m_pCommitLimit = pNewCommitLimit;

		RegisterAllocation();
	}
	else if ( pNewCommitLimit < m_pCommitLimit )
	{
		if  ( m_pNextAlloc > pNewCommitLimit )
		{
			Warning( eDLL_T::COMMON, "ATTEMPTED TO DECOMMIT OWNED MEMORY STACK SPACE\n" );
			pNewCommitLimit = AlignValue( m_pNextAlloc, m_commitIncrement );
		}

		if ( pNewCommitLimit < m_pCommitLimit )
		{
			RegisterDeallocation(false);
			ptrdiff_t decommitIncrement = m_pCommitLimit - pNewCommitLimit;
#ifdef _WIN32
			VirtualFree( pNewCommitLimit, decommitIncrement, MEM_DECOMMIT );
#else
			m_pVirtualMemorySection->DecommitPages( pNewCommitLimit, decommitIncrement );
#endif
			m_pCommitLimit = pNewCommitLimit;
			RegisterAllocation();
		}
	}

	return true;
#else
	return false;
#endif
}

// Identify the owner of this memory stack's memory
void CMemoryStack::SetAllocOwner( const char *pszAllocOwner )
{
	if ( !pszAllocOwner || !Q_strcmp( m_pszAllocOwner, pszAllocOwner ) )
		return;
	free( m_pszAllocOwner );
	m_pszAllocOwner = strdup( pszAllocOwner );
}

void CMemoryStack::RegisterAllocation()
{
	// 'physical' allocations on PS3 come from RSX local memory, so we don't count them here:
	if ( IsPS3() && m_bPhysical )
		return;

	if ( GetSize() )
	{
		if ( m_bRegisteredAllocation )
			Warning( eDLL_T::COMMON, "CMemoryStack: ERROR - mismatched RegisterAllocation/RegisterDeallocation!\n" );

		// NOTE: we deliberately don't use MemAlloc_RegisterExternalAllocation. CMemoryStack needs to bypass 'GetActualDbgInfo'
		// due to the way it allocates memory: there's just one representative memory address (m_pBase), it grows at unpredictable
		// times (in CommitTo, not every Alloc call) and it is freed en-masse (instead of freeing each individual allocation).
		MemAlloc_RegisterAllocation( m_pszAllocOwner, 0, GetSize(), GetSize(), 0 );
	}
	m_bRegisteredAllocation = true;

	// Temp memorystack spew: very useful when we crash out of memory
	if ( IsGameConsole() && bSpewAllocations ) DevMsg( eDLL_T::COMMON, "CMemoryStack: %4.1fMB (%s)\n", GetSize()/(float)(1024*1024), m_pszAllocOwner );
}

void CMemoryStack::RegisterDeallocation( bool bShouldSpewSize )
{
	// 'physical' allocations on PS3 come from RSX local memory, so we don't count them here:
	if ( IsPS3() && m_bPhysical )
		return;

	if ( GetSize() )
	{
		if ( !m_bRegisteredAllocation )
			Warning( eDLL_T::COMMON, "CMemoryStack: ERROR - mismatched RegisterAllocation/RegisterDeallocation!\n" );
		MemAlloc_RegisterDeallocation( m_pszAllocOwner, 0, GetSize(), GetSize(), 0 );
	}
	m_bRegisteredAllocation = false;

	// Temp memorystack spew: very useful when we crash out of memory
	if ( bShouldSpewSize && IsGameConsole() && bSpewAllocations ) DevMsg( eDLL_T::COMMON, "CMemoryStack: %4.1fMB (%s)\n", GetSize()/(float)(1024*1024), m_pszAllocOwner );
}

//-------------------------------------

void CMemoryStack::FreeToAllocPoint( MemoryStackMark_t mark, bool bDecommit )
{
	mark = AlignValue( mark, m_alignment );
	byte *pAllocPoint = m_pBase + mark;

	Assert( pAllocPoint >= m_pBase && pAllocPoint <= m_pNextAlloc );
	if ( pAllocPoint >= m_pBase && pAllocPoint <= m_pNextAlloc )
	{
		m_pNextAlloc = pAllocPoint;
#ifdef MEMSTACK_VIRTUAL_MEMORY_AVAILABLE
		if ( bDecommit && !m_bPhysical )
		{
			CommitTo( MAX( m_pNextAlloc, (m_pBase + m_minCommit) ) );
		}
#endif
	}
}

//-------------------------------------

void CMemoryStack::FreeAll( bool bDecommit )
{
	if ( m_pBase && ( m_pBase < m_pCommitLimit ) )
	{
		FreeToAllocPoint( 0, bDecommit );
	}
}

//-------------------------------------

void CMemoryStack::Access( void **ppRegion, uint64 *pBytes )
{
	*ppRegion = m_pBase;
	*pBytes = ( m_pNextAlloc - m_pBase);
}

//const char* CMemoryStack::GetMemoryName() const
//{
//	return m_pszAllocOwner;
//}
//
//size_t CMemoryStack::GetAllocatedBytes() const
//{
//	return GetUsed();
//}
//
//size_t CMemoryStack::GetCommittedBytes() const
//{
//	return GetSize();
//}
//
//size_t CMemoryStack::GetReservedBytes() const
//{
//	return GetMaxSize();
//}

//size_t CMemoryStack::GetHighestBytes() const
//{
//	size_t highest = m_pHighestAllocLimit - m_pBase;
//	return highest;
//}

//-------------------------------------

void CMemoryStack::PrintContents() const
{
	if (IsCert())
	{
		return;
	}

	MEMORY_BASIC_INFORMATION info;
	char moduleName[260];
	strcpy( moduleName, "unknown module" );
	// Because this code is statically linked into each DLL, this function and the PrintStatus
	// function will be in the DLL that constructed the CMemoryStack object. We can then
	// retrieve the DLL name to give slightly more verbose memory dumps.
	if ( VirtualQuery( &PrintStatus, &info, sizeof( info ) ) == sizeof( info ) )
	{
		GetModuleFileNameA( (HMODULE) info.AllocationBase, moduleName, _countof( moduleName ) );
		moduleName[ _countof( moduleName )-1 ] = 0;
	}
	DevMsg( eDLL_T::COMMON, "CMemoryStack %s in %s\n", m_pszAllocOwner, moduleName );
	DevMsg( eDLL_T::COMMON, "    Total used memory:      %zu KB\n", GetUsed() / 1024 );
	DevMsg( eDLL_T::COMMON, "    Total committed memory: %zu KB\n", GetSize() / 1024 );
	DevMsg( eDLL_T::COMMON, "    Max committed memory: %zu KB out of %zu KB\n", (m_pHighestAllocLimit - m_pBase) / 1024, GetMaxSize() / 1024 );
}
