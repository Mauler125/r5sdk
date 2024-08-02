//=============================================================================//
//
// Purpose: Status Effect code and script definitions
//
//=============================================================================//
#ifndef STATUS_EFFECT_H
#define STATUS_EFFECT_H

struct StatusEffectTimedData
{
	void* __vftable;
	int seComboVars;
	float seTimeEnd;
	float seEaseOut;
	float sePausedTimeRemaining;
};

struct StatusEffectEndlessData
{
	void* __vftable;
	int seComboVars;
	char gap_c[4];
};

#endif // STATUS_EFFECT_H
