//=============================================================================//
//
// Purpose: Status Effect code and script definitions
//
//=============================================================================//
#ifndef STATUS_EFFECT_H
#define STATUS_EFFECT_H

struct StatusEffectTimedData
{
	char gap_0[8];
	int seComboVars;
	float seTimeEnd;
	float seEaseOut;
	float sePausedTimeRemaining;
};

struct StatusEffectEndlessData
{
	char gap_0[8];
	int seComboVars;
	char gap_c[4];
};

#endif // STATUS_EFFECT_H
