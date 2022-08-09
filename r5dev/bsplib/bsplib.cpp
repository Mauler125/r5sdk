#include "core/stdafx.h"
#include "tier1/cvar.h"
#include "datacache/mdlcache.h"
#include "common/pseudodefs.h"
#include "materialsystem/cmaterialglue.h"
#include "engine/host_state.h"
#include "engine/modelloader.h"
#include "bsplib/bsplib.h"

//void* __fastcall BuildPropStaticFrustumCullMap(int64_t a1, int64_t a2, unsigned int a3, unsigned int a4, int64_t a5, int64_t a6, int64_t a7)
//{
//    if (staticProp_defaultBuildFrustum->GetBool())
//        return v_BuildPropStaticFrustumCullMap(a1, a2, a3, a4, a5, a6, a7);
//
//    float v9; // xmm6_4
//    char v10; // r13
//    double v11; // xmm7_8
//    __int64 v12; // rsi
//    MDLHandle_t mdlhandle; // dx
//    studiohdr_t* studio; // rbx
//    unsigned __int8 v15; // r8
//    const char* v16; // rax
//    int v17; // eax
//    char v18; // dl
//    bool v19; // cc
//    char v20; // al
//    __int16 v21; // ax
//    __int64 v22; // rax
//    __m128 v23; // xmm9
//    __m128 v24; // xmm10
//    __m128 v25; // xmm11
//    unsigned __int64 v26; // rbx
//    __m128 v27; // xmm8
//    __int64 v28; // rax
//    __int64 v29; // rcx
//    __int64 v30; // xmm1_8
//    int v31; // er8
//    __m128i v32; // xmm0
//    __int64 v33; // rcx
//    unsigned __int64 v34; // rdx
//    __int64 v35; // rax
//    __int64 v36; // rax
//    unsigned __int64 v37; // rcx
//    __int64 v38; // rax
//    unsigned __int64 v39; // rcx
//    __int64 v40; // rax
//    float v41; // xmm2_4
//    studiohdr_t* v42; // rbx
//    float v43; // xmm2_4
//    float v44; // xmm4_4
//    __m128i v45; // xmm1
//    __m128i v46; // xmm2
//    __m128i v47; // xmm3
//    unsigned int v48; // eax
//    float v49; // xmm0_4
//    float v50; // xmm2_4
//    float v51; // xmm1_4
//    float v52; // xmm2_4
//    unsigned __int8 v53; // si
//    __int64 v54; // rax
//    __int64 v55; // rcx
//    int v56; // eax
//    __int64 v57; // rcx
//    int v58; // edx
//    __int64 v59; // rax
//    __int64 v60; // r13
//    int v61; // eax
//    char* v62; // r13
//    __int64 v63; // rdx
//    int v64; // er14
//    char* v65; // rdi
//    __int64 v66; // r13
//    __int64 v67; // r15
//    void* v68; // rbx
//    int32_t v69; // rcx
//    void* result; // rax
//    __m128i v71; // [rsp+38h] [rbp-D0h] BYREF
//    __int64 v72{}; // [rsp+48h] [rbp-C0h]
//    __int64 v73; // [rsp+50h] [rbp-B8h]
//    __m128 v74; // [rsp+58h] [rbp-B0h] BYREF
//    __int64 v75{}; // [rsp+68h] [rbp-A0h]
//    __int64 v76; // [rsp+78h] [rbp-90h]
//    __m128 v77[3]; // [rsp+88h] [rbp-80h] BYREF
//    char Destination[376]; // [rsp+B8h] [rbp-50h] BYREF
//    __int64 v79; // [rsp+278h] [rbp+170h]
//    int v82; // [rsp+288h] [rbp+180h]
//    int v83; // [rsp+290h] [rbp+188h]
//    studiohdr_t* v84; // [rsp+298h] [rbp+190h]
//    __int64 v85; // [rsp+2A8h] [rbp+1A0h]
//
//    v9 = 1.0f;
//    v10 = a4;
//    *(_QWORD*)(a1 + 20) = *(_QWORD*)a5;
//    *(_DWORD*)(a1 + 28) = *(_DWORD*)(a5 + 8);
//    *(_DWORD*)(a1 + 8) = a4;
//    *(_QWORD*)&v11 = *(unsigned int*)(a5 + 24);
//    v12 = a4 >> 1;
//    *(float*)(a1 + 12) = 1.0f / (float)(*(float*)&v11 * *(float*)&v11);
//    mdlhandle = *(unsigned __int16*)(a7 + 0x140);
//    *(MDLHandle_t*)a1 = mdlhandle;
//    studio = CMDLCache::FindMDL(g_MDLCache, mdlhandle, nullptr);
//    v84 = studio;
//    if ((studio->flags & STUDIOHDR_FLAGS_STATIC_PROP) == 0 && (*dword_1696A9D20) < 0x64)
//        ++(*dword_1696A9D20);
//    v15 = *(_BYTE*)(a5 + 30);
//    if (v15 > 2u && (unsigned __int8)(v15 - 6) > 2u)
//    {
//        v16 = (const char*)(*((__int64(__fastcall**)(CModelLoader**, __int64))g_pModelLoader + 4))(&g_pModelLoader, a7);
//        strncpy(Destination, v16, 0x104ui64);
//        v15 = 0;
//    }
//    v17 = *(unsigned __int8*)(a5 + 32);
//    *(_BYTE*)(a1 + 4) = v17;
//    v18 = v17;
//    v19 = v17 < studio->numskinfamilies;
//    *(_BYTE*)(a1 + 5) = v15;
//    if (!v19)
//        v18 = 0;
//    *(_BYTE*)(a1 + 4) = v18;
//    v20 = *(_BYTE*)(a5 + 31);
//    if ((v20 & 4) != 0)
//    {
//        v15 |= 0x40u;
//        *(_BYTE*)(a1 + 5) = v15;
//        v20 = *(_BYTE*)(a5 + 31);
//    }
//    if ((v20 & 8) != 0)
//    {
//        v15 |= 0x20u;
//        *(_BYTE*)(a1 + 5) = v15;
//        v20 = *(_BYTE*)(a5 + 31);
//    }
//    if ((v20 & 0x30) != 16)
//        *(_BYTE*)(a1 + 5) = v15 | 0x10;
//    v21 = 0;
//    if (*(_WORD*)(a5 + 34) != 0xFFFF)
//        v21 = *(_WORD*)(a5 + 34);
//    *(_WORD*)(a1 + 2) = v21;
//    sub_1404365A0(v77, (const __m128i*)a5, (__m128i*)(a5 + 12), v11);
//    v22 = *qword_141744EA8;
//    v23 = v77[0];
//    v24 = v77[1];
//    v25 = v77[2];
//    v26 = (unsigned __int64)(unsigned int)v12 << 6;
//    *(__m128*)(v26 + (*qword_141744EA8)) = v77[0];
//    *(__m128*)(v26 + v22 + 16) = v24;
//    *(__m128*)(v26 + v22 + 32) = v25;
//    __m128i m2 = { 0 };
//    __m128i m3 = { 0 };
//    v74 = _mm_mul_ps(_mm_cvtepi32_ps(_mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(*(_DWORD*)(a5 + 52)), m2), m3)), (*xmmword_1415BD270));
//    v27 = sub_140270130(&v74);
//    *(__m128*)(v26 + (*qword_141744EA8) + 48) = v27;// DIFF 1
//    (*((void(__fastcall**)(void**, __int64, __m128i*, __int8*)) * (void**)off_141731448 + 10))(&*(void**)off_141731448, a7, &v71, &v71.m128i_i8[12]);
//    sub_14028F170((__int64)&v74, (__int64)&v74.m128_i64[1] + 4, v77, &v71, (const __m128i*) & v71.m128i_i8[12]);
//    v28 = *qword_141744EA0;
//    v29 = 3 * v12;
//    v30 = v75;
//    *(__m128*)((*qword_141744EA0) + 8 * v29) = v74;
//    *(_QWORD*)(v28 + 8 * v29 + 16) = v30;
//    if ((v10 & 1) != 0)
//    {
//        v31 = *dword_141744EBC;
//        v32 = v71;
//        *(_DWORD*)a2 = *(_DWORD*)(a6 + 48);
//        *(_DWORD*)(a2 + 4) = *(_DWORD*)(a6 + 52);
//        *(_QWORD*)(a2 + 8) = 0i64;
//        v33 = 3i64 * (unsigned int)(v31 + v12);
//        v34 = (unsigned __int64)(unsigned int)(v31 + v12) << 6;
//        v35 = *qword_141744EA0;
//        *(__m128i*)((*qword_141744EA0) + 8 * v33) = v32;
//        *(_QWORD*)(v35 + 8 * v33 + 16) = v72;
//        v36 = *qword_141744EA8;
//        v37 = (unsigned __int64)(unsigned int)(v12 + 2 * v31) << 6;
//        *(__m128*)(v34 + (*qword_141744EA8)) = v23;
//        *(__m128*)(v34 + v36 + 16) = v24;
//        *(__m128*)(v34 + v36 + 32) = v25;
//        *(__m128*)(v34 + (*qword_141744EA8) + 48) = v27;
//        v38 = *qword_141744EA8;
//        *(__m128*)(v37 + (*qword_141744EA8)) = v23;
//        *(__m128*)(v37 + v38 + 16) = v24;
//        *(__m128*)(v37 + v38 + 32) = v25;
//        *(__m128*)(v37 + (*qword_141744EA8) + 48) = v27;
//        v39 = (unsigned __int64)(unsigned int)(v31 + v12 + 2 * v31) << 6;
//        *(__m128*)(v39 + (*qword_141744EA8) + 48) = v27;
//        v40 = *qword_141744EA8;
//        *(__m128*)(v39 + (*qword_141744EA8)) = *(__m128*)a6; //*(_OWORD*)(v39 + qword_141744EA8) = *(_OWORD*)a6;
//        *(__m128*)(v39 + v40 + 16) = *(__m128*)(a6 + 16); //*(_OWORD*)(v39 + v40 + 16) = *(_OWORD*)(a6 + 16);
//        *(__m128*)(v39 + v40 + 32) = *(__m128*)(a6 + 32); //*(__m128*)(v40 + v41 + 32) = *(__m128*)(a6 + 32);
//    }
//    v41 = *(float*)(a5 + 36);
//    v42 = v84;
//    if (v41 <= 0.0f)
//    {
//        if ((v84->flags & STUDIOHDR_FLAGS_NO_FORCED_FADE) != 0)
//        {
//            v41 = 227023.36f;
//        }
//        else
//        {
//            v43 = v84->fadeDist;
//            if (v43 <= 0.0f)
//                v41 = fmaxf(
//                    (float)((float)(sqrtf(
//                        (float)((float)((float)(*(float*)v71.m128i_i32 - *(float*)&v71.m128i_i32[3]) * (float)(*(float*)v71.m128i_i32 - *(float*)&v71.m128i_i32[3]))
//                            + (float)((float)(*(float*)&v71.m128i_i32[1] - *(float*)&v72) * (float)(*(float*)&v71.m128i_i32[1] - *(float*)&v72)))
//                        + (float)((float)(*(float*)&v71.m128i_i32[2] - *((float*)&v72 + 1)) * (float)(*(float*)&v71.m128i_i32[2] - *((float*)&v72 + 1))))
//                        * 0.5)
//                        * *(float*)&v11)
//                    * model_defaultFadeDistScale->GetFloat(),
//                    model_defaultFadeDistMin->GetFloat());
//            else
//                v41 = v43 * *(float*)&v11;
//        }
//    }
//    v44 = fmaxf(v41, 100.0f);
//    *(float*)(a1 + 16) = v44 * v44; // <-- Data written here is incorrect [v41 used in the 'fmaxf' operation is most likely computed wrong]!
//    v45 = _mm_castps_si128(_mm_load_ss(&v84->gatherSize));
//    if (*(float*)v45.m128i_i32 <= 0.0f)
//    {
//        LOWORD(v48) = 0;
//    }
//    else
//    {
//        *(float*)v45.m128i_i32 = *(float*)v45.m128i_i32 * *(float*)v45.m128i_i32;
//        v46 = v45;
//        *(float*)v46.m128i_i32 = fmaxf(*(float*)v45.m128i_i32, 1.0004883f);
//        v47 = v46;
//        *(float*)v47.m128i_i32 = fminf(*(float*)v46.m128i_i32, 4293918700.0f);
//        v48 = (unsigned int)(_mm_cvtsi128_si32(v47) - 1065351168) >> 12;
//    }
//    *(_WORD*)(a1 + 6) = v48; // <-- DEBUG!
//    v49 = *(float*)((*qword_141744EA0) + 24 * ((unsigned __int64)*(unsigned int*)(a1 + 8) >> 1) + 8) - *(float*)((*qword_141744EA0) + 24 * ((unsigned __int64)*(unsigned int*)(a1 + 8) >> 1) + 20);
//    v50 = *(float*)((*qword_141744EA0) + 24 * ((unsigned __int64)*(unsigned int*)(a1 + 8) >> 1) + 4) - *(float*)((*qword_141744EA0) + 24 * ((unsigned __int64)*(unsigned int*)(a1 + 8) >> 1) + 16);
//    v51 = *(float*)((*qword_141744EA0) + 24 * ((unsigned __int64)*(unsigned int*)(a1 + 8) >> 1)) - *(float*)((*qword_141744EA0) + 24 * ((unsigned __int64)*(unsigned int*)(a1 + 8) >> 1) + 12);
//    v52 = (float)((float)(v50 * v50) + (float)(v51 * v51)) + (float)(v49 * v49);
//    if (v44 >= 227023.363449684f)
//        v9 = staticProp_no_fade_scalar->GetFloat();
//    v53 = 0;
//    *(float*)((*qword_141744E88) + 8i64 * a3) = v9 * (float)(1.0f / (float)(v52 * staticProp_gather_size_weight->GetFloat()));
//    v54 = *qword_141744E88;
//    *(_BYTE*)((*qword_141744E88) + 8i64 * a3 + 4) &= 0xFEu;
//    *(_BYTE*)(v54 + 8i64 * a3 + 4) |= v44 >= 227023.363449684f;
//    v55 = (__int64)CMDLCache::GetStudioMaterialGlue(g_MDLCache, *(unsigned __int16*)(a7 + 320)); // Gets some object containing pointer to 2 CMaterialGlue vtables.
//    v56 = *(unsigned __int16*)(a5 + 0x20);
//    v76 = *(__int64*)v55;
//    v57 = (__int64)v84 + 2 * v56 * v84->numskinref + v84->skinindex;
//    v58 = 0;
//    v85 = v57;
//    v83 = 0;
//    if (v84->numbodyparts <= 0)
//        return 0i64;
//    v59 = 0i64;
//    v73 = 0i64;
//    do
//    {
//        v60 = v59 + v42->bodypartindex;
//        v61 = 0;
//        v62 = (char*)v42 + v60;
//        v82 = 0;
//        v74.m128_u64[0] = (unsigned __int64)v62;
//        if (*((int*)v62 + 1) > 0)
//        {
//            v63 = 0i64;
//            v79 = 0i64;
//            do
//            {
//                v64 = 0;
//                v65 = &v62[v63 + *((int*)v62 + 3)];
//                if (*((int*)v65 + 19) > 0)
//                {
//                    v66 = v76;
//                    v67 = 0i64;
//                    do
//                    {
//                        v68 = *(void**)(v66 + 8i64 * *(__int16*)(v57 + 2i64 * *(int*)(v67 + *(int*)(v65 + 80) + v65)));
//
//                        static CModule::ModuleSections_t mData = g_GameDll.GetSectionByName(".data");
//                        static CModule::ModuleSections_t mPData = g_GameDll.GetSectionByName(".pdata");
//                        if (reinterpret_cast<uintptr_t>(v68) < mData.m_pSectionBase ||
//                            reinterpret_cast<uintptr_t>(v68) > mPData.m_pSectionBase) // Check bounds (data could only be within the '.data' segment.
//                            continue;
//
//                        if (!(*(unsigned __int8(__fastcall**)(void*))(*(_QWORD*)v68 + 688i64))(v68))
//                        {
//                            if (!(*(unsigned __int8(__fastcall**)(void*))(*(_QWORD*)v68 + 256i64))(v68) && (*(unsigned __int8(__fastcall**)(void*))(*(_QWORD*)v68 + 248i64))(v68))
//                            {
//                                v69 = 0;
//                                if (*dword_141744EE8)
//                                {
//                                    // Compares equality of pointers to the CMaterial object
//                                    // between the current offset [v69] in the CStaticPropMgr buffer
//                                    // and that of v68 (obtained via operation in parent loop).
//
//                                    while (*(&*(void**)off_141744E70 + v69 + 16) != v68)
//                                    {
//                                        v69++;// = (unsigned int)(v69 + 1);
//
//                                        if (static_cast<int32_t>(v69) >= *dword_141744EE8)
//                                            goto LABEL_42;
//                                    }
//                                }
//                                else
//                                {
//                                LABEL_42:
//                                    *(&*(void**)off_141744E70 + (*dword_141744EE8)++ + 16) = v68;
//                                }
//                            }
//                            if ((*(unsigned __int8(__fastcall**)(void*))(*(_QWORD*)v68 + 168i64))(v68) && (*(unsigned int(__fastcall**)(void*, __int64))(*(_QWORD*)v68 + 144i64))(v68, 1i64))
//                                *(_BYTE*)(a1 + 5) |= 0x80u;
//                            if (!(*(unsigned __int8(__fastcall**)(void*))(*(_QWORD*)v68 + 384i64))(v68) && (*(unsigned int(__fastcall**)(void*, __int64))(*(_QWORD*)v68 + 144i64))(v68, 21844i64))
//                                v53 |= 2u;
//                            v53 |= (*(unsigned __int8(__fastcall**)(void*))(*(_QWORD*)v68 + 384i64))(v68) != 0;
//                        }
//                        v57 = v85;
//                        ++v64;
//                        v67 += 92i64;
//                    } while (v64 < *((_DWORD*)v65 + 19));
//                    v62 = (char*)v74.m128_u64[0];
//                    v61 = v82;
//                    v63 = v79;
//                }
//                ++v61;
//                v63 += 136i64;
//                v82 = v61;
//                v79 = v63;
//            } while (v61 < *((_DWORD*)v62 + 1));
//            v42 = v84;
//            v58 = v83;
//        }
//        ++v58;
//        v59 = v73 + 16;
//        v83 = v58;
//        v73 += 16i64;
//    } while (v58 < v42->numbodyparts);
//    result = (void*)v53;
//    if (v53)
//        *(_BYTE*)(a1 + 5) &= 0x7Fu;
//    return result;
//}

//-----------------------------------------------------------------------------
// Purpose: calculates the view frustum culling data per static prop
//-----------------------------------------------------------------------------
void* __fastcall BuildPropStaticFrustumCullMap(int64_t a1, int64_t a2, unsigned int a3, unsigned int a4, int64_t a5, int64_t a6, int64_t a7)
{
    if (staticProp_defaultBuildFrustum->GetBool())
        return v_BuildPropStaticFrustumCullMap(a1, a2, a3, a4, a5, a6, a7);

    MDLHandle_t  handle; // dx
    studiohdr_t *studio; // rbx
    double          v54; // xmm7_8
    int64_t         v55; // rcx
    int             v56; // eax
    int64_t         v57; // rcx
    int             v58; // edx
    int64_t         v59; // rax
    int64_t         v60; // r13
    int             v61; // eax
    char           *v62; // r13
    int64_t         v63; // rdx
    int             v64; // er14
    char           *v65; // rdi
    int64_t         v67; // r15
    void           *v68; // rbx
    int64_t         v73; // [rsp+50h] [rbp-B8h]
    bool error  = false;

    handle = *reinterpret_cast<uint16_t*>(a7 + 0x140);
    studio = g_MDLCache->FindMDL(g_MDLCache, handle, nullptr);
    v55 = *reinterpret_cast<int64_t*>(g_MDLCache->GetMaterialTable(g_MDLCache, *reinterpret_cast<uint16_t*>((a7 + 320)))); // Gets some object containing pointer to 2 CMaterialGlue vtables.
    v56 = *reinterpret_cast<uint16_t*>(a5 + 0x20);
    v57 = reinterpret_cast<int64_t>(studio) + 2i64 * v56 * studio->numskinref + studio->skinindex;
    v58 = 0;
    if (studio->numbodyparts <= 0)
        return nullptr;
    v59 = 0i64;
    v73 = 0i64;
    do
    {
        v60 = v59 + studio->bodypartindex;
        v61 = 0;
        v62 = reinterpret_cast<char*>(studio) + v60;
        if (*((int*)v62 + 1) > 0)
        {
            v63 = 0i64;
            do
            {
                v64 = 0;
                v65 = &v62[v63 + *((int*)v62 + 3)];
                if (*((int*)v65 + 19) > 0)
                {
                    v67 = 0i64;
                    do
                    {
                        v68 = *(void**)(v55 + 8i64 * *(__int16*)(v57 + 2i64 * *(int*)(v67 + *(int*)(v65 + 80) + v65)));
                        ++v64;
                        v67 += 92i64;

                        if (reinterpret_cast<uintptr_t>(v68) < g_GameDll.m_RunTimeData.m_pSectionBase    || // Check bounds (data could only be within the '.data' segment.
                            reinterpret_cast<uintptr_t>(v68) > g_GameDll.m_ExceptionTable.m_pSectionBase || error)
                        {
                            error = true;
                            continue;
                        }
                    }           while (v64 < *((int*)v65 + 19));
                }
                ++v61;
                v63 += 136i64;
            }       while (v61 < *((int*)v62 + 1));
        }
        ++v58;
        v59 = v73 + 16;
        v73 += 16i64;
    }   while (v58 < studio->numbodyparts);

    if (error) // Don't use engine's implementation if batch contains errors!
    {
        *(MDLHandle_t*)a1 = handle;
        *(_QWORD*)&v54 = *(unsigned int*)(a5 + 24);
        *(_DWORD*)(a1 + 8) = a4;
        *(float*)(a1 + 12) = 1.0f / (float)(*(float*)&v54 * *(float*)&v54);
        *(float*)(a1 + 16) = 227023.36f * 227023.36f; // STUDIOHDR_FLAGS_NO_FORCED_FADE
        *(_QWORD*)(a1 + 20) = *(_QWORD*)a5;
        *(_DWORD*)(a1 + 28) = *(_DWORD*)(a5 + 8);

        return nullptr;
    }
    return v_BuildPropStaticFrustumCullMap(a1, a2, a3, a4, a5, a6, a7);
}

void BspLib_Attach()
{
#ifndef DEDICATED
    DetourAttach((LPVOID*)&v_BuildPropStaticFrustumCullMap, &BuildPropStaticFrustumCullMap);
#endif // !DEDICATED
}

void BspLib_Detach()
{
#ifndef DEDICATED
    DetourDetach((LPVOID*)&v_BuildPropStaticFrustumCullMap, &BuildPropStaticFrustumCullMap);
#endif // !DEDICATED
}
