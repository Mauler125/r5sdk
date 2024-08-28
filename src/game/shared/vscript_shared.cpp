//=============================================================================//
//
// Purpose: Expose native code to VScript API
// 
//-----------------------------------------------------------------------------
// 
// Create functions here under the target VM namespace. If the function has to
// be registered for 2 or more VM's, put them under the 'SHARED' namespace. 
// Ifdef them out for 'SERVER_DLL' / 'CLIENT_DLL' if the target VM's do not 
// include 'SERVER' / 'CLIENT'.
//
//=============================================================================//

#include "core/stdafx.h"
#include "rtech/playlists/playlists.h"
#include "engine/client/cl_main.h"
#include "engine/cmodel_bsp.h"
#include "vscript/languages/squirrel_re/include/sqvm.h"
#include "vscript_shared.h"

namespace VScriptCode
{
    namespace Shared
    {
        //-----------------------------------------------------------------------------
        // Purpose: expose SDK version to the VScript API
        //-----------------------------------------------------------------------------
        SQRESULT GetSDKVersion(HSQUIRRELVM v)
        {
            sq_pushstring(v, SDK_VERSION, -1);
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }

        //-----------------------------------------------------------------------------
        // Purpose: return all available maps
        //-----------------------------------------------------------------------------
        SQRESULT GetAvailableMaps(HSQUIRRELVM v)
        {
            AUTO_LOCK(g_InstalledMapsMutex);

            if (g_InstalledMaps.IsEmpty())
                SCRIPT_CHECK_AND_RETURN(v, SQ_OK);

            sq_newarray(v, 0);

            FOR_EACH_VEC(g_InstalledMaps, i)
            {
                const CUtlString& mapName = g_InstalledMaps[i];

                sq_pushstring(v, mapName.String(), -1);
                sq_arrayappend(v, -2);
            }

            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }

        //-----------------------------------------------------------------------------
        // Purpose: return all available playlists
        //-----------------------------------------------------------------------------
        SQRESULT GetAvailablePlaylists(HSQUIRRELVM v)
        {
            if (g_vecAllPlaylists.IsEmpty())
                SCRIPT_CHECK_AND_RETURN(v, SQ_OK);

            sq_newarray(v, 0);
            for (const CUtlString& it : g_vecAllPlaylists)
            {
                sq_pushstring(v, it.String(), -1);
                sq_arrayappend(v, -2);
            }

            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }

        SQRESULT ScriptError(HSQUIRRELVM v)
        {
            SQChar* pString = NULL;
            SQInteger a4 = 0;

            if (SQVM_sprintf(v, 0, 1, &a4, &pString) < 0)
                SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);

            v_SQVM_ScriptError("%s", pString);
            SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
        }
    }
}

//---------------------------------------------------------------------------------
// Purpose: common script abstractions
// Input  : *s - 
//---------------------------------------------------------------------------------
void Script_RegisterCommonAbstractions(CSquirrelVM* s)
{
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, GetSDKVersion, "Gets the SDK version as a string", "string", "");

    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, GetAvailableMaps, "Gets an array of all available maps", "array< string >", "");
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, GetAvailablePlaylists, "Gets an array of all available playlists", "array< string >", "");

    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, ScriptError, "", "void", "string format, ...")
}

//---------------------------------------------------------------------------------
// Purpose: listen server constants
// Input  : *s - 
//---------------------------------------------------------------------------------
void Script_RegisterListenServerConstants(CSquirrelVM* s)
{
    const SQBool hasListenServer = !IsClientDLL();
    s->RegisterConstant("LISTEN_SERVER", hasListenServer);
}

//---------------------------------------------------------------------------------
// Purpose: server enums
// Input  : *s - 
//---------------------------------------------------------------------------------
void Script_RegisterCommonEnums_Server(CSquirrelVM* const s)
{
    v_Script_RegisterCommonEnums_Server(s);

    if (ServerScriptRegisterEnum_Callback)
        ServerScriptRegisterEnum_Callback(s);
}

//---------------------------------------------------------------------------------
// Purpose: client/ui enums
// Input  : *s - 
//---------------------------------------------------------------------------------
void Script_RegisterCommonEnums_Client(CSquirrelVM* const s)
{
    v_Script_RegisterCommonEnums_Client(s);

    const SQCONTEXT context = s->GetContext();

    if (context == SQCONTEXT::CLIENT && ClientScriptRegisterEnum_Callback)
        ClientScriptRegisterEnum_Callback(s);
    else if (context == SQCONTEXT::UI && UIScriptRegisterEnum_Callback)
        UIScriptRegisterEnum_Callback(s);
}

void VScriptShared::Detour(const bool bAttach) const
{
    DetourSetup(&v_Script_RegisterCommonEnums_Server, &Script_RegisterCommonEnums_Server, bAttach);
    DetourSetup(&v_Script_RegisterCommonEnums_Client, &Script_RegisterCommonEnums_Client, bAttach);
}


constexpr std::array<std::string_view, 583> KEYS{
    "GibEvent",
    "ProjectileTrail",
    "SoundDispatch",
    "UNAVAILABLE",
    "acceleration",
    "aiEnemy_priority",
    "aimAngleBackwardEnd",
    "aimAngleBackwardStart",
    "aimAngleForwardEnd",
    "aimAngleForwardStart",
    "aimassist_adspull_centerAttachmentName",
    "aimassist_adspull_centerRadius",
    "aimassist_adspull_headshotAttachmentName",
    "aimassist_adspull_headshotRadius",
    "aimassist_adspull_noPitchUp",
    "aimassist_bounds_override",
    "aimassist_use_short_inner_bounds",
    "airAcceleration",
    "airDrag",
    "airFriction",
    "airSlowMoSpeed",
    "airSpeed",
    "airStrafeAcceleration",
    "airStrafeEnabled",
    "airStrafeTaperFinish",
    "airStrafeTaperStart",
    "antiMultiJumpHeightFrac",
    "antiMultiJumpTimeMax",
    "antiMultiJumpTimeMin",
    "armorType",
    "armsModel",
    "automantle",
    "bodyModel",
    "boostAutoGlideMeterThreshold",
    "boostAutoJetpackMeterThreshold",
    "boostDelay",
    "boostDuration",
    "boostEnabled",
    "boostHorizontalDragAmount",
    "boostHorizontalDragDelay",
    "boostHorizontalDragSpeed",
    "boostOnGroundSafety",
    "boostRepeatedBoostMeterCost",
    "boostRepeatedBoostsAreShort",
    "boostThrust",
    "boostTightness",
    "camera_lcd",
    "camera_lcdStartup",
    "canJumpWhileCrouched",
    "chasecamDistanceMax",
    "chasecamMaxOrbitDepth",
    "chasecamMaxPitchUp",
    "chasecamOffsetForward",
    "chasecamOffsetRight",
    "chasecamOffsetUp",
    "class",
    "classActivityModifier",
    "climbAccel",
    "climbDecel",
    "climbEnabled",
    "climbFinalJumpUpHeight",
    "climbHeight",
    "climbHorzAccel",
    "climbHorzDecel",
    "climbHorzSpeed",
    "climbSpeedEnd",
    "climbSpeedStart",
    "cockpitModel",
    "cockpitSwayGain",
    "cockpitSwayMaxOriginY",
    "cockpitSwayMinAngleRoll",
    "cockpitSwayMinOriginY",
    "cockpitSwayMoveAngleFactor",
    "cockpitSwayMoveAngleRollFactor",
    "cockpitSwayMoveOriginFactor",
    "cockpitSwayTurnAngleFactor",
    "cockpitSwayTurnAngleRollFactor",
    "cockpitSwayTurnOriginFactor",
    "cockpit_spring_originConstant",
    "cockpit_spring_originDamping",
    "cockpit_spring_pitchConstant",
    "cockpit_spring_pitchDamping",
    "cockpit_spring_rollConstant",
    "cockpit_spring_rollDamping",
    "cockpit_spring_yawConstant",
    "cockpit_spring_yawDamping",
    "cockpit_stepJolt_angles",
    "cockpit_stepJolt_origin",
    "context_action_can_melee",
    "context_action_can_use",
    "crouch",
    "damageImpulseScale",
    "damageImpulseScaleAir",
    "damageImpulseSpeedBoostLimit",
    "damageImpulseSpeedLossLimit",
    "deathcamDistanceGrowRate",
    "deathcamDistanceMax",
    "deathcamDistanceMin",
    "deathcamExtraHeight",
    "deathcamLookdownPitch",
    "deathcamLookdownTime",
    "deathcamMinHeight",
    "deathcamRotateSpeed",
    "deceleration",
    "default",
    "description",
    "doFaceAnim",
    "dodge",
    "dodgeAnimDuration",
    "dodgeDuration",
    "dodgeHeight",
    "dodgeImpactTable",
    "dodgeInterval",
    "dodgeKeepSpeedFrac",
    "dodgePowerDelay",
    "dodgePowerDrain",
    "dodgeSpeed",
    "dodgeStopSpeed",
    "dodgeVerticalHeight",
    "doubleJump",
    "firstpersonproxyoffset",
    "footstepImpactTable",
    "footstepInterval",
    "footstepIntervalSprint",
    "footstepMinSpeed",
    "footstepRunSoundRadius",
    "footstepWalkSoundRadius",
    "footstep_type",
    "forceAimYawToEye",
    "fov",
    "friction",
    "fx_hover_enemy",
    "fx_hover_friendly",
    "fx_jetwash_enabled",
    "fx_jetwash_frequency",
    "fx_jetwash_height",
    "fx_jetwash_impactTable",
    "generalClass",
    "gibAngularSpeed",
    "gibAttachment0",
    "gibAttachment1",
    "gibAttachment2",
    "gibAttachment3",
    "gibAttachment4",
    "gibAttachments",
    "gibFX",
    "gibMaxDist",
    "gibModel0",
    "gibModel1",
    "gibModel2",
    "gibModel3",
    "gibModel4",
    "gibModels",
    "gibModelsSoftened",
    "gibSound",
    "gibSpeed",
    "glideDuration",
    "glideEnabled",
    "glideRechargeDelay",
    "glideRechargeOnlyWhenGrounded",
    "glideRechargeRate",
    "glideRedirectBelowLastJumpHeight",
    "glideRedirectDown",
    "glideRedirectFadeOutFinishSpeed",
    "glideRedirectFadeOutStartSpeed",
    "glideRedirectJumpHeightFinishOffset",
    "glideRedirectJumpHeightStartOffset",
    "glideRedirectSideways",
    "glideStrafe",
    "glideStrafeTaperFinish",
    "glideStrafeTaperStart",
    "glideThreshold",
    "glideThrust",
    "glideTightnessDown",
    "glideTightnessSideways",
    "glideTightnessUp",
    "grapple_airAccel",
    "grapple_airSpeedMax",
    "grapple_detachAwaySpeed",
    "grapple_detachGravityScale",
    "grapple_detachGravitySpeed",
    "grapple_detachGravityTime",
    "grapple_detachLengthMax",
    "grapple_detachLengthMin",
    "grapple_detachLowSpeedGroundTime",
    "grapple_detachLowSpeedMeleeThreshold",
    "grapple_detachLowSpeedSwingThreshold",
    "grapple_detachLowSpeedThreshold",
    "grapple_detachLowSpeedTime",
    "grapple_detachLowSpeedWallTime",
    "grapple_detachOnGrapple",
    "grapple_detachOnGrappleDebounceTime",
    "grapple_detachSpeedLoss",
    "grapple_detachSpeedLossMin",
    "grapple_detachVerticalBoost",
    "grapple_detachVerticalMaxSpeed",
    "grapple_disableWeaponsWhenActive",
    "grapple_gravityFracMax",
    "grapple_gravityFracMin",
    "grapple_impactVerticalBoost",
    "grapple_impactVerticalMaxSpeed",
    "grapple_power_regen_delay",
    "grapple_power_regen_rate",
    "grapple_rollDistanceMax",
    "grapple_rollDistanceMin",
    "grapple_rollMax",
    "grapple_rollStrength",
    "grapple_rollViewAngleMax",
    "grapple_rollViewAngleMin",
    "grapple_swing",
    "grapple_swingGravity",
    "grapple_swingHoldTime",
    "grapple_swingMinLength",
    "grapple_swingSticky",
    "grapple_swingType",
    "grapple_swingingSpeedRampMin",
    "grapple_touchGroundOnDetach",
    "gravityScale",
    "hardFallDist",
    "headshotFX",
    "health",
    "healthDoomed",
    "healthPerSegment",
    "healthShield",
    "healthpacks",
    "hoverEnabled",
    "hoverMeterRate",
    "hoverSafety",
    "hoverStrafeScale",
    "hoverTightnessHorizontal",
    "hoverTightnessVertical",
    "hull_max",
    "hull_min",
    "impactSpeed",
    "jetpackAfterburnerBoundToJump",
    "jetpackAfterburnerIsSticky",
    "jetpackAfterburnerMeterRate",
    "jetpackAfterburnerThrust",
    "jetpackAfterburnerTouchesGround",
    "jetpackBlendToGlideThreshold",
    "jetpackEnabled",
    "jetpackFloatLook",
    "jetpackHeightToActivate",
    "jetpackHeightToActivateDelay",
    "jetpackHorizontalMoveThrust",
    "jetpackJumpsToActivate",
    "jetpackMeterRate",
    "jetpackPitchMovement",
    "jetpackScriptToActivate",
    "jetpackStrafe",
    "jetpackStrafeTaperFinish",
    "jetpackStrafeTaperStart",
    "jetpackThrust",
    "jetpackTightnessDown",
    "jetpackTightnessForwardBack",
    "jetpackTightnessSideways",
    "jetpackTightnessUp",
    "jetpackToggleOff",
    "jetpackToggleOn",
    "jetpackZeroVerticalVelocityOnActivation",
    "jetpackZeroVerticalVelocityOnDeactivation",
    "jump",
    "jumpHeight",
    "landDistHigh",
    "landDistMedium",
    "landSlowdownDuration",
    "landSlowdownFrac",
    "landSlowdownHeightMax",
    "landSlowdownHeightMin",
    "landSlowdownNoSprintFrac",
    "landSlowdownTimePower",
    "landingImpactTable",
    "leech_range",
    "lowAcceleration",
    "lowSpeed",
    "magneticRange",
    "mantleAngleScale",
    "mantleDurationAbove",
    "mantleDurationBelow",
    "mantleDurationHigh",
    "mantleDurationLevel",
    "mantleHeight",
    "mantlePitchLeveling",
    "mechanical",
    "melee_cone_trace_angle",
    "melee_cone_trace_range",
    "modelScale",
    "passThroughThickness",
    "physicsIgnoreBelowMass",
    "physicsMass",
    "physicsPushMassLimit",
    "physicsPushSpeedLimit",
    "physicsSoftBelowMass",
    "pitchMaxDown",
    "pitchMaxUp",
    "pitchOffsetScale",
    "player_slideBoostCooldown",
    "player_slideBoostEnabled",
    "player_sound_fallingEffects_1p",
    "player_wallrunOneHanded",
    "poseSettings",
    "powerRegenRate",
    "printname",
    "quadMoveAnims",
    "rodeoViewdriftPitchSpeedMax",
    "rumbleOnDodge",
    "rumbleOnDoubleJump",
    "rumbleOnJump",
    "rumbleOnLandingHard",
    "rumbleOnLandingSoft",
    "rumbleOnSlideBegin",
    "rumbleOnWallHangStart",
    "rumbleOnWallrunStart",
    "rumbleOnWallrunTimeout",
    "rumbleOnZiplineAttach",
    "rumbleOnZiplineDetach",
    "sharedEnergyAllowOveruse",
    "sharedEnergyNotUsableSound",
    "sharedEnergyRegenDelay",
    "sharedEnergyRegenRate",
    "sharedEnergyRegenSound",
    "sharedEnergyTotal",
    "skydive_acceleration_max",
    "skydive_acceleration_min",
    "skydive_anticipate_acceleration_max",
    "skydive_anticipate_acceleration_min",
    "skydive_anticipate_deceleration_max",
    "skydive_anticipate_deceleration_min",
    "skydive_anticipate_forward_acceleration",
    "skydive_anticipate_forward_speed",
    "skydive_anticipate_speed",
    "skydive_anticipate_strafe_acceleration_max",
    "skydive_anticipate_strafe_acceleration_min",
    "skydive_deceleration_max",
    "skydive_deceleration_min",
    "skydive_finish_distance",
    "skydive_forward_pose_parameter_smooth_time",
    "skydive_glide_acceleration_max",
    "skydive_glide_angle",
    "skydive_glide_deceleration_max",
    "skydive_glide_deceleration_min",
    "skydive_glide_speed",
    "skydive_max_distance_from_ground",
    "skydive_max_distance_from_hover_tank",
    "skydive_max_distance_from_wall",
    "skydive_pitch_acceleration_max",
    "skydive_pitch_acceleration_min",
    "skydive_side_pose_parameter_smooth_time",
    "skydive_slipAcceleration",
    "skydive_slipSpeed",
    "skydive_speed_max",
    "skydive_speed_min",
    "skydive_speed_neutral",
    "skydive_strafe_acceleration_max",
    "skydive_strafe_acceleration_min",
    "skydive_strafe_angle",
    "skydive_yaw_smooth_time_max",
    "skydive_yaw_smooth_time_min",
    "slide",
    "slideAccel",
    "slideDecel",
    "slideEffectTable",
    "slideFOVLerpInTime",
    "slideFOVLerpOutTime",
    "slideFOVScale",
    "slideJumpHeight",
    "slideMaxJumpSpeed",
    "slideMaxStopSpeed",
    "slideRequiredStartSpeed",
    "slideRequiredStartSpeedAir",
    "slideSpeedBoost",
    "slideSpeedBoostCap",
    "slideStopSpeed",
    "slideVelocityDecay",
    "slideWantToStopDecel",
    "slipAcceleration",
    "slipAirRestrictDuration",
    "slipSpeed",
    "smartAmmoLockType",
    "sound_boost_body_1p",
    "sound_boost_body_3p",
    "sound_boost_finish_1p",
    "sound_boost_finish_3p",
    "sound_boost_meter_depleted_1p",
    "sound_boost_meter_depleted_3p",
    "sound_boost_meter_fail_1p",
    "sound_boost_meter_fail_3p",
    "sound_boost_meter_recharged_1p",
    "sound_boost_meter_recharged_3p",
    "sound_boost_short_1p",
    "sound_boost_short_3p",
    "sound_boost_start_1p",
    "sound_boost_start_3p",
    "sound_climb_body_1p",
    "sound_climb_body_3p",
    "sound_climb_finish_1p",
    "sound_climb_finish_3p",
    "sound_climb_start_1p",
    "sound_climb_start_3p",
    "sound_crouchToStand",
    "sound_crouchToStand_1p",
    "sound_crouchToStand_3p",
    "sound_dodgeFail",
    "sound_dodge_1p",
    "sound_dodge_3p",
    "sound_freefall_body_1p",
    "sound_freefall_body_3p",
    "sound_freefall_finish_1p",
    "sound_freefall_finish_3p",
    "sound_freefall_start_1p",
    "sound_freefall_start_3p",
    "sound_glide_body_1p",
    "sound_glide_body_3p",
    "sound_glide_finish_1p",
    "sound_glide_finish_3p",
    "sound_glide_start_1p",
    "sound_glide_start_3p",
    "sound_grapple_fire_1p",
    "sound_grapple_fire_3p",
    "sound_grapple_reel_in_target_3p",
    "sound_grapple_reset_1p",
    "sound_grapple_reset_3p",
    "sound_grapple_retract_1p",
    "sound_grapple_retract_3p",
    "sound_grapple_swing_1p",
    "sound_grapple_swing_3p",
    "sound_grapple_traverse_1p",
    "sound_grapple_traverse_3p",
    "sound_groundImpact",
    "sound_hover_body_1p",
    "sound_hover_body_3p",
    "sound_hover_finish_1p",
    "sound_hover_finish_3p",
    "sound_hover_start_1p",
    "sound_hover_start_3p",
    "sound_jetpack_afterburner_body_1p",
    "sound_jetpack_afterburner_body_3p",
    "sound_jetpack_afterburner_finish_1p",
    "sound_jetpack_afterburner_finish_3p",
    "sound_jetpack_afterburner_start_1p",
    "sound_jetpack_afterburner_start_3p",
    "sound_jetpack_body_1p",
    "sound_jetpack_body_3p",
    "sound_jetpack_finish_1p",
    "sound_jetpack_finish_3p",
    "sound_jetpack_start_1p",
    "sound_jetpack_start_3p",
    "sound_jetwash_body_1p",
    "sound_jetwash_body_3p",
    "sound_jetwash_finish_1p",
    "sound_jetwash_finish_3p",
    "sound_jetwash_start_1p",
    "sound_jetwash_start_3p",
    "sound_jumpjet_jet_body_1p",
    "sound_jumpjet_jet_body_3p",
    "sound_jumpjet_jet_finish_1p",
    "sound_jumpjet_jet_finish_3p",
    "sound_jumpjet_jet_start_1p",
    "sound_jumpjet_jet_start_3p",
    "sound_jumpjet_jump_body_1p",
    "sound_jumpjet_jump_body_3p",
    "sound_jumpjet_jump_finish_1p",
    "sound_jumpjet_jump_finish_3p",
    "sound_jumpjet_jump_start_1p",
    "sound_jumpjet_jump_start_3p",
    "sound_jumpjet_slide_start_1p",
    "sound_jumpjet_slide_start_3p",
    "sound_jumpjet_wallrun_body_1p",
    "sound_jumpjet_wallrun_body_3p",
    "sound_jumpjet_wallrun_finish_1p",
    "sound_jumpjet_wallrun_finish_3p",
    "sound_jumpjet_wallrun_start_1p",
    "sound_jumpjet_wallrun_start_3p",
    "sound_pain_layer1_end",
    "sound_pain_layer1_loop",
    "sound_pain_layer2_loop",
    "sound_pain_layer2_start",
    "sound_pain_layer3_end",
    "sound_pain_layer3_loop",
    "sound_slide_prefix",
    "sound_standToCrouch",
    "sound_standToCrouch_1p",
    "sound_standToCrouch_3p",
    "sound_superJump",
    "sound_superJumpFail",
    "sound_wallHangComplete",
    "sound_wallHangComplete_1p",
    "sound_wallHangComplete_3p",
    "sound_wallHangFall",
    "sound_wallHangSlip",
    "sound_wallHangStart",
    "sound_wallrunFall",
    "sound_wallrunFall_1p",
    "sound_wallrunFall_3p",
    "sound_wallrunImpact",
    "sound_wallrunImpact_1p",
    "sound_wallrunImpact_3p",
    "sound_wallrunSlip",
    "speed",
    "speedScaleBack",
    "speedScaleSide",
    "sprint",
    "sprintAcceleration",
    "sprintAnimOrientToMoveDir",
    "sprintDeceleration",
    "sprintEndDuration",
    "sprintStartDelay",
    "sprintStartDuration",
    "sprintStartFastDuration",
    "sprintViewOffset",
    "sprintspeed",
    "sprinttiltMaxRoll",
    "stealthSounds",
    "stepTestDist",
    "subclass",
    "superjumpCanUseAfterWallrun",
    "superjumpHorzSpeed",
    "superjumpLimit",
    "superjumpMaxHeight",
    "superjumpMinHeight",
    "swingRollDistanceMax",
    "swingRollMax",
    "swingRollStrength",
    "swingRollViewAngleMin",
    "titan_buddy",
    "titan_footstep_damage_dist_interval",
    "titan_footstep_damage_height_check_ratio",
    "titan_footstep_damage_interval",
    "titan_footstep_damage_min_speed",
    "ui_targetinfo",
    "val",
    "verticalGainCutoff_doubleJump",
    "verticalGainCutoff_wallrun",
    "viewPunchSpring",
    "viewheight",
    "viewkickFallDistMax",
    "viewkickFallDistMaxScale",
    "viewkickFallDistMin",
    "viewmodelDuckOffset",
    "viewmodelfov",
    "wallrun",
    "wallrunAccelerateHorizontal",
    "wallrunAccelerateVertical",
    "wallrunAdsType",
    "wallrunAllowedWallDistance",
    "wallrunAllowedWallDistanceWallHang",
    "wallrunCeilingLimit",
    "wallrunDecelerateHorizontal",
    "wallrunDecelerateVertical",
    "wallrunDuckCausesFallOff",
    "wallrunGravityRampUpTime",
    "wallrunJumpInputDirSpeed",
    "wallrunJumpOutwardSpeed",
    "wallrunJumpUpSpeed",
    "wallrunMaxSpeedHorizontal",
    "wallrunMaxSpeedHorizontalBackward",
    "wallrunMaxSpeedVertical",
    "wallrunSameWallAllowed",
    "wallrunSameWallHeight",
    "wallrunUpWallBoost",
    "wallrun_hangTimeLimit",
    "wallrun_timeLimit",
    "wallrun_viewpunch_onStart_basePitch",
    "wallrun_viewpunch_onStart_baseRoll",
    "wallrun_viewpunch_onStart_baseYaw",
    "wallrun_viewpunch_onStart_randomPitch",
    "wallrun_viewpunch_onStart_randomRoll",
    "wallrun_viewpunch_onStart_randomYaw",
    "wallrun_viewpunch_onTimeout_basePitch",
    "wallrun_viewpunch_onTimeout_baseRoll",
    "wallrun_viewpunch_onTimeout_baseYaw",
    "wallrun_viewpunch_onTimeout_randomPitch",
    "wallrun_viewpunch_onTimeout_randomRoll",
    "wallrun_viewpunch_onTimeout_randomYaw",
    "wallstickEnabled",
    "weaponClass",
    "ziplineCanMelee",
    "ziplineJumpOffSpeed",
    "ziplineRollStrength",
    "ziplineRollViewAngleMax",
    "ziplineRollViewAngleMin",
    "ziplineSpeed",
    "ziplineViewOffset",
};
bool iequals(std::string_view a, std::string_view b) {
    return std::equal(a.begin(), a.end(), b.begin(), b.end(),
        [](char a, char b) {
            return std::tolower(static_cast<unsigned char>(a)) ==
                std::tolower(static_cast<unsigned char>(b));
        });
}

bool isValidPlayerSettingsKeyName(std::string_view key) {
    for (const auto& validKey : KEYS) {
        if (iequals(key, validKey)) return true;
    }
    return false;
}

bool isValidPlayerSettingsKeyValue(std::string_view value) {
    size_t len = value.length();
    if (len == 0 || len > 64) return false;

    for (size_t i = 0; i < len; i++) {
        char c = value.at(i);
        if (!(isalnum(c) || c == '+' || c == '-' || c == '/' || c == '*' || c == '_' || c == '.' || c == ' ')) {
            return false;
        }
    }

    return true;
}
