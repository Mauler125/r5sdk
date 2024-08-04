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

inline void (*GamePad_LoadAimAssistScripts)();

struct AimCurveConfig_s // Move to gamepad!
{
	int field_0;
	int field_4;
	int field_8;
	int field_C;
	int field_10;
	int field_14;
	char unknown[224];
};

inline float (*GamePad_CalcOuterDeadzoneCustom)(float a1);
inline float (*GamePad_CalcOuterDeadzone)(AimCurveConfig_s* curve, float a2);

inline char* g_lookSensitivity_TitanZoomed;
inline char* g_lookSensitivity_Titan;
inline char* g_lookSensitivity_Zoomed;
inline char* g_lookSensitivity;

inline AimCurveConfig_s** g_aimCurveConfig;

///////////////////////////////////////////////////////////////////////////////
class V_GamePad : public IDetour
{
	virtual void GetAdr(void) const
	{
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? 48 8B 5C 24 ?? 48 83 C4 ?? 5E").FollowNearCallSelf().GetPtr(GamePad_LoadAimAssistScripts);
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? EB ?? 49 63 C4").FollowNearCallSelf().GetPtr(GamePad_CalcOuterDeadzoneCustom);
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 0F 28 F0 F3 41 0F 5E F2").FollowNearCallSelf().GetPtr(GamePad_CalcOuterDeadzone);
	}
	virtual void GetVar(void) const
	{
		CMemory(GamePad_LoadAimAssistScripts).FindPatternSelf("48 8D").ResolveRelativeAddressSelf(3, 7).GetPtr(g_aimCurveConfig);

		CMemory(GamePad_LoadAimAssistScripts).OffsetSelf(0x60).FindPatternSelf("48 8D").ResolveRelativeAddressSelf(3, 7).GetPtr(g_lookSensitivity);
		CMemory(GamePad_LoadAimAssistScripts).OffsetSelf(0x77).FindPatternSelf("48 8D").ResolveRelativeAddressSelf(3, 7).GetPtr(g_lookSensitivity_Zoomed);
		CMemory(GamePad_LoadAimAssistScripts).OffsetSelf(0x8A).FindPatternSelf("48 8D").ResolveRelativeAddressSelf(3, 7).GetPtr(g_lookSensitivity_Titan);
		CMemory(GamePad_LoadAimAssistScripts).OffsetSelf(0x9D).FindPatternSelf("48 8D").ResolveRelativeAddressSelf(3, 7).GetPtr(g_lookSensitivity_TitanZoomed);
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { };
};
///////////////////////////////////////////////////////////////////////////////

#endif // CLIENT_GAMEPAD_H
