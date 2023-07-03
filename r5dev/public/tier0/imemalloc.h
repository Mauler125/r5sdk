//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: This header should never be used directly from leaf code!!!
// Instead, just add the file memoverride.cpp into your project and all this
// will automagically be used
//
// $NoKeywords: $
//=============================================================================//

#ifndef TIER0_IMEMALLOC_H
#define TIER0_IMEMALLOC_H

/// This interface class is used to let the mem_dump command retrieve
/// information about memory allocations outside of the heap. It is currently
/// used by CMemoryStack to report on its allocations.
abstract_class IMemoryInfo
{
public:
	virtual const char* GetMemoryName() const = 0; // User friendly name for this stack or pool
	virtual size_t GetAllocatedBytes() const = 0; // Number of bytes currently allocated
	virtual size_t GetCommittedBytes() const = 0; // Bytes committed -- may be greater than allocated.
	virtual size_t GetReservedBytes() const = 0; // Bytes reserved -- may be greater than committed.
	virtual size_t GetHighestBytes() const = 0; // The maximum number of bytes allocated or committed.
};

//-----------------------------------------------------------------------------

#if (defined(_DEBUG) || defined(USE_MEM_DEBUG))
struct MemAllocFileLine_t
{
	const char* pszFile;
	int line;
};

#define MEMALLOC_DEFINE_EXTERNAL_TRACKING( tag ) \
	static CUtlMap<void *, MemAllocFileLine_t, int> s_##tag##Allocs( DefLessFunc( void *) ); \
	CUtlMap<void *, MemAllocFileLine_t, int> * g_p##tag##Allocs = &s_##tag##Allocs; \
	static CThreadFastMutex s_##tag##AllocsMutex; \
	CThreadFastMutex * g_p##tag##AllocsMutex = &s_##tag##AllocsMutex; \
	const char * g_psz##tag##Alloc = strcpy( (char *)MemAlloc_Alloc( strlen( #tag "Alloc" ) + 1, "intentional leak", 0 ), #tag "Alloc" );

#define MEMALLOC_DECLARE_EXTERNAL_TRACKING( tag ) \
	extern CUtlMap<void *, MemAllocFileLine_t, int> * g_p##tag##Allocs; \
	extern CThreadFastMutex *g_p##tag##AllocsMutex; \
	extern const char * g_psz##tag##Alloc;

#define MemAlloc_RegisterExternalAllocation( tag, p, size ) \
	if ( !p ) \
		; \
	else \
	{ \
		AUTO_LOCK_FM( *g_p##tag##AllocsMutex ); \
		MemAllocFileLine_t fileLine = { g_psz##tag##Alloc, 0 }; \
		g_pMemAlloc->GetActualDbgInfo( fileLine.pszFile, fileLine.line ); \
		if ( fileLine.pszFile != g_psz##tag##Alloc ) \
		{ \
			g_p##tag##Allocs->Insert( p, fileLine ); \
		} \
		\
		MemAlloc_RegisterAllocation( fileLine.pszFile, fileLine.line, size, size, 0); \
	}

#define MemAlloc_RegisterExternalDeallocation( tag, p, size ) \
	if ( !p ) \
		; \
	else \
	{ \
		AUTO_LOCK_FM( *g_p##tag##AllocsMutex ); \
		MemAllocFileLine_t fileLine = { g_psz##tag##Alloc, 0 }; \
		CUtlMap<void *, MemAllocFileLine_t, int>::IndexType_t iRecordedFileLine = g_p##tag##Allocs->Find( p ); \
		if ( iRecordedFileLine != g_p##tag##Allocs->InvalidIndex() ) \
		{ \
			fileLine = (*g_p##tag##Allocs)[iRecordedFileLine]; \
			g_p##tag##Allocs->RemoveAt( iRecordedFileLine ); \
		} \
		\
		MemAlloc_RegisterDeallocation( fileLine.pszFile, fileLine.line, size, size, 0); \
	}

#else

#define MEMALLOC_DEFINE_EXTERNAL_TRACKING( tag )
#define MEMALLOC_DECLARE_EXTERNAL_TRACKING( tag )
#define MemAlloc_RegisterExternalAllocation( tag, p, size ) ((void)0)
#define MemAlloc_RegisterExternalDeallocation( tag, p, size ) ((void)0)

#endif

//-----------------------------------------------------------------------------
// Stubbed, not used for anything!!!
#define MEM_ALLOC_CREDIT_(tag)	((void)0)
#define MemAlloc_PushAllocDbgInfo( pszFile, line ) ((void)0)
#define MemAlloc_PopAllocDbgInfo() ((void)0)
#define MemAlloc_RegisterAllocation( pFileName, nLine, nLogicalSize, nActualSize, nTime ) ((void)0)
#define MemAlloc_RegisterDeallocation( pFileName, nLine, nLogicalSize, nActualSize, nTime ) ((void)0)

#endif /* TIER0_MEMALLOC_H */