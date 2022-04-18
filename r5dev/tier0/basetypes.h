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

#define MAX_SPLITSCREEN_CLIENT_BITS 2 // Max 2 player splitscreen in portal (don't merge this back), saves a bunch of memory [8/31/2010 tom]
#define MAX_SPLITSCREEN_CLIENTS	( 1 << MAX_SPLITSCREEN_CLIENT_BITS ) // 4 // this should == MAX_JOYSTICKS in InputEnums.h

#define MAX_PLAYERS 128 // Max R5 players.

#define SDK_VERSION "beta 1.6" // Increment this with every /breaking/ SDK change (i.e. security/backend changes breaking compatibility).

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

struct vrect_t
{
	int      x, y, width, height;
	vrect_t* pnext;
};

constexpr int MAX_NETCONSOLE_INPUT_LEN = 4096;
constexpr int MSG_NOSIGNAL             = 0;
