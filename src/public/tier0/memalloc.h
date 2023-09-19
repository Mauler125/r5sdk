#ifndef TIER0_MEMALLOC_H
#define TIER0_MEMALLOC_H

#if defined(MSVC) && ( defined(_DEBUG) || defined(USE_MEM_DEBUG) )

#pragma warning(disable:4290)
#pragma warning(push)
//#include <typeinfo.h>

// MEM_DEBUG_CLASSNAME is opt-in.
// Note: typeid().name() is not threadsafe, so if the project needs to access it in multiple threads
// simultaneously, it'll need a mutex.

#define MEM_ALLOC_CREDIT_(tag)	((void)0) // Stubbed for now.

#if defined(_CPPRTTI) && defined(MEM_DEBUG_CLASSNAME)

template <typename T> const char* MemAllocClassName(T* p)
{
	static const char* pszName = typeid(*p).name(); // @TODO: support having debug heap ignore certain allocations, and ignore memory allocated here [5/7/2009 tom]
	return pszName;
}

#define MEM_ALLOC_CREDIT_CLASS()	MEM_ALLOC_CREDIT_( MemAllocClassName( this ) )
#define MEM_ALLOC_CLASSNAME(type) (typeid((type*)(0)).name())
#else
#define MEM_ALLOC_CREDIT_CLASS()	MEM_ALLOC_CREDIT_( __FILE__ )
#define MEM_ALLOC_CLASSNAME(type) (__FILE__)
#endif

// MEM_ALLOC_CREDIT_FUNCTION is used when no this pointer is available ( inside 'new' overloads, for example )
#ifdef _MSC_VER
#define MEM_ALLOC_CREDIT_FUNCTION()		MEM_ALLOC_CREDIT_( __FUNCTION__ )
#else
#define MEM_ALLOC_CREDIT_FUNCTION() (__FILE__)
#endif

#pragma warning(pop)
#else
#define MEM_ALLOC_CREDIT_CLASS()
#define MEM_ALLOC_CLASSNAME(type) NULL
#define MEM_ALLOC_CREDIT_FUNCTION() 
#endif

#define MEM_ALLOC_CREDIT() // Stubbed

#endif /* TIER0_MEMALLOC_H */