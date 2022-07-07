#ifndef PLATFORM_H
#define PLATFORM_H

#if defined( _WIN32 ) && defined( _MSC_VER ) && ( _MSC_VER >= 1400 )
#pragma intrinsic(__rdtsc)
#endif


#define TIER0_DLL_EXPORT
#define COMPILER_MSVC

//-----------------------------------------------------------------------------
// Set up platform defines.
//-----------------------------------------------------------------------------
#ifdef _WIN32
#define IsPlatformLinux()	0
#define IsPlatformPosix()	0
#define IsPlatformOSX()		0
#define IsOSXOpenGL()		0
#define IsPlatformPS3()		0
#define IsPlatformPS3_PPU()	0
#define IsPlatformPS3_SPU()	0
#define PLATFORM_WINDOWS	1
#define PLATFORM_OPENGL 0

#ifndef _X360
#define IsPlatformX360() 0
#define IsPlatformWindowsPC() 1
#define PLATFORM_WINDOWS_PC 1

#ifdef _WIN64
#define IsPlatformWindowsPC64() 1
#define IsPlatformWindowsPC32() 0
#define PLATFORM_WINDOWS_PC64 1

#define COMPILER_MSVC64 1
#else
#define IsPlatformWindowsPC64() 0
#define IsPlatformWindowsPC32() 1
#define PLATFORM_WINDOWS_PC32 1
#endif

#else // _X360

#define IsPlatformWindowsPC()	0
#define IsPlatformWindowsPC64() 0
#define IsPlatformWindowsPC32() 0
#define IsPlatformX360()		1
#define PLATFORM_X360 1

#endif // _X360
#elif defined(_PS3)

// Adding IsPlatformOpenGL() to help fix a bunch of code that was using IsPosix() to infer if the DX->GL translation layer was being used.
#if defined( DX_TO_GL_ABSTRACTION )
#define IsPlatformOpenGL() true
#else
#define IsPlatformOpenGL() false
#endif

#define IsPlatformX360()		0
#define IsPlatformPS3()			1
#ifdef SPU
#define IsPlatformPS3_PPU()		0
#define IsPlatformPS3_SPU()		1
#else
#define IsPlatformPS3_PPU()		1
#define IsPlatformPS3_SPU()		0
#endif
#define IsPlatformWindowsPC()	0
#define IsPlatformWindowsPC64()	0
#define IsPlatformWindowsPC32()	0
#define IsPlatformPosix()		1
#define PLATFORM_POSIX 1
#define PLATFORM_OPENGL 0

#define IsPlatformLinux() 0
#define IsPlatformOSX() 0
#define IsOSXOpenGL() 0


#elif defined(POSIX)
#define IsPlatformX360()		0
#define IsPlatformPS3()			0
#define IsPlatformPS3_PPU()		0
#define IsPlatformPS3_SPU()		0
#define IsPlatformWindowsPC()	0
#define IsPlatformWindowsPC64()	0
#define IsPlatformWindowsPC32()	0
#define IsPlatformPosix()		1
#define PLATFORM_POSIX 1

#if defined( LINUX ) && !defined( OSX ) // for havok we define both symbols, so don't let the osx build wander down here
#define IsPlatformLinux() 1
#define IsPlatformOSX() 0
#define IsOSXOpenGL() 0
#define PLATFORM_OPENGL 0
#define PLATFORM_LINUX 1
#elif defined ( OSX )
#define IsPlatformLinux() 0
#define IsPlatformOSX() 1
#define IsOSXOpenGL() 1
#define PLATFORM_OSX 1
#define PLATFORM_OPENGL 1
#else
#define IsPlatformLinux() 0
#define IsPlatformOSX() 0
#define IsOSXOpenGL() 0
#define PLATFORM_OPENGL 0
#endif

#else
#error
#endif

//-----------------------------------------------------------------------------
// Old-school defines we're going to support since much code uses them
//-----------------------------------------------------------------------------
#if CROSS_PLATFORM_VERSION < 2

#define IsLinux()	IsPlatformLinux() 
#define IsOSX()		IsPlatformOSX()
#define IsPosix()	IsPlatformPosix()
#define IsX360()	IsPlatformX360()
#define IsPS3()		IsPlatformPS3()

// Setup platform defines.
#ifdef COMPILER_MSVC
#define MSVC 1
#endif

#ifdef COMPILER_GCC
#define GNUC 1
#endif

#if defined( _WIN32 )
#define _WINDOWS 1
#endif

#ifdef PLATFORM_WINDOWS_PC
#define IS_WINDOWS_PC 1
#endif

#if _MSC_VER >= 1800
#define	VECTORCALL __vectorcall 
#else 
#define	VECTORCALL 
#endif

#endif // CROSS_PLATFORM_VERSION < 2

#if defined( GNUC )	&& !defined( COMPILER_PS3 ) // use pre-align on PS3
// gnuc has the align decoration at the end
#define ALIGN4
#define ALIGN8 
#define ALIGN16
#define ALIGN32
#define ALIGN128

#undef ALIGN16_POST
#define ALIGN4_POST DECL_ALIGN(4)
#define ALIGN8_POST DECL_ALIGN(8)
#define ALIGN16_POST DECL_ALIGN(16)
#define ALIGN32_POST DECL_ALIGN(32)
#define ALIGN128_POST DECL_ALIGN(128)
#else
// MSVC has the align at the start of the struct
// PS3 SNC supports both
#define ALIGN4 DECL_ALIGN(4)
#define ALIGN8 DECL_ALIGN(8)
#define ALIGN16 DECL_ALIGN(16)
#define ALIGN32 DECL_ALIGN(32)
#define ALIGN128 DECL_ALIGN(128)

#define ALIGN4_POST
#define ALIGN8_POST
#define ALIGN16_POST
#define ALIGN32_POST
#define ALIGN128_POST
#endif

// This can be used to declare an abstract (interface only) class.
// Classes marked abstract should not be instantiated.  If they are, and access violation will occur.
//
// Example of use:
//
// abstract_class CFoo
// {
//      ...
// }
//
// MSDN __declspec(novtable) documentation: http://msdn.microsoft.com/library/default.asp?url=/library/en-us/vclang/html/_langref_novtable.asp
//
// Note: NJS: This is not enabled for regular PC, due to not knowing the implications of exporting a class with no no vtable.
//       It's probable that this shouldn't be an issue, but an experiment should be done to verify this.
//
#ifndef COMPILER_MSVCX360
#define abstract_class class
#else
#define abstract_class class NO_VTABLE
#endif

//-----------------------------------------------------------------------------
// Generally useful platform-independent macros (move to another file?)
//-----------------------------------------------------------------------------

// need macro for constant expression
#define ALIGN_VALUE( val, alignment ) ( ( val + alignment - 1 ) & ~( alignment - 1 ) ) 

// Force a function call site -not- to inlined. (useful for profiling)
#define DONT_INLINE(a) (((int)(a)+1)?(a):(a))

// Marks the codepath from here until the next branch entry point as unreachable,
// and asserts if any attempt is made to execute it.
#define UNREACHABLE() { Assert(0); HINT(0); }

// In cases where no default is present or appropriate, this causes MSVC to generate
// as little code as possible, and throw an assertion in debug.
#define NO_DEFAULT default: UNREACHABLE();

// Defines MAX_PATH
#ifndef MAX_PATH
#define MAX_PATH  260
#endif

//-----------------------------------------------------------------------------
// Time stamp counter
//-----------------------------------------------------------------------------
inline uint64_t Plat_Rdtsc()
{
#if defined( _X360 )
	return (uint64)__mftb32();
#elif defined( _WIN64 )
	return (uint64_t)__rdtsc();
#elif defined( _WIN32 )
#if defined( _MSC_VER ) && ( _MSC_VER >= 1400 )
	return (uint64_t)__rdtsc();
#else
	__asm rdtsc;
	__asm ret;
#endif
#elif defined( __i386__ )
	uint64 val;
	__asm__ __volatile__("rdtsc" : "=A" (val));
	return val;
#elif defined( __x86_64__ )
	uint32 lo, hi;
	__asm__ __volatile__("rdtsc" : "=a" (lo), "=d" (hi));
	return (((uint64)hi) << 32) | lo;
#else
#error
#endif
}
double Plat_FloatTime();
uint64_t Plat_MSTime();
const char* Plat_GetProcessUpTime();

//-----------------------------------------------------------------------------
// Silences a number of warnings on 360 compiles
//-----------------------------------------------------------------------------
inline uint64 CastPtrToUint64(const void* p)
{
	return (uint64)((uintptr_t)p);
}

inline int64 CastPtrToInt64(const void* p)
{
	return (int64)((uintptr_t)p);
}

//-----------------------------------------------------------------------------
// Stack-based allocation related helpers
//-----------------------------------------------------------------------------
#if defined( COMPILER_GCC ) || defined( COMPILER_SNC )

#define stackalloc( _size )		alloca( ALIGN_VALUE( _size, 16 ) )

#ifdef PLATFORM_OSX
#define mallocsize( _p )	( malloc_size( _p ) )
#else
#define mallocsize( _p )	( malloc_usable_size( _p ) )
#endif

#elif defined ( COMPILER_MSVC )

#define stackalloc( _size )		_alloca( ALIGN_VALUE( _size, 16 ) )
#define mallocsize( _p )		( _msize( _p ) )

#endif

#define NO_MALLOC_OVERRIDE

//-----------------------------------------------------------------------------
// Various compiler-specific keywords
//-----------------------------------------------------------------------------
#ifdef COMPILER_MSVC

#ifdef FORCEINLINE
#undef FORCEINLINE
#endif
#define STDCALL					__stdcall
#ifndef FASTCALL
#define  FASTCALL			__fastcall
#endif
#define FORCEINLINE				__forceinline
#define FORCEINLINE_TEMPLATE	__forceinline
#define NULLTERMINATED			__nullterminated

// This can be used to ensure the size of pointers to members when declaring
// a pointer type for a class that has only been forward declared
#define SINGLE_INHERITANCE		__single_inheritance
#define MULTIPLE_INHERITANCE	__multiple_inheritance
#define EXPLICIT				explicit
#define NO_VTABLE				__declspec( novtable )

// gcc doesn't allow storage specifiers on explicit template instatiation, but visual studio needs them to avoid link errors.
#define TEMPLATE_STATIC			static

// Used for dll exporting and importing
#define DLL_EXPORT				extern "C" __declspec( dllexport )
#define DLL_IMPORT				extern "C" __declspec( dllimport )

// Can't use extern "C" when DLL exporting a class
#define DLL_CLASS_EXPORT		__declspec( dllexport )
#define DLL_CLASS_IMPORT		__declspec( dllimport )

// Can't use extern "C" when DLL exporting a global
#define DLL_GLOBAL_EXPORT		extern __declspec( dllexport )
#define DLL_GLOBAL_IMPORT		extern __declspec( dllimport )

// Pass hints to the compiler to prevent it from generating unnessecary / stupid code
// in certain situations.  Several compilers other than MSVC also have an equivilent
// construct.
//
// Essentially the 'Hint' is that the condition specified is assumed to be true at
// that point in the compilation.  If '0' is passed, then the compiler assumes that
// any subsequent code in the same 'basic block' is unreachable, and thus usually
// removed.
#define HINT(THE_HINT)			__assume((THE_HINT))

// decls for aligning data
#define DECL_ALIGN(x)			__declspec( align( x ) )

// GCC had a few areas where it didn't construct objects in the same order 
// that Windows does. So when CVProfile::CVProfile() would access g_pMemAlloc,
// it would crash because the allocator wasn't initalized yet.
#define CONSTRUCT_EARLY

#define SELECTANY				__declspec(selectany)

#define RESTRICT				__restrict
#define RESTRICT_FUNC			__declspec(restrict)
#define FMTFUNCTION( a, b )
#define NOINLINE

#if !defined( NO_THREAD_LOCAL )
#define DECL_THREAD_LOCAL		__declspec(thread)
#endif 

#define DISABLE_VC_WARNING( x ) __pragma(warning(disable:4310) )
#define DEFAULT_VC_WARNING( x ) __pragma(warning(default:4310) )


#elif defined ( COMPILER_GCC ) || defined( COMPILER_SNC )

#if defined( COMPILER_SNC ) || defined( PLATFORM_64BITS )
#define  STDCALL
#define  __stdcall
#elif (CROSS_PLATFORM_VERSION >= 1) && !defined( PLATFORM_64BITS ) && !defined( COMPILER_PS3 )
#define  STDCALL			__attribute__ ((__stdcall__))
#else
#define  STDCALL
#define  __stdcall			__attribute__ ((__stdcall__))
#endif

#define  FASTCALL
#ifdef _LINUX_DEBUGGABLE
#define  FORCEINLINE
#else
#ifdef _PS3
	// [IESTYN 7/29/2010] As of SDK 3.4.0, this causes bad code generation in NET_Tick::ReadFromBuffer in netmessages.cpp,
	//                    which caused (seeming) random network packet corruption. It probably causes other bugs too.
#define  FORCEINLINE inline /* __attribute__ ((always_inline)) */
#else
#define  FORCEINLINE inline __attribute__ ((always_inline))
#endif
#endif

// GCC 3.4.1 has a bug in supporting forced inline of templated functions
// this macro lets us not force inlining in that case
#define FORCEINLINE_TEMPLATE	inline
#define SINGLE_INHERITANCE
#define MULTIPLE_INHERITANCE
#define EXPLICIT
#define NO_VTABLE

#define NULLTERMINATED			

#if defined( COMPILER_SNC )
#define TEMPLATE_STATIC static
#else
#define TEMPLATE_STATIC
#endif

	// Used for dll exporting and importing
#ifdef COMPILER_SNC
#define DLL_DECLARATION_DEFAULT_VISIBILITY 
#else
#define DLL_DECLARATION_DEFAULT_VISIBILITY __attribute__ ((visibility("default")))
#endif
#define DLL_EXPORT				extern "C" DLL_DECLARATION_DEFAULT_VISIBILITY 
#define DLL_IMPORT				extern "C" 

// Can't use extern "C" when DLL exporting a class
#if !defined( _PS3 ) && !defined( LINUX ) && !defined( PLATFORM_64BITS )
#define  __stdcall			__attribute__ ((__stdcall__))
#endif
#define DLL_CLASS_EXPORT		DLL_DECLARATION_DEFAULT_VISIBILITY 
#define DLL_CLASS_IMPORT

// Can't use extern "C" when DLL exporting a global
#define DLL_GLOBAL_EXPORT		DLL_DECLARATION_DEFAULT_VISIBILITY 
#define DLL_GLOBAL_IMPORT		extern

#define HINT(THE_HINT)			__builtin_expect( THE_HINT, 1 )
#define DECL_ALIGN(x)			__attribute__( ( aligned( x ) ) )
#define CONSTRUCT_EARLY			__attribute__((init_priority(101)))
#define SELECTANY				__attribute__((weak))
#if defined(__clang__)
	// [will] - clang is very strict about restrict, and we have a bunch of core functions that use the keyword which have issues with it.
	// This seemed to be a cleaner solution for now so we don't have to fill core code with tons of #ifdefs.
#define RESTRICT
#else
#define RESTRICT				__restrict__
#endif
#define RESTRICT_FUNC			RESTRICT_FUNC_NOT_YET_DEFINED_FOR_THIS_COMPILER
#define FMTFUNCTION( fmtargnumber, firstvarargnumber ) __attribute__ (( format( printf, fmtargnumber, firstvarargnumber )))
#define NOINLINE				__attribute__ ((noinline))

#if !defined( NO_THREAD_LOCAL )
#define DECL_THREAD_LOCAL		__thread
#endif

#define DISABLE_VC_WARNING( x )
#define DEFAULT_VC_WARNING( x )

#else

#define DECL_ALIGN(x)			/* */
#define SELECTANY				static

#endif

//-----------------------------------------------------------------------------
// DLL export for platform utilities
//-----------------------------------------------------------------------------
#ifndef STATIC_TIER0

#ifdef TIER0_DLL_EXPORT
#define PLATFORM_INTERFACE	DLL_EXPORT
#define PLATFORM_OVERLOAD	DLL_GLOBAL_EXPORT
#define PLATFORM_CLASS		DLL_CLASS_EXPORT
#else
#define PLATFORM_INTERFACE	DLL_IMPORT
#define PLATFORM_OVERLOAD	DLL_GLOBAL_IMPORT
#define PLATFORM_CLASS		DLL_CLASS_IMPORT
#endif

#else	// BUILD_AS_DLL

#define PLATFORM_INTERFACE	extern
#define PLATFORM_OVERLOAD
#define PLATFORM_CLASS

#endif	// BUILD_AS_DLL

//-----------------------------------------------------------------------------
// Processor Information:
//-----------------------------------------------------------------------------
struct CPUInformation
{
	int	 m_Size; // Size of this structure, for forward compatability.

	uint8_t m_nLogicalProcessors;  // Number op logical processors.
	uint8_t m_nPhysicalProcessors; // Number of physical processors

	bool m_bRDTSC : 1, // Is RDTSC supported?
		m_bCMOV   : 1, // Is CMOV supported?
		m_bFCMOV  : 1, // Is FCMOV supported?
		m_bSSE    : 1, // Is SSE supported?
		m_bSSE2   : 1, // Is SSE2 Supported?
		m_b3DNow  : 1, // Is 3DNow! Supported?
		m_bMMX    : 1, // Is MMX supported?
		m_bHT     : 1; // Is HyperThreading supported?


	bool m_bSSE3 : 1,
		m_bSSSE3 : 1,
		m_bSSE4a : 1,
		m_bSSE41 : 1,
		m_bSSE42 : 1,
		m_bAVX : 1;  // Is AVX supported?

	int64_t m_Speed;                    // In cycles per second.

	char* m_szProcessorID;              // Processor vendor Identification.
	char* m_szProcessorBrand;           // Processor brand string, if available

	uint32_t m_nModel;
	uint32_t m_nFeatures[3];
	uint32_t m_nL1CacheSizeKb;
	uint32_t m_nL1CacheDesc;
	uint32_t m_nL2CacheSizeKb;
	uint32_t m_nL2CacheDesc;
	uint32_t m_nL3CacheSizeKb;
	uint32_t m_nL3CacheDesc;

	CPUInformation() : m_Size(0)
	{
		m_Size = 0;

		m_nLogicalProcessors = 0;
		m_nPhysicalProcessors = 0;

		m_bRDTSC = false;
		m_bCMOV = false;
		m_bCMOV = false;
		m_bFCMOV = false;
		m_bSSE = false;
		m_bSSE2 = false;
		m_b3DNow = false;
		m_bMMX = false;
		m_bHT = false;

		m_Speed = 0i64;

		m_szProcessorID = nullptr;
		m_szProcessorBrand = nullptr;

		m_nModel = 0;
		m_nFeatures[0] = 0;
		m_nFeatures[1] = 0;
		m_nFeatures[2] = 0;
		m_nL1CacheSizeKb = 0;
		m_nL1CacheDesc = 0;
		m_nL2CacheSizeKb = 0;
		m_nL2CacheDesc = 0;
		m_nL3CacheSizeKb = 0;
		m_nL3CacheDesc = 0;
	}
};

#endif /* PLATFORM_H */