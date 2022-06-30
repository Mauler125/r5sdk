#pragma once

/*-----------------------------------------------------------------------------
 * _basetypes
 *-----------------------------------------------------------------------------*/

//#define GAMEDLL_S0 /*[r]*/
//#define GAMEDLL_S1 /*[r]*/
//#define GAMEDLL_S2 /*[i]*/
#define GAMEDLL_S3 /*[r]*/
//#define GAMEDLL_S4 /*[i]*/
//#define GAMEDLL_S7 /*[i]*/

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
#define IsLinux()	IsPlatformLinux() 
#define IsOSX()		IsPlatformOSX()
#define IsPosix()	IsPlatformPosix()
#define IsX360()	IsPlatformX360()
#define IsPS3()		IsPlatformPS3()

#define MAX_SPLITSCREEN_CLIENT_BITS 2 // Max 2 player splitscreen in portal (don't merge this back), saves a bunch of memory [8/31/2010 tom]
#define MAX_SPLITSCREEN_CLIENTS	( 1 << MAX_SPLITSCREEN_CLIENT_BITS ) // 4 // this should == MAX_JOYSTICKS in InputEnums.h

#define MAX_PLAYERS 128 // Max R5 players.

#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1) && !defined (GAMEDLL_S2)
#define MAX_MAP_NAME_HOST 64
#else
#define MAX_MAP_NAME_HOST 32
#endif // Max BSP file name len.
#define MAX_MAP_NAME 64

#define SDK_VERSION "VGameSDK001" // Increment this with every /breaking/ SDK change (i.e. security/backend changes breaking compatibility).
#define SDK_ARRAYSIZE(arr) ((int)(sizeof(arr) / sizeof(*arr))) // Name due to IMGUI implementation and NT implementation that we shouldn't share across everywhere.

#ifndef DEDICATED
#define SDK_DEFAULT_CFG "platform\\cfg\\startup_default.cfg"
#else
#define SDK_DEFAULT_CFG "platform\\cfg\\startup_dedi_default.cfg"
#endif

// #define COMPILETIME_MAX and COMPILETIME_MIN for max/min in constant expressions
#define COMPILETIME_MIN( a, b ) ( ( ( a ) < ( b ) ) ? ( a ) : ( b ) )
#define COMPILETIME_MAX( a, b ) ( ( ( a ) > ( b ) ) ? ( a ) : ( b ) )
#ifndef MIN
#define MIN( a, b ) ( ( ( a ) < ( b ) ) ? ( a ) : ( b ) )
#endif

#ifndef MAX
#define MAX( a, b ) ( ( ( a ) > ( b ) ) ? ( a ) : ( b ) )
#endif

#define FORWARD_DECLARE_HANDLE(name) typedef struct name##__ *name

#ifndef NOTE_UNUSED
#define NOTE_UNUSED(x)	(void)(x)	// for pesky compiler / lint warnings
#endif

typedef float				vec_t;
typedef float				vec3_t[3];

typedef float				float32;
typedef double				float64;

struct vrect_t
{
	int      x, y, width, height;
	vrect_t* pnext;
};

constexpr int MAX_NETCONSOLE_INPUT_LEN = 4096;
constexpr int MSG_NOSIGNAL             = 0;
