#ifndef CLIENT_GAMEPAD_H
#define CLIENT_GAMEPAD_H

enum WeaponScopeZoomLevel_e // TODO: move to shared game scripts!
{
	kScope1X = 0,
	kScope2X,
	kScope3X,
	kScope4X,
	kScope6X,
	kScope8X,
	kScope10X,
	kScopeUnused,

	kScopeCount // NOTE: not a scope!
};

extern bool GamePad_UseAdvancedAdsScalarsPerScope();
extern float GamePad_GetAdvancedAdsScalarForOptic(const WeaponScopeZoomLevel_e opticType);


#endif // CLIENT_GAMEPAD_H
