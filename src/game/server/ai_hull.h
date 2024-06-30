//=============================================================================//
//
// Purpose: 
//
//=============================================================================//
#ifndef AI_HULL_H
#define AI_HULL_H

//=========================================================
// Link Properties. These hulls must correspond to the hulls
// in AI_Hull.cpp!
//=========================================================
enum Hull_t
{
	HULL_HUMAN,
	HULL_MEDIUM,
	HULL_FLYING_VEHICLE,
	HULL_SMALL,
	HULL_TITAN,
	HULL_GOLIATH,
	HULL_PROWLER,
	//--------------------------------------------
	NUM_HULLS,					// Confirmed, see [r5apex_ds + CB2765]
	HULL_NONE = -1				// No Hull (appears after num hulls as we don't want to count it)
};

#endif // AI_HULL_H
