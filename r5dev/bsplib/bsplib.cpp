#include "core/stdafx.h"
#include "tier1/cvar.h"
#include "common/pseudodefs.h"
#include "bsplib/bsplib.h"
#include "engine/host_state.h"

//-----------------------------------------------------------------------------
// Purpose: calculates the view frustum culling data per static prop
//-----------------------------------------------------------------------------
__int64 __fastcall HCalcPropStaticFrustumCulling(__int64 a1, __int64 a2, unsigned int a3, unsigned int a4, __int64 a5, __int64 a6, __int64 a7)
{
    /*
    float v9; // xmm6_4
    char v10; // r13
    double v11; // xmm7_8
    __int64 v12; // rsi
    __int64 v13; // rdx
    __int64 v14; // rbx
    unsigned __int8 v15; // r8
    const char* v16; // rax
    int v17; // eax
    char v18; // dl
    bool v19; // cc
    char v20; // al
    __int16 v21; // ax
    __int64 v22; // rax
    __m128 v23; // xmm9
    __m128 v24; // xmm10
    __m128 v25; // xmm11
    unsigned __int64 v26; // rbx
    __m128 v27; // xmm8
    __int64 v28; // rax
    __int64 v29; // rcx
    __int64 v30; // xmm1_8
    int v31; // er8
    __m128i v32; // xmm0
    __int64 v33; // rcx
    unsigned __int64 v34; // rdx
    __int64 v35; // rax
    __int64 v36; // rax
    unsigned __int64 v37; // rcx
    __int64 v38; // rax
    unsigned __int64 v39; // rcx
    __int64 v40; // rax
    float v41; // xmm2_4
    __int64 v42; // rbx
    float v43; // xmm2_4
    float v44; // xmm4_4
    __m128i v45; // xmm1
    __m128i v46; // xmm2
    __m128i v47; // xmm3
    unsigned int v48; // eax
    float v49; // xmm0_4
    float v50; // xmm2_4
    float v51; // xmm1_4
    float v52; // xmm2_4
    unsigned __int8 v53; // si
    __int64 v54; // rax
    __int64 v55; // rcx
    int v56; // eax
    __int64 v57; // rcx
    int v58; // edx
    __int64 v59; // rax
    __int64 v60; // r13
    int v61; // eax
    unsigned __int64 v62; // r13
    __int64 v63; // rdx
    int v64; // er14
    unsigned __int64 v65; // rdi
    __int64 v66; // r13
    __int64 v67; // r15
    void* v68; // rbx
    __int64 v69; // rcx
    __int64 result; // rax
    __m128i v71; // [rsp+38h] [rbp-D0h] BYREF
    __int64 v72; // [rsp+48h] [rbp-C0h]
    __int64 v73; // [rsp+50h] [rbp-B8h]
    __m128 v74; // [rsp+58h] [rbp-B0h] BYREF
    __int64 v75; // [rsp+68h] [rbp-A0h]
    __int64 v76; // [rsp+78h] [rbp-90h]
    __m128 v77[3]; // [rsp+88h] [rbp-80h] BYREF
    char Destination[376]; // [rsp+B8h] [rbp-50h] BYREF
    __int64 v79; // [rsp+278h] [rbp+170h]
    int v82; // [rsp+288h] [rbp+180h]
    int v83; // [rsp+290h] [rbp+188h]
    __int64 v84; // [rsp+298h] [rbp+190h]
    __int64 v85; // [rsp+2A8h] [rbp+1A0h]

    static auto g_MdlCache = ADDRESS(0x14D40B328).RCast<void*>();
    static auto dword_1696A9D20 = *ADDRESS(0x1696A9D20).RCast<std::uint32_t*>();
    static auto sub_1404365A0 = ADDRESS(0x1404365A0).RCast<void**(*)(__m128*, const __m128i*, unsigned int*, double)>();
    static auto qword_141744EA8 = *ADDRESS(0x141744EA8).RCast<std::int64_t*>();
    static auto sub_140270130 = ADDRESS(0x140270130).RCast<__m128(*)(__m128*)>();
    static auto off_141731448 = ADDRESS(0x141731448).RCast<void*>();
    static auto sub_14028F170 = ADDRESS(0x14028F170).RCast<const __m128i* (*)(__int64, __int64, __m128*, const __m128i*, const __m128i*)>();
    static auto qword_141744EA0 = *ADDRESS(0x141744EA0).RCast<std::int64_t*>();
    static auto dword_141744EBC = *ADDRESS(0x141744EBC).RCast<std::int32_t*>();
    static auto qword_141744E88 = *ADDRESS(0x141744E88).RCast<std::int32_t*>();
    static auto dword_141744EE8 = *ADDRESS(0x141744EE8).RCast<std::int32_t*>();
    static auto off_141744E70 = ADDRESS(0x141744E70).RCast<void*>();
    static auto sub_1401E7900 = ADDRESS(0x1401E7900).RCast<__int64(*)(void*, unsigned __int16, __int64)>();
    static auto sub_140257F20 = ADDRESS(0x140257F20).RCast<__int64(*)(void*, __int64, __m128i*, __int8*)>();

    v9 = 1.0;
    v10 = a4;
    *(_QWORD*)(a1 + 20) = *(_QWORD*)a5;
    *(_DWORD*)(a1 + 28) = *(_DWORD*)(a5 + 8);
    *(_DWORD*)(a1 + 8) = a4;
    *(_QWORD*)&v11 = *(unsigned int*)(a5 + 24);
    v12 = a4 >> 1;
    *(float*)(a1 + 12) = 1.0 / (float)(*(float*)&v11 * *(float*)&v11);
    v13 = *(unsigned __int16*)(a7 + 320);
    *(_WORD*)a1 = v13;
//    v14 = (*(__int64(__fastcall**)(void*, __int64, _QWORD))(*(_QWORD*)g_MdlCache + 104i64))(g_MdlCache, v13, 0i64);
    v14 = sub_1401E7900(g_MdlCache, v13, 0i64);
    v84 = v14;
    if ((*(_BYTE*)(v14 + 156) & 0x10) == 0 && dword_1696A9D20 < 100)
        ++dword_1696A9D20;
    v15 = *(_BYTE*)(a5 + 30);
    if (v15 > 2u && (unsigned __int8)(v15 - 6) > 2u)
    {
        v16 = (const char*)(*((__int64(__fastcall**)(void**, __int64))g_CModelLoader + 4))(&g_CModelLoader, a7);
#pragma warning( push )
#pragma warning( disable : 4996)
        strncpy(Destination, v16, 0x104ui64);
#pragma warning( pop ) 
        v15 = 0;
    }
    v17 = *(unsigned __int8*)(a5 + 32);
    *(_BYTE*)(a1 + 4) = v17;
    v18 = v17;
    v19 = v17 < *(_DWORD*)(v14 + 228);
    *(_BYTE*)(a1 + 5) = v15;
    if (!v19)
        v18 = 0;
    *(_BYTE*)(a1 + 4) = v18;
    v20 = *(_BYTE*)(a5 + 31);
    if ((v20 & 4) != 0)
    {
        v15 |= 0x40u;
        *(_BYTE*)(a1 + 5) = v15;
        v20 = *(_BYTE*)(a5 + 31);
    }
    if ((v20 & 8) != 0)
    {
        v15 |= 0x20u;
        *(_BYTE*)(a1 + 5) = v15;
        v20 = *(_BYTE*)(a5 + 31);
    }
    if ((v20 & 0x30) != 16)
        *(_BYTE*)(a1 + 5) = v15 | 0x10;
    v21 = 0;
    if (*(_WORD*)(a5 + 34) != 0xFFFF)
        v21 = *(_WORD*)(a5 + 34);
    *(_WORD*)(a1 + 2) = v21;
    sub_1404365A0(v77, (const __m128i*)a5, (unsigned int*)(a5 + 12), v11);
    v22 = qword_141744EA8;
    v23 = v77[0];
    v24 = v77[1];
    v25 = v77[2];
    v26 = (unsigned __int64)(unsigned int)v12 << 6;
    *(__m128*)(v26 + qword_141744EA8) = v77[0];
    *(__m128*)(v26 + v22 + 16) = v24;
    *(__m128*)(v26 + v22 + 32) = v25;
    auto m1 = _mm_set_ps(0.003922, 0.003922, 0.003922, 0.003922);
    __m128i m2 = { 0 };
    __m128i m3 = { 0 };
    v74 = _mm_mul_ps(_mm_cvtepi32_ps(_mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(*(_DWORD*)(a5 + 52)), m2), m3)), m1);
    v27 = sub_140270130(&v74);
    *(__m128*)(v26 + qword_141744EA8 + 48) = v27;
    sub_140257F20(&off_141731448, a7, &v71, &v71.m128i_i8[12]);
    sub_14028F170((__int64)&v74, (__int64)&v74.m128_i64[1] + 4, v77, &v71, (const __m128i*) & v71.m128i_i8[12]);
    v72 = v71.m128i_i8[12]; // may be wrong
    v28 = qword_141744EA0;
    v29 = 3 * v12;
    v75 = (__int64)&v74.m128_i64[1] + 4; // may be wrong.
    v30 = v75;
    *(__m128*)(qword_141744EA0 + 8 * v29) = v74;
    *(_QWORD*)(v28 + 8 * v29 + 16) = v30;
    if ((v10 & 1) != 0)
    {
        v31 = dword_141744EBC;
        v32 = v71;
        *(_DWORD*)a2 = *(_DWORD*)(a6 + 48);
        *(_DWORD*)(a2 + 4) = *(_DWORD*)(a6 + 52);
        *(_QWORD*)(a2 + 8) = 0i64;
        v33 = 3i64 * (unsigned int)(v31 + v12);
        v34 = (unsigned __int64)(unsigned int)(v31 + v12) << 6;
        v35 = qword_141744EA0;
        *(__m128i*)(qword_141744EA0 + 8 * v33) = v32;
        *(_QWORD*)(v35 + 8 * v33 + 16) = v72;
        v36 = qword_141744EA8;
        v37 = (unsigned __int64)(unsigned int)(v12 + 2 * v31) << 6;
        *(__m128*)(v34 + qword_141744EA8) = v23;
        *(__m128*)(v34 + v36 + 16) = v24;
        *(__m128*)(v34 + v36 + 32) = v25;
        *(__m128*)(v34 + qword_141744EA8 + 48) = v27;
        v38 = qword_141744EA8;
        *(__m128*)(v37 + qword_141744EA8) = v23;
        *(__m128*)(v37 + v38 + 16) = v24;
        *(__m128*)(v37 + v38 + 32) = v25;
        *(__m128*)(v37 + qword_141744EA8 + 48) = v27;
        v39 = (unsigned __int64)(unsigned int)(v31 + v12 + 2 * v31) << 6;
        *(__m128*)(v39 + qword_141744EA8 + 48) = v27;
        v40 = qword_141744EA8;
        *(__m128*)(v39 + qword_141744EA8) = *(__m128*)a6; //*(_OWORD*)(v39 + qword_141744EA8) = *(_OWORD*)a6;
        *(__m128*)(v39 + v40 + 16) = *(__m128*)(a6 + 16); //*(_OWORD*)(v39 + v40 + 16) = *(_OWORD*)(a6 + 16);
        *(__m128*)(v39 + v40 + 32) = *(__m128*)(a6 + 32); //*(__m128*)(v40 + v41 + 32) = *(__m128*)(a6 + 32);
    }
    v41 = *(float*)(a5 + 36);
    v42 = v84;
    if (v41 <= 0.0)
    {
        if ((*(_DWORD*)(v84 + 156) & 0x800) != 0)
        {
            v41 = 227023.36;
        }
        else
        {
            v43 = *(float*)(v84 + 364);
            if (v43 <= 0.0)
                v41 = fmaxf(
                    (float)((float)(sqrt(
                        (float)((float)((float)(*(float*)v71.m128i_i32 - *(float*)&v71.m128i_i32[3]) * (float)(*(float*)v71.m128i_i32 - *(float*)&v71.m128i_i32[3]))
                            + (float)((float)(*(float*)&v71.m128i_i32[1] - *(float*)&v72) * (float)(*(float*)&v71.m128i_i32[1] - *(float*)&v72)))
                        + (float)((float)(*(float*)&v71.m128i_i32[2] - *((float*)&v72 + 1)) * (float)(*(float*)&v71.m128i_i32[2] - *((float*)&v72 + 1))))
                        * 0.5)
                        * *(float*)&v11)
                    * g_pCVar->FindVar("model_defaultFadeDistScale")->GetFloat(),
                    g_pCVar->FindVar("model_defaultFadeDistMin")->GetFloat());
            else
                v41 = v43 * *(float*)&v11;
        }
    }
    v44 = fmaxf(v41, 100.0);
    *(float*)(a1 + 16) = v44 * v44;
    v45 = _mm_loadu_si32((unsigned int*)(v84 + 368)); //v45 = (__m128i)*(unsigned int*)(v84 + 368);
    if (*(float*)v45.m128i_i32 <= 0.0)
    {
        LOWORD(v48) = 0;
    }
    else
    {
        *(float*)v45.m128i_i32 = *(float*)v45.m128i_i32 * *(float*)v45.m128i_i32;
        v46 = v45;
        *(float*)v46.m128i_i32 = fmaxf(*(float*)v45.m128i_i32, 1.0004883);
        v47 = v46;
        *(float*)v47.m128i_i32 = fminf(*(float*)v46.m128i_i32, 4293918700.0);
        v48 = (unsigned int)(_mm_cvtsi128_si32(v47) - 1065351168) >> 12;
    }
    *(_WORD*)(a1 + 6) = v48;
    v49 = *(float*)(qword_141744EA0 + 24 * ((unsigned __int64)*(unsigned int*)(a1 + 8) >> 1) + 8) - *(float*)(qword_141744EA0 + 24 * ((unsigned __int64)*(unsigned int*)(a1 + 8) >> 1) + 20);
    v50 = *(float*)(qword_141744EA0 + 24 * ((unsigned __int64)*(unsigned int*)(a1 + 8) >> 1) + 4) - *(float*)(qword_141744EA0 + 24 * ((unsigned __int64)*(unsigned int*)(a1 + 8) >> 1) + 16);
    v51 = *(float*)(qword_141744EA0 + 24 * ((unsigned __int64)*(unsigned int*)(a1 + 8) >> 1)) - *(float*)(qword_141744EA0 + 24 * ((unsigned __int64)*(unsigned int*)(a1 + 8) >> 1) + 12);
    v52 = (float)((float)(v50 * v50) + (float)(v51 * v51)) + (float)(v49 * v49);
    if (v44 >= 227023.363449684)
        v9 = g_pCVar->FindVar("staticProp_no_fade_scalar")->GetFloat();
    v53 = 0;
    *(float*)(qword_141744E88 + 8i64 * a3) = v9 * (float)(1.0 / (float)(v52 * g_pCVar->FindVar("staticProp_gather_size_weight")->GetFloat()));
    v54 = qword_141744E88;
    *(_BYTE*)(qword_141744E88 + 8i64 * a3 + 4) &= 0xFEu;
    *(_BYTE*)(v54 + 8i64 * a3 + 4) |= v44 >= 227023.363449684;
    v55 = *(_QWORD*)(*(__int64(__fastcall**)(void*, _QWORD))(*(_QWORD*)g_MdlCache + 160i64))(g_MdlCache, *(unsigned __int16*)(a7 + 320));
    v56 = *(unsigned __int16*)(a5 + 0x20);
    v76 = v55;
    v57 = v84 + *(int*)(v84 + 232) + 2i64 * v56 * *(_DWORD*)(v84 + 224);
    v58 = 0;
    v85 = v57;
    v83 = 0;
    if (*(int*)(v84 + 236) <= 0)
        return 0i64;
    v59 = 0i64;
    v73 = 0i64;
    do
    {
        v60 = v59 + *(int*)(v42 + 240);
        v61 = 0;
        v62 = v42 + v60;
        v82 = 0;
        v74.m128_u64[0] = v62;
        if (*(int*)(v62 + 4) > 0)
        {
            v63 = 0i64;
            v79 = 0i64;
            do
            {
                v64 = 0;
                v65 = v62 + v63 + *(int*)(v62 + 12);
                if (*(int*)(v65 + 76) > 0)
                {
                    v66 = v76;
                    v67 = 0i64;
                    do
                    {
                        v68 = *(void**)(v66 + 8i64 * *(__int16*)(v57 + 2i64 * *(int*)(v67 + *(int*)(v65 + 80) + v65)));
                        if (!(*(unsigned __int8(__fastcall**)(void*))(*(_QWORD*)v68 + 688i64))(v68))
                        {
                            if (!(*(unsigned __int8(__fastcall**)(void*))(*(_QWORD*)v68 + 256i64))(v68) && (*(unsigned __int8(__fastcall**)(void*))(*(_QWORD*)v68 + 248i64))(v68))
                            {
                                v69 = 0i64;
                                if (dword_141744EE8)
                                {
                                    while (*(&off_141744E70 + v69 + 16) != v68)
                                    {
                                        v69 = (unsigned int)(v69 + 1);
                                        if ((unsigned int)v69 >= dword_141744EE8)
                                            goto LABEL_42;
                                    }
                                }
                                else
                                {
                                LABEL_42:
                                    *(&off_141744E70 + (unsigned int)dword_141744EE8++ + 16) = v68;
                                }
                            }
                            if ((*(unsigned __int8(__fastcall**)(void*))(*(_QWORD*)v68 + 168i64))(v68) && (*(unsigned int(__fastcall**)(void*, __int64))(*(_QWORD*)v68 + 144i64))(v68, 1i64))
                                *(_BYTE*)(a1 + 5) |= 0x80u;
                            if (!(*(unsigned __int8(__fastcall**)(void*))(*(_QWORD*)v68 + 384i64))(v68) && (*(unsigned int(__fastcall**)(void*, __int64))(*(_QWORD*)v68 + 144i64))(v68, 21844i64))
                                v53 |= 2u;
                            v53 |= (*(unsigned __int8(__fastcall**)(void*))(*(_QWORD*)v68 + 384i64))(v68) != 0;
                        }
                        v57 = v85;
                        ++v64;
                        v67 += 92i64;
                    } while (v64 < *(_DWORD*)(v65 + 76));
                    v62 = v74.m128_u64[0];
                    v61 = v82;
                    v63 = v79;
                }
                ++v61;
                v63 += 136i64;
                v82 = v61;
                v79 = v63;
            } while (v61 < *(_DWORD*)(v62 + 4));
            v42 = v84;
            v58 = v83;
        }
        ++v58;
        v59 = v73 + 16;
        v83 = v58;
        v73 += 16i64;
    } while (v58 < *(_DWORD*)(v42 + 236));
    result = v53;
    if (v53)
        *(_BYTE*)(a1 + 5) &= 0x7Fu;
    return result;
    */

    return NULL;
}

void BspLib_Attach()
{
    DetourAttach((LPVOID*)&CalcPropStaticFrustumCulling, &HCalcPropStaticFrustumCulling);
}

void BspLib_Detach()
{
    DetourDetach((LPVOID*)&CalcPropStaticFrustumCulling, &HCalcPropStaticFrustumCulling);
}
