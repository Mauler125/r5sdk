//=============================================================================//
//
// Purpose: RTech game utilities
//
//=============================================================================//
#include "core/stdafx.h"
#include "engine/host_cmd.h"
#include "engine/host_state.h"
#include "engine/cmodel_bsp.h"
#include "rtech/rtech_game.h"
#include "rtech/rtech_utils.h"

// Pak handles that have been loaded with the level
// from within the level settings KV (located in
// scripts/levels/settings/*.kv). On level unload,
// each pak listed in this vector gets unloaded.
CUtlVector<PakHandle_t> g_vLoadedPakHandle;

#ifdef GAMEDLL_S3
//-----------------------------------------------------------------------------
// Purpose: process guid relations for asset
// Input  : *pak   - 
//          *asset - 
//-----------------------------------------------------------------------------
void Pak_ProcessGuidRelationsForAsset(PakFile_t* pak, PakAsset_t* asset)
{
#if defined (GAMEDLL_S0) && defined (GAMEDLL_S1) && defined (GAMEDLL_S2)
    static const int GLOBAL_MUL = 0x1D;
#else
    static const int GLOBAL_MUL = 0x17;
#endif

    PakPage_t* pGuidDescriptors = &pak->m_memoryData.m_guidDescriptors[asset->m_usesStartIdx];
    volatile uint32_t* v5 = reinterpret_cast<volatile uint32_t*>(*(reinterpret_cast<uint64_t*>(g_pPakGlobals) + GLOBAL_MUL * (pak->m_memoryData.qword2D8 & 0x1FF) + 0x160212));
    const bool bDebug = rtech_debug->GetBool();

    if (bDebug)
        Msg(eDLL_T::RTECH, "Processing GUID relations for asset '0x%-16llX' in pak '%-32s'. Uses: %-4i\n", asset->m_guid, pak->m_memoryData.m_fileName, asset->m_usesCount);

    for (uint32_t i = 0; i < asset->m_usesCount; i++)
    {
        void** pCurrentGuid = reinterpret_cast<void**>(pak->m_memoryData.m_pagePointers[pGuidDescriptors[i].m_index] + pGuidDescriptors[i].m_offset);

        // Get current guid.
        const uint64_t currentGuid = reinterpret_cast<uint64_t>(*pCurrentGuid);

        // Get asset index.
        int assetIdx = currentGuid & 0x3FFFF;
        uint64_t assetIdxEntryGuid = g_pPakGlobals->m_assets[assetIdx].m_guid;

        const int64_t v9 = 2i64 * InterlockedExchangeAdd(v5, 1u);
        *reinterpret_cast<uint64_t*>(const_cast<uint32_t*>(&v5[2 * v9 + 2])) = currentGuid;
        *reinterpret_cast<uint64_t*>(const_cast<uint32_t*>(&v5[2 * v9 + 4])) = asset->m_guid;

        std::function<bool(bool)> fnCheckAsset = [&](bool shouldCheckTwo)
        {
            while (true)
            {
                if (shouldCheckTwo && assetIdxEntryGuid == 2)
                {
                    if (pak->m_memoryData.m_pakHeader.m_assetEntryCount)
                        return false;
                }

                assetIdx++;

                // Check if we have a deadlock and report it if we have rtech_debug enabled.
                if (bDebug && assetIdx >= 0x40000)
                {
                    if (assetIdx == 0x40000) // Only log it once.
                        Warning(eDLL_T::RTECH, "Possible deadlock detected while processing asset '0x%-16llX' in pak '%-32s'. Uses: %-4i | assetIdxEntryGuid: '0x%-16llX' | currentGuid: '0x%-16llX'\n", asset->m_guid, pak->m_memoryData.m_fileName, asset->m_usesCount, assetIdxEntryGuid, currentGuid);

                    if (IsDebuggerPresent())
                        DebugBreak();
                }

                assetIdx &= 0x3FFFF;
                assetIdxEntryGuid = g_pPakGlobals->m_assets[assetIdx].m_guid;

                if (assetIdxEntryGuid == currentGuid)
                    return true;
            }
        };

        if (assetIdxEntryGuid != currentGuid)
        {
            // Are we some special asset with the guid 2?
            if (!fnCheckAsset(true))
            {
                PakAsset_t* assetEntries = pak->m_memoryData.m_assetEntries;
                uint64_t a = 0;

                for (; assetEntries->m_guid != currentGuid; a++, assetEntries++)
                {
                    if (a >= pak->m_memoryData.m_pakHeader.m_assetEntryCount)
                    {
                        fnCheckAsset(false);
                        break;
                    }
                }

                assetIdx = pak->m_memoryData.qword2E0[a];
            }
        }

        // Finally write the pointer to the guid entry.
        *pCurrentGuid = g_pPakGlobals->m_assets[assetIdx].m_head;
    }
}
#endif // GAMEDLL_S3


//-----------------------------------------------------------------------------
// Purpose: load user-requested pak files on-demand
// Input  : *fileName  - 
//			*allocator - 
//			nIdx       - 
//			bUnk       - 
// Output : pak file handle on success, INVALID_PAK_HANDLE on failure
//-----------------------------------------------------------------------------
PakHandle_t Pak_LoadAsync(const char* fileName, CAlignedMemAlloc* allocator, int nIdx, bool bUnk)
{
	PakHandle_t pakHandle = INVALID_PAK_HANDLE;

	CUtlString pakBasePath;
	CUtlString pakOverridePath;

	pakBasePath.Format(PLATFORM_PAK_PATH "%s", fileName);
	pakOverridePath.Format(PLATFORM_PAK_OVERRIDE_PATH "%s", fileName);

	if (FileExists(pakOverridePath.Get()) || FileExists(pakBasePath.Get()))
	{
		Msg(eDLL_T::RTECH, "Loading pak file: '%s'\n", fileName);
		pakHandle = v_Pak_LoadAsync(fileName, allocator, nIdx, bUnk);

		if (pakHandle == INVALID_PAK_HANDLE)
		{
			Error(eDLL_T::RTECH, NO_ERROR, "%s: Failed read '%s' results '%u'\n", __FUNCTION__, fileName, pakHandle);
		}
	}
	else
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s: Failed; file '%s' doesn't exist\n", __FUNCTION__, fileName);
	}

	return pakHandle;
}

//-----------------------------------------------------------------------------
// Purpose: unloads loaded pak files
// Input  : handle - 
//-----------------------------------------------------------------------------
void Pak_UnloadPak(PakHandle_t handle)
{
	PakLoadedInfo_t* pakInfo = g_pRTech->GetPakLoadedInfo(handle);

	if (pakInfo && pakInfo->m_fileName)
	{
		Msg(eDLL_T::RTECH, "Unloading pak file: '%s'\n", pakInfo->m_fileName);

		if (strcmp(pakInfo->m_fileName, "mp_lobby.rpak") == 0)
			s_bBasePaksInitialized = false;
	}

	v_Pak_UnloadPak(handle);
}

//----------------------------------------------------------------------------------
// Purpose: open a file and add it to the file handle array
// Input  : *filePath    - 
//          unused       - 
//          *fileSizeOut - 
// Output : slot index in the array to which the file was added in
//----------------------------------------------------------------------------------
int32_t Pak_OpenFile(const CHAR* filePath, int64_t unused, LONGLONG* fileSizeOut)
{
    const CHAR* szFileToLoad = filePath;
    CUtlString pakBasePath(filePath);

    if (pakBasePath.Find(PLATFORM_PAK_PATH) != -1)
    {
        pakBasePath = pakBasePath.Replace(PLATFORM_PAK_PATH, PLATFORM_PAK_OVERRIDE_PATH);

        if (FileExists(pakBasePath.Get()))
        {
            szFileToLoad = pakBasePath.Get();
        }
    }

    const HANDLE hFile = CreateFileA(szFileToLoad, GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_DELETE, 0, OPEN_EXISTING, FILE_SUPPORTS_GHOSTING, 0);

    if (hFile == INVALID_HANDLE_VALUE)
        return -1;

    if (rtech_debug->GetBool())
        Msg(eDLL_T::RTECH, "Opened file: '%s'\n", szFileToLoad);

    if (fileSizeOut)
    {
        LARGE_INTEGER fileSize{};
        if (GetFileSizeEx(hFile, &fileSize))
            *fileSizeOut = fileSize.QuadPart;
    }

    AcquireSRWLockExclusive(reinterpret_cast<PSRWLOCK>(&*s_pFileArrayMutex));
    const int32_t fileIdx = RTech_FindFreeSlotInFiles(s_pFileArray);
    ReleaseSRWLockExclusive(reinterpret_cast<PSRWLOCK>(&*s_pFileArrayMutex));

    const int32_t fileHandleIdx = (fileIdx & 0x3FF); // Something with ArraySize.

    s_pFileHandles->self[fileHandleIdx].m_nFileNumber = fileIdx;
    s_pFileHandles->self[fileHandleIdx].m_hFileHandle = hFile;
    s_pFileHandles->self[fileHandleIdx].m_nCurOfs = 1;

    return fileIdx;
}

//----------------------------------------------------------------------------------
// Purpose: loads and processes a pak file (handles decompression and patching)
// Input  : *pak - 
// Output : true if patch source size == 0, false otherwise
// TODO: !!! FINISH REBUILD !!!
//----------------------------------------------------------------------------------
/*bool __fastcall Pak_ProcessPakFile(PakFile_t* pak)
{
    PakFileHeader_t* pakHeader; // r8
    __int64 dwordB8; // rcx
    int v6; // eax
    __int64 v7; // rax
    char v8; // r13
    signed __int64 index_Maybe; // rdi
    char v10; // r15
    __int64 v11; // rdx
    const char* v12; // rbp
    unsigned int v13; // eax
    __int64 v14; // r12
    char byteBF; // al
    unsigned __int64 v16; // r9
    unsigned __int8 v17; // cl
    unsigned __int64 v18; // r8
    __int64 v19; // rdx
    uint8_t byte1F8; // al
    uint8_t byte1FD; // cl
    _BYTE* v22; // rdi
    uint64_t v23; // rax
    PakDecompState_t* p_m_pakDecompState; // rbp
    __int64 decomp_size_var; // rax
    uint64_t v26; // rdx
    uint64_t qword1D0; // rcx
    __int64 v28; // rax
    unsigned int m_bitsRemaining; // r8d
    char* m_patchData; // rdx
    unsigned __int64 v31; // rax
    uint64_t m_dataBuf; // r11
    int v33; // r8d
    uint64_t v34; // rax
    int v35; // ecx
    __int64 v36; // rdx
    uint64_t v37; // r11
    __int64(__fastcall * v38)(); // rax
    int v39; // r10d
    int v40; // r9d
    uint64_t v41; // r11
    unsigned int v42; // ecx
    unsigned int v43; // r8d
    unsigned int v44; // r12d
    char byteBC; // r15
    __int64 v46; // rbp
    __int64 v47; // r8
    unsigned __int64 v48; // rbp
    unsigned __int64 qword8; // rax
    __int64 v50; // rdi
    __int64 patchCount; // rcx
    __int64 v52; // r15
    char v53; // al
    char* v54; // rcx
    char* i; // rdx
    int v56; // edi
    __int64 v57; // r15
    unsigned __int64 v58; // rdx
    char pak_path_var[260]; // [rsp+40h] [rbp-148h] BYREF
    char v61[12]; // [rsp+144h] [rbp-44h] BYREF
    unsigned __int64 v62; // [rsp+190h] [rbp+8h]
    size_t pak_file_size_var; // [rsp+198h] [rbp+10h] BYREF

    PakFileStream_t* fileStream = &pak->m_fileStream;
    PakMemoryData_t* memoryData = &pak->m_memoryData;

    dwordB8 = (unsigned int)pak->m_fileStream.dwordB8;
    if ((_DWORD)dwordB8)
        v62 = dwordB8 << 19;
    else
        v62 = 128i64;
    v6 = fileStream->unsigned_intB4;
    if (v6 != (_DWORD)dwordB8)
    {
        while (1)
        {
            v7 = v6 & 0x1F;
            v8 = fileStream->gap94[v7];
            if (v8 != 1)
                break;
        LABEL_17:
            v6 = fileStream->unsigned_intB4 + 1;
            fileStream->unsigned_intB4 = v6;
            if (v6 == fileStream->dwordB8)
                goto LABEL_18;
        }
        index_Maybe = (unsigned __int64)LOBYTE(fileStream->gap14[v7]) << 6;
        v10 = *((_BYTE*)&g_pakStatusSlots[0].someState + index_Maybe);
        switch (v10)
        {
        case 0:
            goto LABEL_18;
        case 2:
            v11 = *(unsigned int*)((char*)&g_pakStatusSlots[0].unk7 + index_Maybe);
            if ((_DWORD)v11 == 0x8716253)
                v12 = "Error: Short read.";
            else
                v12 = StrPrintf("Error 0x%08x", v11);
            break;
        case 3:
            v12 = "Cancelled.";
            break;
        default:
            v12 = "OK";
            if (v10 == 1)
            {
                v13 = *(int*)((char*)&g_pakStatusSlots[0].unk6 + index_Maybe);
                goto LABEL_11;
            }
            break;
        }
        v13 = 0;
    LABEL_11:
        v14 = v13;
        v_Pak_CloseFile(*(int*)((char*)&g_pakStatusSlots[0].unk4 + index_Maybe));
        AcquireSRWLockExclusive(&stru_16721D260);
        RTech::FreeSlotInFiles((__int64)&dword_16721D240, index_Maybe >> 6);
        ReleaseSRWLockExclusive(&stru_16721D260);
        if (v10 == 2)
            Error(eDLL_T::RTECH, EXIT_FAILURE, "Error reading pak file \"%s\" -- %s\n", pak->m_memoryData.m_fileName, v12);
        fileStream->qword1D0 += v14;
        if (v8)
        {
            byteBF = fileStream->byteBF;
            pakHeader = &pak->m_memoryData.m_pakHeader;
            v16 = (unsigned __int64)fileStream->unsigned_intB4 << 19;
            v17 = byteBF & 7;
            fileStream->byteBF = byteBF + 1;
            if (v8 == 2)
            {
                v18 = v16 & fileStream->qword1C8;
                fileStream->qword1D0 = v14 + v16;
                pakHeader = (PakFileHeader_t*)&fileStream->buffer[v18];
            }
            v19 = 32i64 * v17;
            *(_QWORD*)&fileStream->gapC0[32 * v17] = v16 + 128;
            *(_QWORD*)&fileStream->gapC0[v19 + 8] = v16 + pakHeader->m_compressedSize;
            *(_QWORD*)&fileStream->gapC0[v19 + 16] = pakHeader->m_decompressedSize;
            fileStream->gapC0[v19 + 24] = pakHeader->m_flags[1] & 1;
        }
        goto LABEL_17;
    }
LABEL_18:
    byte1F8 = pak->byte1F8;
    if (byte1F8 != fileStream->byteBF)
    {
        byte1FD = pak->byte1FD;
        do
        {
            v22 = &fileStream->gapC0[32 * (byte1F8 & 7)];
            if (byte1FD)
            {
                pak->byte1FD = 0;
                pak->m_inputBytePos = *(_QWORD*)v22;
                if (v22[24])
                {
                    pak->flags_1FE = 256;
                    v23 = 128i64;
                }
                else
                {
                    pak->flags_1FE = 1;
                    v23 = *(_QWORD*)v22;
                }
                memoryData->m_processedPatchedDataSize = v23;
                if (!HIBYTE(pak->flags_1FE))
                {
                LABEL_35:
                    v26 = *((_QWORD*)v22 + 1);
                    qword1D0 = v26;
                    if (fileStream->qword1D0 < v26)
                        qword1D0 = fileStream->qword1D0;
                    goto LABEL_41;
                }
                p_m_pakDecompState = &pak->m_pakDecompState;
                decomp_size_var = RTech::DecompressedSize(
                    &pak->m_pakDecompState,
                    (uint64_t)fileStream->buffer,
                    (__int64)pakHeader,
                    *((_QWORD*)v22 + 1) - (*(_QWORD*)v22 - sizeof(PakFileHeader_t)),
                    *(_QWORD*)v22 - sizeof(PakFileHeader_t));

                if (decomp_size_var != *((_QWORD*)v22 + 2))
                {
                    Error(eDLL_T::RTECH, EXIT_FAILURE,
                        "Error reading pak file \"%s\" -- decompressed size %u doesn't match expected value %u\n",
                        pak->m_memoryData.m_fileName,
                        decomp_size_var + sizeof(PakFileHeader_t),
                        pak->m_memoryData.m_pakHeader.m_decompressedSize);
                }

                pak->m_pakDecompState.m_outputBuf = (uint64_t)pak->m_decompBuffer;
                pak->m_pakDecompState.m_outputMask = 0x3FFFFFi64;
            }
            else
            {
                p_m_pakDecompState = &pak->m_pakDecompState;
            }
            if (!HIBYTE(pak->flags_1FE))
                goto LABEL_35;
            qword1D0 = pak->m_pakDecompState.m_decompBytePosition;
            if (qword1D0 != pak->m_pakDecompState.m_decompSize)
            {
                Rtech::Decompress(p_m_pakDecompState, fileStream->qword1D0, memoryData->m_processedPatchedDataSize + 0x400000);
                qword1D0 = pak->m_pakDecompState.m_decompBytePosition;
                pak->m_inputBytePos = pak->m_pakDecompState.m_fileBytePosition;
            }
            v26 = *((_QWORD*)v22 + 1);
        LABEL_41:
            if (pak->m_inputBytePos != v26 || memoryData->m_processedPatchedDataSize != qword1D0)
                goto LABEL_45;
            byte1FD = 1;
            byte1F8 = pak->byte1F8 + 1;
            pak->byte1FD = 1;
            pak->byte1F8 = byte1F8;
        } while (byte1F8 != fileStream->byteBF);
    }
    qword1D0 = memoryData->m_processedPatchedDataSize;
LABEL_45:
    v28 = memoryData->field_2A8;
    pak_file_size_var = qword1D0 - memoryData->m_processedPatchedDataSize;
    if (memoryData->m_patchSrcSize + v28)
    {
        do
        {
            if (!memoryData->m_numBytesToProcess_maybe)
            {
                m_bitsRemaining = memoryData->m_bitBuf.m_bitsRemaining;
                m_patchData = memoryData->m_patchData;
                memoryData->m_bitBuf.m_dataBuf |= *m_patchData << (64 - (unsigned __int8)m_bitsRemaining);
                v31 = m_bitsRemaining;
                m_dataBuf = memoryData->m_bitBuf.m_dataBuf;
                v33 = m_bitsRemaining & 7;
                memoryData->m_bitBuf.m_bitsRemaining = v33;
                memoryData->m_patchData = m_patchData + (v31 >> 3);
                v34 = m_dataBuf & 0x3F;
                v35 = (unsigned __int8)memoryData->PATCH_field_68[v34];
                v36 = (unsigned __int8)memoryData->patchCommands[v34];
                v37 = m_dataBuf >> v35;
                memoryData->m_bitBuf.m_dataBuf = v37;
                v38 = s_pakPatchFuncs[v36];
                memoryData->m_bitBuf.m_bitsRemaining = v33 + v35;
                memoryData->patchFunc = (unsigned __int8(__fastcall*)(PakFile_t*, unsigned __int64*))v38;
                if ((unsigned __int8)v36 > 3u)
                {
                    memoryData->m_numBytesToProcess_maybe = *((unsigned int*)&off_141367980 + v36);
                }
                else
                {
                    v39 = (unsigned __int8)memoryData->PATCH_unk3[(unsigned __int8)v37];
                    v40 = (unsigned __int8)memoryData->PATCH_unk2[(unsigned __int8)v37];
                    v41 = v37 >> memoryData->PATCH_unk3[(unsigned __int8)v37];
                    memoryData->m_bitBuf.m_dataBuf = v41 >> v40;
                    memoryData->m_numBytesToProcess_maybe = (1i64 << v40) + (v41 & ((1i64 << v40) - 1));
                    memoryData->m_bitBuf.m_bitsRemaining = v33 + v35 + v40 + v39;
                }
            }
        } while (pak->m_memoryData.patchFunc(pak, &pak_file_size_var) && memoryData->m_patchSrcSize + memoryData->field_2A8);
    }
    if (LOBYTE(pak->flags_1FE))
        pak->m_inputBytePos = memoryData->m_processedPatchedDataSize;
    if (!fileStream->byteBD)
    {
        v42 = fileStream->unsigned_intB4;
        v43 = fileStream->dwordB8;
        if ((unsigned int)(pak->m_inputBytePos >> 19) < v42)
            v42 = pak->m_inputBytePos >> 19;
        v44 = v42 + 32;
        if (v43 != v42 + 32)
        {
            while (1)
            {
                byteBC = fileStream->byteBC;
                v46 = v43;
                v47 = v43 & 0x1F;
                v48 = (v46 + 1) << 19;
                if (byteBC == 1)
                    break;
                qword8 = fileStream->qword8;
                if (v62 < qword8)
                {
                    v50 = (unsigned int)v47;
                    if (v48 < qword8)
                        qword8 = v48;
                    fileStream->gap14[(unsigned int)v47] = Pak_SetFileStreamContext(
                        fileStream->fileHandle,
                        v62 - fileStream->qword0,
                        qword8 - v62,
                        &fileStream->buffer[v62 & fileStream->qword1C8],
                        0i64,
                        0i64,
                        4);
                    fileStream->gap94[v50] = byteBC;
                    fileStream->byteBC = 0;
                    goto LABEL_65;
                }
                if (pak->m_patchCount >= LOWORD(pak->m_memoryData.m_pakHeader.m_patchIndex))
                {
                    v_Pak_CloseFile(fileStream->fileHandle);
                    fileStream->fileHandle = -1;
                    fileStream->qword0 = 0i64;
                    fileStream->byteBD = 1;
                    return memoryData->m_patchSrcSize == 0;
                }
                if (!pak->dword14)
                    return memoryData->m_patchSrcSize == 0;
                sprintf(pak_path_var, "paks\\Win64\\%s", pak->m_memoryData.m_fileName);
                patchCount = pak->m_patchCount;
                v52 = (unsigned int)patchCount;
                pak->m_patchCount = patchCount + 1;
                if (pak->m_memoryData.UnkPatchIndexes[patchCount])
                {
                    v53 = pak_path_var[0];
                    v54 = pak_path_var;
                    for (i = 0i64; v53; ++v54)
                    {
                        if (v53 == '.')
                        {
                            i = v54;
                        }
                        else if (v53 == '\\' || v53 == '/')
                        {
                            i = 0i64;
                        }
                        v53 = v54[1];
                    }
                    if (i)
                        v54 = i;
                    snprintf(v54, v61 - v54, "(%02u).rpak");
                }

                v56 = v_Pak_OpenFile(pak_path_var, 5i64, (__int64*)&pak_file_size_var);

                if (v56 == -1)
                    Error(eDLL_T::RTECH, EXIT_FAILURE, "Couldn't open file \"%s\".\n", pak_path_var);

                v57 = v52;

                if (pak_file_size_var < pak->m_memoryData.m_patchHeaders[v57].m_sizeDisk)
                    Error(eDLL_T::RTECH, EXIT_FAILURE, "File \"%s\" appears truncated.\n", pak_path_var);

                v_Pak_CloseFile(fileStream->fileHandle);
                v43 = fileStream->dwordB8;
                fileStream->fileHandle = v56;
                v58 = (unsigned __int64)((v43 + 7) & 0xFFFFFFF8) << 19;
                fileStream->qword0 = v58;
                fileStream->byteBC = (v43 == ((v43 + 7) & 0xFFFFFFF8)) + 1;
                fileStream->qword8 = v58 + pak->m_memoryData.m_patchHeaders[v57].m_sizeDisk;
            LABEL_84:
                if (v43 == v44)
                    return memoryData->m_patchSrcSize == 0;
            }
            fileStream->gap14[v47] = -2;
            fileStream->gap94[v47] = 1;
            if ((((_BYTE)v47 + 1) & 7) == 0)
                fileStream->byteBC = 2;
        LABEL_65:
            v43 = ++fileStream->dwordB8;
            v62 = v48;
            goto LABEL_84;
        }
    }
    return memoryData->m_patchSrcSize == 0;
}*/

void V_RTechGame::Detour(const bool bAttach) const
{
    DetourSetup(&v_Pak_OpenFile, &Pak_OpenFile, bAttach);

	DetourSetup(&v_Pak_LoadAsync, &Pak_LoadAsync, bAttach);
	DetourSetup(&v_Pak_UnloadPak, &Pak_UnloadPak, bAttach);

#ifdef GAMEDLL_S3
    //DetourSetup(&RTech_Pak_ProcessGuidRelationsForAsset, &RTech::PakProcessGuidRelationsForAsset, bAttach);
#endif
}

// Symbols taken from R2 dll's.
PakLoadFuncs_t* g_pakLoadApi = nullptr;
