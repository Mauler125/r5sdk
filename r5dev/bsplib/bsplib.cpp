#include "core/stdafx.h"
#include "tier0/cvar.h"
#include "common/pseudodefs.h"
#include "bsplib/bsplib.h"

//-----------------------------------------------------------------------------
// Purpose: calculates the view frustum culling data per static prop
//-----------------------------------------------------------------------------
__int64 __fastcall HCalcPropStaticFrustumCulling(__int64 a1, __int64 a2, unsigned int a3, unsigned int a4, __int64 a5, __int64 a6, __int64 a7)
{
    //float v9; // xmm6_4
    //char v10; // r13
    //float v11; // xmm7_4
    //__int64 v12; // rsi
    //__int64 v13; // rdx
    //__int64 v14; // rbx
    //unsigned __int8 v15; // r8
    //const char* v16; // rax
    //int v17; // eax
    //char v18; // dl
    //bool v19; // cc
    //char v20; // al
    //__int16 v21; // ax
    //__int64 v22; // rax
    //__m128 v23; // xmm9
    //__m128 v24; // xmm10
    //__m128 v25; // xmm11
    //unsigned __int64 v26; // rbx
    //__m128 v27; // xmm0
    //__m128 v28; // xmm8
    //__int64 v29; // rax
    //__int64 v30; // rcx
    //__int64 v31; // xmm1_8
    //int v32; // er8
    //__m128 v33; // xmm0
    //__int64 v34; // rcx
    //unsigned __int64 v35; // rdx
    //__int64 v36; // rax
    //__int64 v37; // rax
    //unsigned __int64 v38; // rcx
    //__int64 v39; // rax
    //unsigned __int64 v40; // rcx
    //__int64 v41; // rax
    //float v42; // xmm2_4
    //__int64 v43; // rbx
    //float v44; // xmm2_4
    //float v45; // xmm4_4
    //__m128i v46; // xmm1
    //__m128i v47; // xmm2
    //__m128i v48; // xmm3
    //unsigned int v49; // eax
    //float v50; // xmm0_4
    //float v51; // xmm2_4
    //float v52; // xmm1_4
    //float v53; // xmm2_4
    //unsigned __int8 v54; // si
    //__int64 v55; // rax
    //__int64 v56; // rcx
    //int v57; // eax
    //__int64 v58; // rcx
    //int v59; // edx
    //__int64 v60; // rax
    //__int64 v61; // r13
    //int v62; // eax
    //unsigned __int64 v63; // r13
    //__int64 v64; // rdx
    //int v65; // er14
    //unsigned __int64 v66; // rdi
    //__int64 v67; // r13
    //__int64 v68; // r15
    //void** v69; // rbx
    //__int64 v70; // rcx
    //__int64 result; // rax
    //__m128 v72; // [rsp+38h] [rbp-D0h] BYREF
    //__int64 v73; // [rsp+48h] [rbp-C0h]
    //__int64 v74; // [rsp+50h] [rbp-B8h]
    //__m128 v75; // [rsp+58h] [rbp-B0h] BYREF
    //__int64 v76; // [rsp+68h] [rbp-A0h]
    //__int64 v77; // [rsp+78h] [rbp-90h]
    //__m128 v78[3]; // [rsp+88h] [rbp-80h] BYREF
    //char Destination[376]; // [rsp+B8h] [rbp-50h] BYREF
    //__int64 v80; // [rsp+278h] [rbp+170h]
    //int v83; // [rsp+288h] [rbp+180h]
    //int v84; // [rsp+290h] [rbp+188h]
    //__int64 v85; // [rsp+298h] [rbp+190h]
    //__int64 v86; // [rsp+2A8h] [rbp+1A0h]

    //v9 = 1.0;
    //v10 = a4;
    //*(_QWORD*)(a1 + 20) = *(_QWORD*)a5;
    //*(_DWORD*)(a1 + 28) = *(_DWORD*)(a5 + 8);
    //*(_DWORD*)(a1 + 8) = a4;
    //v11 = *(float*)(a5 + 24);
    //v12 = a4 >> 1;
    //*(float*)(a1 + 12) = 1.0 / (float)(v11 * v11);
    //v13 = *(unsigned __int16*)(a7 + 320);
    //*(_WORD*)a1 = v13;
    //v14 = (*(__int64(__fastcall**)(__int64, __int64, _QWORD))(*(_QWORD*)qword_14D40B328 + 104i64))(qword_14D40B328, v13, 0i64);
    //v85 = v14;
    //if ((*(_BYTE*)(v14 + 156) & 0x10) == 0 && dword_1696A9D20 < 100)
    //    ++dword_1696A9D20;
    //v15 = *(_BYTE*)(a5 + 30);
    //if (v15 > 2u && (unsigned __int8)(v15 - 6) > 2u)
    //{
    //    v16 = (const char*)(*((__int64(__fastcall**)(void**, __int64))g_CModelLoader + 4))(&g_CModelLoader, a7);
    //    strncpy_s(Destination, v16, 0x104ui64);
    //    v15 = 0;
    //}
    //v17 = *(unsigned __int8*)(a5 + 32);
    //*(_BYTE*)(a1 + 4) = v17;
    //v18 = v17;
    //v19 = v17 < *(_DWORD*)(v14 + 228);
    //*(_BYTE*)(a1 + 5) = v15;
    //if (!v19)
    //    v18 = 0;
    //*(_BYTE*)(a1 + 4) = v18;
    //v20 = *(_BYTE*)(a5 + 31);
    //if ((v20 & 4) != 0)
    //{
    //    v15 |= 0x40u;
    //    *(_BYTE*)(a1 + 5) = v15;
    //    v20 = *(_BYTE*)(a5 + 31);
    //}
    //if ((v20 & 8) != 0)
    //{
    //    v15 |= 0x20u;
    //    *(_BYTE*)(a1 + 5) = v15;
    //    v20 = *(_BYTE*)(a5 + 31);
    //}
    //if ((v20 & 0x30) != 16)
    //    *(_BYTE*)(a1 + 5) = v15 | 0x10;
    //v21 = 0;
    //if (*(_WORD*)(a5 + 34) != 0xFFFF)
    //    v21 = *(_WORD*)(a5 + 34);
    //*(_WORD*)(a1 + 2) = v21;
    //sub_1404365A0(v78, a5, a5 + 12, 0);
    //v22 = qword_141744EA8;
    //v23 = v78[0];
    //v24 = v78[1];
    //v25 = v78[2];
    //v26 = (unsigned __int64)(unsigned int)v12 << 6;
    //*(__m128*)(v26 + qword_141744EA8) = v78[0];
    //*(__m128*)(v26 + v22 + 16) = v24;
    //*(__m128*)(v26 + v22 + 32) = v25;
    //v27 = _mm_mul_ps(_mm_cvtepi32_ps(_mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(*(_DWORD*)(a5 + 52)), (__m128i)0i64), (__m128i)0i64)), (__m128)xmmword_1415BD270);
    //v75 = v27;
    //*(double*)v27.m128_u64 = _mm_castsi128_ps(sub_140270130(&v75));
    //v28 = v27;
    //*(__m128*)(v26 + qword_141744EA8 + 48) = v27;
    //((void(__fastcall*)(void***, __int64, __int64*, char*))off_141731448[10])(&off_141731448, a7, (long long*)&v72, (char*)&v72 + 12);
    //sub_14028F170((unsigned int)&v75, (unsigned int)&v75.m128_u32[3], (__m128*)v78, (__m128i*)&v72, (__m128i*)&v72 + 12);
    //v29 = qword_141744EA0;
    //v30 = 3 * v12;
    //v31 = v76;
    //*(__m128*)(qword_141744EA0 + 8 * v30) = v75;
    //*(_QWORD*)(v29 + 8 * v30 + 16) = v31;
    //if ((v10 & 1) != 0)
    //{
    //    v32 = dword_141744EBC;
    //    v33 = v72;
    //    *(_DWORD*)a2 = *(_DWORD*)(a6 + 48);
    //    *(_DWORD*)(a2 + 4) = *(_DWORD*)(a6 + 52);
    //    *(_QWORD*)(a2 + 8) = 0i64;
    //    v34 = 3i64 * (unsigned int)(v32 + v12);
    //    v35 = (unsigned __int64)(unsigned int)(v32 + v12) << 6;
    //    v36 = qword_141744EA0;
    //    *(__m128*)(qword_141744EA0 + 8 * v34) = v33;
    //    *(_QWORD*)(v36 + 8 * v34 + 16) = v73;
    //    v37 = qword_141744EA8;
    //    v38 = (unsigned __int64)(unsigned int)(v12 + 2 * v32) << 6;
    //    *(__m128*)(v35 + qword_141744EA8) = v23;
    //    *(__m128*)(v35 + v37 + 16) = v24;
    //    *(__m128*)(v35 + v37 + 32) = v25;
    //    *(__m128*)(v35 + qword_141744EA8 + 48) = v28;
    //    v39 = qword_141744EA8;
    //    *(__m128*)(v38 + qword_141744EA8) = v23;
    //    *(__m128*)(v38 + v39 + 16) = v24;
    //    *(__m128*)(v38 + v39 + 32) = v25;
    //    *(__m128*)(v38 + qword_141744EA8 + 48) = v28;
    //    v40 = (unsigned __int64)(unsigned int)(v32 + v12 + 2 * v32) << 6;
    //    *(__m128*)(v40 + qword_141744EA8 + 48) = v28;
    //    v41 = qword_141744EA8;
    //    *(__m128*)(v40 + qword_141744EA8) = *(__m128*)a6;
    //    *(__m128*)(v40 + v41 + 16) = *(__m128*)(a6 + 16);
    //    *(__m128*)(v40 + v41 + 32) = *(__m128*)(a6 + 32);
    //}
    //v42 = *(float*)(a5 + 36);
    //v43 = v85;
    //if (v42 <= 0.0)
    //{
    //    if ((*(_DWORD*)(v85 + 156) & 0x800) != 0)
    //    {
    //        v42 = 227023.36;
    //    }
    //    else
    //    {
    //        v44 = *(float*)(v85 + 364);
    //        if (v44 <= 0.0)
    //            v42 = fmaxf(
    //                (float)((float)(sqrtf(
    //                    (float)((float)((float)(*(float*)&v72 - *((float*)&v72 + 3)) * (float)(*(float*)&v72 - *((float*)&v72 + 3)))
    //                        + (float)((float)(*((float*)&v72 + 1) - *(float*)&v73) * (float)(*((float*)&v72 + 1) - *(float*)&v73)))
    //                    + (float)((float)(*((float*)&v72 + 2) - *((float*)&v73 + 1)) * (float)(*((float*)&v72 + 2) - *((float*)&v73 + 1))))
    //                    * 0.5)
    //                    * v11)
    //                * g_pCvar->FindVar("model_defaultFadeDistScale")->m_pParent->m_flValue,
    //                g_pCvar->FindVar("model_defaultFadeDistMin")->m_pParent->m_flValue);
    //        else
    //            v42 = v44 * v11;
    //    }
    //}
    //v45 = fmaxf(v42, 100.0);
    //*(float*)(a1 + 16) = v45 * v45;
    //v46 = (__m128i) * (unsigned int*)(v85 + 368);
    //if (*(float*)v46.m128i_i32 <= 0.0)
    //{
    //    LOWORD(v49) = 0;
    //}
    //else
    //{
    //    *(float*)v46.m128i_i32 = *(float*)v46.m128i_i32 * *(float*)v46.m128i_i32;
    //    v47 = v46;
    //    *(float*)v47.m128i_i32 = fmaxf(*(float*)v46.m128i_i32, 1.0004883);
    //    v48 = v47;
    //    *(float*)v48.m128i_i32 = fminf(*(float*)v47.m128i_i32, 4293918700.0);
    //    v49 = (unsigned int)(_mm_cvtsi128_si32(v48) - 1065351168) >> 12;
    //}
    //*(_WORD*)(a1 + 6) = v49;
    //v50 = *(float*)(qword_141744EA0 + 24 * ((unsigned __int64)*(unsigned int*)(a1 + 8) >> 1) + 8)
    //    - *(float*)(qword_141744EA0 + 24 * ((unsigned __int64)*(unsigned int*)(a1 + 8) >> 1) + 20);
    //v51 = *(float*)(qword_141744EA0 + 24 * ((unsigned __int64)*(unsigned int*)(a1 + 8) >> 1) + 4)
    //    - *(float*)(qword_141744EA0 + 24 * ((unsigned __int64)*(unsigned int*)(a1 + 8) >> 1) + 16);
    //v52 = *(float*)(qword_141744EA0 + 24 * ((unsigned __int64)*(unsigned int*)(a1 + 8) >> 1))
    //    - *(float*)(qword_141744EA0 + 24 * ((unsigned __int64)*(unsigned int*)(a1 + 8) >> 1) + 12);
    //v53 = (float)((float)(v51 * v51) + (float)(v52 * v52)) + (float)(v50 * v50);
    //if (v45 >= 227023.363449684)
    //    v9 = g_pCvar->FindVar("staticProp_no_fade_scalar")->m_pParent->m_flValue;
    //v54 = 0;
    //*(float*)(qword_141744E88 + 8i64 * a3) = v9 * (float)(1.0 / (float)(v53 * g_pCvar->FindVar("staticProp_gather_size_weight")->m_pParent->m_flValue));
    //v55 = qword_141744E88;
    //*(_BYTE*)(qword_141744E88 + 8i64 * a3 + 4) &= 0xFEu;
    //*(_BYTE*)(v55 + 8i64 * a3 + 4) |= v45 >= 227023.363449684;
    //v56 = *(_QWORD*)(*(__int64(__fastcall**)(__int64, _QWORD))(*(_QWORD*)qword_14D40B328 + 160i64))(qword_14D40B328, *(unsigned __int16*)(a7 + 320));
    //v57 = *(unsigned __int16*)(a5 + 32);
    //v77 = v56;
    //v58 = v85 + *(int*)(v85 + 232) + 2i64 * v57 * *(_DWORD*)(v85 + 224);
    //v59 = 0;
    //v86 = v58;
    //v84 = 0;
    //if (*(int*)(v85 + 236) <= 0)
    //    return 0i64;
    //v60 = 0i64;
    //v74 = 0i64;
    //do
    //{
    //    v61 = v60 + *(int*)(v43 + 240);
    //    v62 = 0;
    //    v63 = v43 + v61;
    //    v83 = 0;
    //    v75.m128_u64[0] = v63;
    //    if (*(int*)(v63 + 4) > 0)
    //    {
    //        v64 = 0i64;
    //        v80 = 0i64;
    //        do
    //        {
    //            v65 = 0;
    //            v66 = v63 + v64 + *(int*)(v63 + 12);
    //            if (*(int*)(v66 + 76) > 0)
    //            {
    //                v67 = v77;
    //                v68 = 0i64;
    //                do
    //                {
    //                    v69 = *(void***)(v67 + 8i64 * *(__int16*)(v58 + 2i64 * *(int*)(v68 + *(int*)(v66 + 80) + v66)));
    //                    if (!(*((unsigned __int8(__fastcall**)(void**)) * v69 + 86))(v69))
    //                    {
    //                        if (!(*((unsigned __int8(__fastcall**)(void**)) * v69 + 32))(v69) && (*((unsigned __int8(__fastcall**)(void**)) * v69 + 31))(v69))
    //                        {
    //                            v70 = 0i64;
    //                            if (dword_141744EE8)
    //                            {
    //                                while (off_141744E70[v70 + 16] != v69)
    //                                {
    //                                    v70 = (unsigned int)(v70 + 1);
    //                                    if ((unsigned int)v70 >= dword_141744EE8)
    //                                        goto LABEL_42;
    //                                }
    //                            }
    //                            else
    //                            {
    //                            LABEL_42:
    //                                off_141744E70[(unsigned int)dword_141744EE8++ + 16] = v69;
    //                            }
    //                        }
    //                        if ((*((unsigned __int8(__fastcall**)(void**)) * v69 + 21))(v69) && (*((unsigned int(__fastcall**)(void**, __int64)) * v69 + 18))(v69, 1i64))
    //                            *(_BYTE*)(a1 + 5) |= 0x80u;
    //                        if (!(*((unsigned __int8(__fastcall**)(void**)) * v69 + 48))(v69) && (*((unsigned int(__fastcall**)(void**, __int64)) * v69 + 18))(v69, 21844i64))
    //                            v54 |= 2u;
    //                        v54 |= (*((unsigned __int8(__fastcall**)(void**)) * v69 + 48))(v69) != 0;
    //                    }
    //                    v58 = v86;
    //                    ++v65;
    //                    v68 += 92i64;
    //                }           while (v65 < *(_DWORD*)(v66 + 76));
    //                v63 = v75.m128_u64[0];
    //                v62 = v83;
    //                v64 = v80;
    //            }
    //            ++v62;
    //            v64 += 136i64;
    //            v83 = v62;
    //            v80 = v64;
    //        }       while (v62 < *(_DWORD*)(v63 + 4));
    //        v43 = v85;
    //        v59 = v84;
    //    }
    //    ++v59;
    //    v60 = v74 + 16;
    //    v84 = v59;
    //    v74 += 16i64;
    //}   while (v59 < *(_DWORD*)(v43 + 236));
    //result = v54;
    //if (v54)
    //    *(_BYTE*)(a1 + 5) &= 0x7Fu;
    //return result;

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
