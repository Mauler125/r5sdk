#include "input.h"
#include "edict.h"
#include "r1/c_weapon_x.h"
#include "c_player.h"
#include "cliententitylist.h"

bool C_Player::CheckMeleeWeapon()
{
    const C_EntInfo* pInfo = g_clientEntityList->GetEntInfoPtr(m_latestMeleeWeapon);
    const C_WeaponX* pWeapon = (C_WeaponX*)pInfo->m_pEntity;

    return (pInfo->m_SerialNumber == m_latestMeleeWeapon.GetSerialNumber())
        && (pWeapon != NULL)
        && *(float*)&pWeapon->m_modVars[600] > (float)(m_currentFramePlayer__timeBase - m_melee.attackLastHitNonWorldEntity);
}

void C_Player::CurveLook(C_Player* player, CInput::UserInput_t* input, float a3, float a4, float a5, int a6, float inputSampleFrametime, bool runAimAssist, JoyAngle_t* a9)
{
    float v11; // xmm7_4
    float v12; // xmm11_4
    float v13; // xmm14_4
    float v14; // xmm15_4
    bool isZoomed; // si
    C_WeaponX* activeWeapon; // rax
    float v17; // xmm0_4
    float v18; // xmm0_4
    __int64 v19; // xmm1_8
    bool m_bAutoAim_UnknownBool1AC; // r13
    bool m_bAutoAim_UnknownBool1AD; // r15
    bool m_bAutoAim_UnknownBool1B1; // r14
    vec_t z; // eax
    QAngle* v24; // rax
    bool v25; // zf
    QAngle v26; // xmm0_12
    QAngle* p_m_angUnknown1C8; // rax
    __int64 v28; // xmm0_8
    vec_t v29; // eax
    float inputSampleFrametime_c; // xmm13_4
    float v31; // xmm8_4
    int selectedGamePadLookCurve; // r12d
    float v33; // xmm7_4
    int customAimSpeed; // eax
    char v36; // r10
    int v37; // edx
    float* v38; // rsi
    char* v39; // rsi
    float v40; // xmm6_4
    float m_flUnknownFloat1B4; // xmm6_4
    float v42; // xmm0_4
    float v43; // xmm0_4
    float v44; // xmm10_4
    float v45; // xmm2_4
    float v46; // xmm6_4
    float v47; // xmm0_4
    float v48; // xmm7_4
    float v49; // xmm9_4
    float v50; // xmm6_4
    bool bZooming; // al
    bool v55; // cl
    float v56; // xmm1_4
    float v57; // xmm2_4
    float v58; // xmm0_4
    float v59; // xmm8_4
    float v60; // xmm9_4
    float v62; // xmm10_4
    float v63; // xmm0_4
    float v64; // xmm6_4
    JoyAngle_t* v65; // rsi
    float v66; // xmm7_4
    QAngle v68; // [rsp+68h] [rbp-A0h] BYREF
    QAngle v69; // [rsp+78h] [rbp-90h] BYREF
    Vector3D v70; // [rsp+88h] [rbp-80h] BYREF
    QAngle v71; // [rsp+178h] [rbp+70h] BYREF
    float m_flUnknownFloat1B8; // [rsp+188h] [rbp+80h]
    float v73; // [rsp+190h] [rbp+88h]

    v73 = a4;
    v11 = sub_1408A0600(player);
    v12 = 1.0f - v11;
    v13 = (float)(1.0f - v11) * a3;
    v14 = (float)(1.0f - v11) * a4;

    if (player->m_bZooming)
    {
        activeWeapon = C_BaseCombatCharacter__GetActiveWeapon(player);
        isZoomed = !activeWeapon || activeWeapon->m_modVars[3100];
    }
    else
    {
        isZoomed = false;
    }

    v17 = fabs(v13);
    if (v17 > 0.050000001 || (v18 = fabs(v14), v18 > 0.050000001))
        (*double_14D413928) = Plat_FloatTime();

    if (!runAimAssist)
        sub_1405B0E00(player, input);

    // NOTE: in the decompiler and disassembler, it appears that this
    // 'runAimAssist' param is always a bool, but this function below
    // indexes beyond the size of bool, just 2 bytes.. Looking at the
    // stack, there always seem to be space for it and nothing gets
    // smashed, nor does that area contain random data; the 4 bytes
    // are always free. Comparing this with the original code results
    // in identical results. Keeping it like this for now as even though
    // it looks off, it actually is correct.
    // Also, even though the function below does set 2 extra bools next
    // to the address of 'runAimAssist', only 'runAimAssist' is ever used
    // based on my hardware breakpoint tests. So its possible the function
    // below technically expects an array of bools or something but then
    // the original code only passes in the 'runAimAssist' which is stored
    // on the stack, and only having the below call work properly due to
    // stack alignment.
    sub_1405AD760(player, (unsigned char*)&runAimAssist);

    const bool gamePadCustomEnabled = gamepad_custom_enabled->GetBool();

    if (gamePadCustomEnabled && !gamepad_custom_assist_on->GetBool()
        || Plat_FloatTime() - (*double_14D4151B8) < 2.0
        || (*double_14D4151B8) > (*double_14D413928)
        || runAimAssist
        || (unsigned int)sub_14066D190(player)
        || C_Player__IsInTimeShift(player)
        || v11 > 0.050000001
        || !gamePadCustomEnabled && (unsigned int)C_Player__GetAimSpeed(player, isZoomed) == 7
        || (v19 = *(_QWORD*)&player->pl.lastPlayerView_angle.x, v70.z = player->pl.lastPlayerView_angle.z, *(_QWORD*)&v70.x = v19, *(float*)&v19 < -50.0)
        || sub_1405AD4E0(player) < FLT_EPSILON)
    {
        m_bAutoAim_UnknownBool1AC = 0;
        m_bAutoAim_UnknownBool1AD = 0;
        runAimAssist = 0;
        m_bAutoAim_UnknownBool1B1 = 0;
    }
    else
    {
        m_bAutoAim_UnknownBool1AC = input->m_bAutoAim_UnknownBool1AC;
        m_bAutoAim_UnknownBool1AD = input->m_bAutoAim_UnknownBool1AD;
        m_bAutoAim_UnknownBool1B1 = input->m_bAutoAim_UnknownBool1B1;
        runAimAssist = input->m_bAutoAim_UnknownBool1B0;
    }

    z = input->m_vecUnknown1BC.z;
    m_flUnknownFloat1B8 = input->m_flUnknownFloat1B8;
    *(_QWORD*)&v70.x = *(_QWORD*)&input->m_vecUnknown1BC.x;
    v70.z = z;
    v24 = sub_1406257E0(&v69, player);
    v25 = !input->m_bUnknown1D4;
    *(_QWORD*)&v26.x = *(_QWORD*)&v24->x;
    v68.z = v24->z;
    p_m_angUnknown1C8 = &input->m_angUnknown1C8;
    *(_QWORD*)&v68.x = *(_QWORD*)&v26.x;
    if (v25)
        p_m_angUnknown1C8 = &v68;

    v28 = *(_QWORD*)&p_m_angUnknown1C8->x;
    v29 = p_m_angUnknown1C8->z;
    inputSampleFrametime_c = inputSampleFrametime;
    *(_QWORD*)&v69.x = v28;
    v69.z = v29;
    if (m_bAutoAim_UnknownBool1AD && m_bAutoAim_UnknownBool1B1)
        input->m_flUnknownFloat1B4 = 0.0;
    else
        input->m_flUnknownFloat1B4 = inputSampleFrametime + input->m_flUnknownFloat1B4;

    v71.Init();
    v31 = 0.0f;
    sub_1405B03A0(input, player, &v71);
    selectedGamePadLookCurve = 1;
    v33 = sub_1405B0BC0(player, input, 0);
    a5 = sub_1405B0BC0(player, input, 1);
    customAimSpeed = C_Player__GetAimSpeed(player, isZoomed);
    v37 = *(_DWORD*)((unsigned int)(*dword_1423880E0) + *(_QWORD*)&player->gap_21a0[16]);

    bool v35 = m_bAutoAim_UnknownBool1B1; // r9

    if (m_bAutoAim_UnknownBool1AC && m_bAutoAim_UnknownBool1AD)
    {
        v36 = true;
    }
    else
    {
        v36 = false;

        if (!m_bAutoAim_UnknownBool1AC)
        {
            v35 = false;
        }
    }

    if (gamePadCustomEnabled)
    {
        sub_1405AEA10(nullptr, isZoomed, v37 == 1);
        v38 = dword_16A2A1640;
    }
    else
    {
        if (v37 == 1)
        {
            if (isZoomed)
                v39 = (char*)g_lookSensitivity_TitanZoomed;
            else
                v39 = (char*)g_lookSensitivity_Titan;
        }
        else
        {
            v25 = !isZoomed;
            v39 = (char*)g_lookSensitivity_Zoomed;
            if (v25)
                v39 = (char*)g_lookSensitivity;
        }

        v38 = (float*)&v39[276 * customAimSpeed];
    }

    if (v35)
    {
        v40 = 0.64999998f;
    }
    else
    {
        m_flUnknownFloat1B4 = input->m_flUnknownFloat1B4;
        if (m_flUnknownFloat1B4 <= 0.2)
        {
            if (m_flUnknownFloat1B4 > 0.1)
                v40 = (float)((float)((float)(m_flUnknownFloat1B4 - 0.1f) / 0.1f) * 0.35000002f) + 0.64999998f;
            else
                v40 = 0.64999998f;
        }
        else
        {
            v40 = 1.0f;
        }
    }

    v42 = m_flUnknownFloat1B8;
    sub_1405AF810(player, input, (__int64)v38, m_bAutoAim_UnknownBool1AC, v36, &v70, &v69, &v68, m_flUnknownFloat1B8);
    inputSampleFrametime = (float)((float)(v42 * (float)(1.0 - v40)) + v40) * (float)(1.0 - (float)(v33 * 0.94999999f));
    v43 = sqrtf((float)(a3 * a3) + (float)(v73 * v73));
    v44 = v43;

    const int gamePadLookCurve = gamepad_look_curve->GetInt();

    if (gamePadLookCurve <= 4u)
        selectedGamePadLookCurve = gamePadLookCurve;

    v45 = fabs((float)(v43 - 0.0f));
    if (v45 > 0.001f)
    {
        if (gamePadCustomEnabled)
            v47 = GamePad_CalcOuterDeadzoneCustom(v43);
        else
            v47 = GamePad_CalcOuterDeadzone(g_aimCurveConfig[selectedGamePadLookCurve], v43);

        v46 = v47 / v44;
    }
    else
    {
        v46 = 0.0;
    }

    v48 = v46 * v13;
    v49 = 0.0f;
    v50 = v46 * v14;

    if (v38[67] <= 0.0f
        || v38[65] == 0.0f && v38[64] == 0.0
        || m_bAutoAim_UnknownBool1AC
        || v44 < 0.99000001f
        || (unsigned int)(player->m_contextAction - 2) <= 1
        || ((player->m_melee.scriptedState - 3) & 0xFFFFFFFA) == 0
        || sub_1409DC4E0(player)
        || (player->m_latestMeleeWeapon.IsValid())
        || (player->CheckMeleeWeapon())
        || player->m_MoveType == MOVETYPE_TRAVERSE && !player->m_traversalType
        || (bZooming = input->m_bZooming, v55 = player->m_bZooming, input->m_bZooming = v55, bZooming) && !v55)
    {
        input->m_flSomeInputSampleFrameTime = 0.0f;
    }
    else
    {
        v56 = inputSampleFrametime_c + input->m_flSomeInputSampleFrameTime;
        input->m_flSomeInputSampleFrameTime = v56;
        v57 = v38[66];
        if (v57 <= v56)
        {
            v58 = GamePad_CalcOuterDeadzone((AimCurveConfig_s*)(v38 + 2), fminf(v56 - v57, v38[67]) / v38[67]);
            v31 = (float)(v58 * v48) * v38[65];
            v49 = (float)(v58 * v50) * v38[64];
        }
    }

    v59 = v31 * v12;
    v60 = v49 * v12;
    C_WeaponX* pWeapon = C_BaseCombatCharacter__GetActiveWeapon(player);
    if (pWeapon && C_Player__GetZoomFrac(player) >= 0.99000001 && pWeapon->m_modVars[412])
        v62 = *v38;
    else
        v62 = v38[1];

    float adsScalar = 1.0f;

    if (isZoomed && pWeapon && GamePad_UseAdvancedAdsScalarsPerScope())
    {
        const float interpAmount = pWeapon->HasTargetZoomFOV()
            ? pWeapon->GetZoomFOVInterpAmount(g_ClientGlobalVariables->exactCurTime)
            : 1.0f - pWeapon->GetZoomFOVInterpAmount(g_ClientGlobalVariables->exactCurTime);

        const float baseScalar1 = GamePad_GetAdvancedAdsScalarForOptic((WeaponScopeZoomLevel_e)pWeapon->m_modVars[0xA0C]);
        const float baseScalar2 = GamePad_GetAdvancedAdsScalarForOptic((WeaponScopeZoomLevel_e)pWeapon->m_modVars[0xA10]);

        adsScalar = ((baseScalar2 - baseScalar1) * interpAmount) + baseScalar1;
    }

    v63 = sub_1405D4300(player);
    v64 = (float)(v50 * inputSampleFrametime) * *v38;
    v65 = a9;
    v66 = (float)(v48 * inputSampleFrametime) * v62;
    a9->unk1 = v71.y;
    v65->unk2 = 0i64;

    const float pitchX = ((v66 * adsScalar) + v59) * v63;
    const float pitchY = ((v64 * adsScalar) + v60) * v63;

    v65->pitch.x = pitchX * inputSampleFrametime_c;
    v65->pitch.y = (pitchY * inputSampleFrametime_c) * -1.0f;
    v65->pitch.z = v71.x;

    if (m_bAutoAim_UnknownBool1AD && runAimAssist)
    {
        sub_1405AF1F0(input, player, (QAngle*)&v70, &v69, v14, v13, inputSampleFrametime_c, a5, (QAngle*)&a9);
        v65->unk2 = a9;
    }
}

void V_Player::Detour(const bool bAttach) const
{
    DetourSetup(&C_Player__CurveLook, C_Player::CurveLook, bAttach);
}
