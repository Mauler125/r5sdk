//=============================================================================//
//
// Purpose: AI navigation types
//
//=============================================================================//

#ifndef AI_NAVTYPES_H
#define AI_NAVTYPES_H

#if defined( _WIN32 )
#pragma once
#endif

// ---------------------------
//  Navigation Type
// ---------------------------
enum Navigation_e
{
	NAV_NONE   = -1,// error condition
	NAV_GROUND = 0, // walk/run
	NAV_JUMP,       // jump/leap
	NAV_FLY,        // can fly, move all around
	NAV_CLIMB,      // climb ladders
	NAV_CRAWL,      // crawl
	NAV_TRAVERSE,   // traverse
	NAV_WALLJUMP,   // walljump
	NAV_WALLRUN,    // wallrun
};

#endif // AI_NAVTYPES_H
