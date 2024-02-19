#ifndef PLATFORM_H
#define PLATFORM_H

#if defined( _WIN32 ) && defined( _MSC_VER ) && ( _MSC_VER >= 1400 )
#pragma intrinsic(__rdtsc)
#endif

#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
//C++17 specific
#define SDK_HAS_CPP17 1
#define V_CONSTEXPR constexpr
#else
#define V_CONSTEXPR
#endif

#define TIER0_DLL_EXPORT

#ifdef _MSC_VER
#define COMPILER_MSVC 1 // !TODO: Set in CMake!
#endif // _MSC_VER

#ifdef __clang__
#define COMPILER_CLANG 1 // !TODO: Set in CMake!
#endif // __clang__

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

#define COMPILER_MSVC32 1
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

#ifdef COMPILER_CLANG
#define CLANG 1
#define POSIX_MATH 1
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

// VXConsole is enabled for...
#if defined(_X360) || defined(_PS3)
#define USE_VXCONSOLE 1
#define HasVxConsole() 1
#else
#define HasVxConsole() 0
#endif

//-----------------------------------------------------------------------------
// Set up platform type defines.
//-----------------------------------------------------------------------------
#if defined( PLATFORM_X360 ) || defined( _PS3 )
#ifndef _GAMECONSOLE
#define _GAMECONSOLE
#endif
#define IsPC()		0
#define IsGameConsole() 1
#else
#define IsPC()		1
#define IsGameConsole() 0
#endif

#if defined( _SGI_SOURCE ) || defined( PLATFORM_X360 ) || defined( _PS3 )
#define	PLAT_BIG_ENDIAN 1
#else
#define PLAT_LITTLE_ENDIAN 1
#endif

//-----------------------------------------------------------------------------
// Set up build configuration defines.
//-----------------------------------------------------------------------------
#ifdef _CERT
#define IsCert() 1
#else
#define IsCert() 0
#endif

#ifdef _DEBUG
#define IsRelease() 0
#define IsDebug() 1
#else
#define IsRelease() 1
#define IsDebug() 0
#endif

#ifdef _RETAIL
#define IsRetail() 1
#else
#define IsRetail() 0
#endif

#if defined( GNUC )	&& !defined( COMPILER_PS3 ) // use pre-align on PS3
// gnuc has the align decoration at the end
#define ALIGN4
#define ALIGN8 
#define ALIGN16
#define ALIGN32
#define ALIGN128
#define ALIGN_N( _align_ )

#undef ALIGN16_POST
#define ALIGN4_POST DECL_ALIGN(4)
#define ALIGN8_POST DECL_ALIGN(8)
#define ALIGN16_POST DECL_ALIGN(16)
#define ALIGN32_POST DECL_ALIGN(32)
#define ALIGN128_POST DECL_ALIGN(128)
#define ALIGN_N_POST( _align_ ) DECL_ALIGN( _align_ )
#else
// MSVC has the align at the start of the struct
// PS3 SNC supports both
#define ALIGN4 DECL_ALIGN(4)
#define ALIGN8 DECL_ALIGN(8)
#define ALIGN16 DECL_ALIGN(16)
#define ALIGN32 DECL_ALIGN(32)
#define ALIGN128 DECL_ALIGN(128)
#define ALIGN_N( _align_ ) DECL_ALIGN( _align_ )

#define ALIGN4_POST
#define ALIGN8_POST
#define ALIGN16_POST
#define ALIGN32_POST
#define ALIGN128_POST
#define ALIGN_N_POST( _align_ )
#endif

// !!! NOTE: if you get a compile error here, you are using VALIGNOF on an abstract type :NOTE !!!
#define VALIGNOF_PORTABLE( type ) ( sizeof( AlignOf_t<type> ) - sizeof( type ) )

#if defined( COMPILER_GCC ) || defined( COMPILER_MSVC )
#define VALIGNOF( type ) __alignof( type )
#define VALIGNOF_TEMPLATE_SAFE( type ) VALIGNOF_PORTABLE( type )
#else
#error "PORT: Code only tested with MSVC! Must validate with new compiler, and use built-in keyword if available."
#endif

// Use ValidateAlignment to sanity-check alignment usage when allocating arrays of an aligned type
#define ALIGN_ASSERT( pred ) { COMPILE_TIME_ASSERT( pred ); }
template< class T, int ALIGN >
inline void ValidateAlignmentExplicit(void)
{
	// Alignment must be a power of two
	ALIGN_ASSERT((ALIGN & (ALIGN - 1)) == 0);
	// Alignment must not imply gaps in the array (which the CUtlMemory pattern does not allow for)
	ALIGN_ASSERT(ALIGN <= sizeof(T));
	// Alignment must be a multiple of the size of the object type, or elements will *NOT* be aligned!
	ALIGN_ASSERT((sizeof(T) % ALIGN) == 0);
	// Alignment should be a multiple of the base alignment of T
//	ALIGN_ASSERT((ALIGN % VALIGNOF(T)) == 0);
}
template< class T > inline void ValidateAlignment(void) { ValidateAlignmentExplicit<T, VALIGNOF(T)>(); }

// Portable alternative to __alignof
template<class T> struct AlignOf_t { AlignOf_t() {} AlignOf_t& operator=(const AlignOf_t&) { return *this; } byte b; T t; };

template < size_t NUM, class T, int ALIGN > struct AlignedByteArrayExplicit_t {};
template < size_t NUM, class T > struct AlignedByteArray_t : public AlignedByteArrayExplicit_t< NUM, T, VALIGNOF_TEMPLATE_SAFE(T) > {};

//-----------------------------------------------------------------------------
// Macro to assist in asserting constant invariants during compilation

// This implementation of compile time assert has zero cost (so it can safely be
// included in release builds) and can be used at file scope or function scope.
#ifdef __GNUC__
#define COMPILE_TIME_ASSERT( pred ) typedef int UNIQUE_ID[ (pred) ? 1 : -1 ]
#else
#if _MSC_VER >= 1600
// If available use static_assert instead of weird language tricks. This
// leads to much more readable messages when compile time assert constraints
// are violated.
#define COMPILE_TIME_ASSERT( pred ) static_assert( pred, "Compile time assert constraint is not true: " #pred )
#else
// Due to gcc bugs this can in rare cases (some template functions) cause redeclaration
// errors when used multiple times in one scope. Fix by adding extra scoping.
#define COMPILE_TIME_ASSERT( pred ) typedef char compile_time_assert_type[(pred) ? 1 : -1];
#endif
#endif
// ASSERT_INVARIANT used to be needed in order to allow COMPILE_TIME_ASSERTs at global
// scope. However the new COMPILE_TIME_ASSERT macro supports that by default.
#define ASSERT_INVARIANT( pred )	COMPILE_TIME_ASSERT( pred )

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

#if defined( _MSC_VER )
#define OVERRIDE override
// warning C4481: nonstandard extension used: override specifier 'override'
#pragma warning(disable : 4481)
#elif defined( __clang__ )
#define OVERRIDE override
// warning: 'override' keyword is a C++11 extension [-Wc++11-extensions]
// Disabling this warning is less intrusive than enabling C++11 extensions
#pragma GCC diagnostic ignored "-Wc++11-extensions"
#else
#define OVERRIDE
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

#define MAX_FILEPATH 512 

// Defines MAX_PATH
#ifndef MAX_PATH
#define MAX_PATH  260
#endif

#ifdef _DEBUG
#define LEAKTRACK
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
void Plat_GetProcessUpTime(char* szBuf, size_t nSize);

inline bool Plat_IsInDebugSession()
{
#if defined( _X360 )
	return (XBX_IsDebuggerPresent() != 0);
#elif defined( _WIN32 )
	return (IsDebuggerPresent() != 0);
#elif defined( _PS3 ) && !defined(_CERT)
	return snIsDebuggerPresent();
#else
	return false;
#endif
}

inline void Plat_DebugString(const char* psz)
{
#if defined( _X360 )
	XBX_OutputDebugString(psz);
#elif defined( _WIN32 )
	::OutputDebugStringA(psz);
#elif defined(_PS3)
	printf("%s", psz);
#else 
	// do nothing?
#endif
}

#if defined( _X360 )
#define Plat_FastMemset XMemSet
#define Plat_FastMemcpy XMemCpy
#else
#define Plat_FastMemset memset
#define Plat_FastMemcpy memcpy
#endif

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

#define stackalloc_aligned( _size, _align )		(void*)( ( ((uintp)alloca( ALIGN_VALUE( ( _size ) + (_align ),  ( _align ) ) )) + ( _align ) ) & ~_align )

// We should probably always just align to 16 bytes, stackalloc just causes too many problems without this behavior. Source2 does it already.
// #define stackalloc( _size )							stackalloc_aligned( _size, 16 )

#define  stackfree( _p )			0
// two-argument ( type, #elements) stackalloc
#define StackAlloc( typ, nelements ) ( ( typ * )	stackalloc_aligned( ( nelements ) * sizeof(typ), 16 ) )

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

#define FORCENOINLINE __declspec(noinline)

// This can be used to ensure the size of pointers to members when declaring
// a pointer type for a class that has only been forward declared
#define SINGLE_INHERITANCE		__single_inheritance
#define MULTIPLE_INHERITANCE	__multiple_inheritance
#define EXPLICIT				explicit
#define NO_VTABLE				__declspec( novtable )

// gcc doesn't allow storage specifiers on explicit template instantiation, but visual studio needs them to avoid link errors.
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

// Pass hints to the compiler to prevent it from generating unnecessary / stupid code
// in certain situations.  Several compilers other than MSVC also have an equivalent
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
// it would crash because the allocator wasn't initialized yet.
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
// C++11 helpers
//-----------------------------------------------------------------------------
#define VALVE_CPP11 1

#if VALVE_CPP11
template <class T> struct C11RemoveReference { typedef T Type; };
template <class T> struct C11RemoveReference<T&> { typedef T Type; };
template <class T> struct C11RemoveReference<T&&> { typedef T Type; };

template <class T>
inline typename C11RemoveReference<T>::Type&& Move(T&& obj)
{
	return static_cast<typename C11RemoveReference<T>::Type&&>(obj);
}

template <class T>
inline T&& Forward(typename C11RemoveReference<T>::Type& obj)
{
	return static_cast<T&&>(obj);
}

template <class T>
inline T&& Forward(typename C11RemoveReference<T>::Type&& obj)
{
	return static_cast<T&&>(obj);
}
#endif

//-----------------------------------------------------------------------------
// Methods to invoke the constructor, copy constructor, and destructor
//-----------------------------------------------------------------------------

template <class T>
inline T* Construct(T* pMemory)
{
	return ::new(pMemory) T;
}

template <class T, typename ARG1>
inline T* Construct(T* pMemory, ARG1 a1)
{
	return ::new(pMemory) T(a1);
}

template <class T, typename ARG1, typename ARG2>
inline T* Construct(T* pMemory, ARG1 a1, ARG2 a2)
{
	return ::new(pMemory) T(a1, a2);
}

template <class T, typename ARG1, typename ARG2, typename ARG3>
inline T* Construct(T* pMemory, ARG1 a1, ARG2 a2, ARG3 a3)
{
	return ::new(pMemory) T(a1, a2, a3);
}

template <class T, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
inline T* Construct(T* pMemory, ARG1 a1, ARG2 a2, ARG3 a3, ARG4 a4)
{
	return ::new(pMemory) T(a1, a2, a3, a4);
}

template <class T, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
inline T* Construct(T* pMemory, ARG1 a1, ARG2 a2, ARG3 a3, ARG4 a4, ARG5 a5)
{
	return ::new(pMemory) T(a1, a2, a3, a4, a5);
}

template <class T>
inline T* CopyConstruct(T* pMemory, T const& src)
{
	return ::new(pMemory) T(src);
}

template <class T>
inline T* MoveConstruct(T* pMemory, T&& src)
{
	return ::new(pMemory) T(Move(src));
}

// [will] - Fixing a clang compile: unable to create a pseudo-destructor (aka a destructor that does nothing) for float __attribute__((__vector_size__(16)))
// Fixed by specializing the Destroy function to not call destructor for that type.
#if defined( __clang__ ) || defined (LINUX)

template <class T>
inline void Destruct(T* pMemory);

template <>
inline void Destruct(float __attribute__((__vector_size__(16)))* pMemory);

#endif // __clang__

template <class T>
inline void Destruct(T* pMemory)
{
	pMemory->~T();

#ifdef _DEBUG
	memset(pMemory, 0xDD, sizeof(T));
#endif
}

// [will] - Fixing a clang compile: unable to create a pseudo-destructor (aka a destructor that does nothing) for float __attribute__((__vector_size__(16)))
// Fixed by specializing the Destroy function to not call destructor for that type.
#if defined( __clang__ ) || defined (LINUX)

template <>
inline void Destruct(float __attribute__((__vector_size__(16)))* pMemory)
{
#ifdef _DEBUG
	memset(pMemory, 0xDD, sizeof(float __attribute__((__vector_size__(16)))));
#endif
}

#endif // __clang__

#endif /* PLATFORM_H */