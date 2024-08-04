//===========================================================================//
//
// Purpose: 
//
//===========================================================================//
#include "gamepad.h"

static ConVar gamepad_use_per_scope_sensitivity_scalars("gamepad_use_per_scope_sensitivity_scalars", "0", FCVAR_ARCHIVE, "Gamepad uses the per scope scalars.");

static ConVar s_gamepadAdvancedAdsScalars[WeaponScopeZoomLevel_e::kScopeCount] = {
	{"gamepad_ads_advanced_sensitivity_scalar_0", "1.0", FCVAR_ARCHIVE, "Gamepad ads sensitivity for 1x scopes / iron sights.", true, 0.1f, true, 20.0f},
	{"gamepad_ads_advanced_sensitivity_scalar_1", "1.0", FCVAR_ARCHIVE, "Gamepad ads sensitivity for 2x scopes.", true, 0.1f, true, 20.0f},
	{"gamepad_ads_advanced_sensitivity_scalar_2", "1.0", FCVAR_ARCHIVE, "Gamepad ads sensitivity for 3x scopes.", true, 0.1f, true, 20.0f},
	{"gamepad_ads_advanced_sensitivity_scalar_3", "1.0", FCVAR_ARCHIVE, "Gamepad ads sensitivity for 4x scopes.", true, 0.1f, true, 20.0f},
	{"gamepad_ads_advanced_sensitivity_scalar_4", "1.0", FCVAR_ARCHIVE, "Gamepad ads sensitivity for 6x scopes.", true, 0.1f, true, 20.0f},
	{"gamepad_ads_advanced_sensitivity_scalar_5", "1.0", FCVAR_ARCHIVE, "Gamepad ads sensitivity for 8x scopes.", true, 0.1f, true, 20.0f},
	{"gamepad_ads_advanced_sensitivity_scalar_6", "1.0", FCVAR_ARCHIVE, "Gamepad ads sensitivity for 10x scopes.", true, 0.1f, true, 20.0f},
	{"gamepad_ads_advanced_sensitivity_scalar_7", "1.0", FCVAR_ARCHIVE, "Gamepad ads sensitivity for an unused scope.", true, 0.1f, true, 20.0f}
};

bool GamePad_UseAdvancedAdsScalarsPerScope()
{
	return gamepad_use_per_scope_sensitivity_scalars.GetBool();
}

float GamePad_GetAdvancedAdsScalarForOptic(const WeaponScopeZoomLevel_e opticType)
{
	Assert((int)opticType >= 0 && (int)opticType < V_ARRAYSIZE(s_gamepadAdvancedAdsScalars));
	return s_gamepadAdvancedAdsScalars[opticType].GetFloat();
}
