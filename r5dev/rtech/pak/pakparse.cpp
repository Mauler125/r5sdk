//=============================================================================//
//
// Purpose: pak file loading and unloading
//
//=============================================================================//
#include "rtech/ipakfile.h"
#include "rtech/async/asyncio.h"

#include "paktools.h"
#include "pakstate.h"
#include "pakpatch.h"
#include "pakalloc.h"
#include "pakparse.h"
#include "pakdecode.h"
#include "pakstream.h"

//-----------------------------------------------------------------------------
// resolve the target guid from lookuo table
//-----------------------------------------------------------------------------
static bool Pak_ResolveAssetDependency(const PakFile_t* const pak, PakGuid_t currentGuid,
    const PakGuid_t targetGuid, int& currentIndex, const bool shouldCheckTwo)
{
    while (true)
    {
        if (shouldCheckTwo && currentGuid == 2)
        {
            if (pak->memoryData.pakHeader.assetCount)
                return false;
        }

        currentIndex++;

        if (currentIndex >= PAK_MAX_ASSETS)
            return false;

        currentIndex &= PAK_MAX_ASSETS_MASK;
        currentGuid = g_pPakGlobals->m_assets[currentIndex].guid;

        if (currentGuid == targetGuid)
            return true;
    }

    UNREACHABLE();
}

//-----------------------------------------------------------------------------
// resolve guid relations for asset
//-----------------------------------------------------------------------------
void Pak_ResolveAssetRelations(PakFile_t* const pak, const PakAsset_t* const asset)
{
    PakPage_t* const pGuidDescriptors = &pak->memoryData.guidDescriptors[asset->dependenciesIndex];
    volatile uint32_t* v5 = reinterpret_cast<volatile uint32_t*>(*(reinterpret_cast<uint64_t*>(g_pPakGlobals) + 0x17 * (pak->memoryData.pakId & PAK_MAX_HANDLES_MASK) + 0x160212));

    if (pak_debugrelations->GetBool())
        Msg(eDLL_T::RTECH, "Resolving relations for asset: '0x%-16llX', dependencies: %-4u; in pak '%s'\n",
            asset->guid, asset->dependenciesCount, pak->memoryData.fileName);

    for (uint32_t i = 0; i < asset->dependenciesCount; i++)
    {
        void** const pCurrentGuid = reinterpret_cast<void**>(pak->memoryData.memPageBuffers[pGuidDescriptors[i].index] + pGuidDescriptors[i].offset);

        // get current guid
        const PakGuid_t targetGuid = reinterpret_cast<uint64_t>(*pCurrentGuid);

        // get asset index
        int currentIndex = targetGuid & PAK_MAX_ASSETS_MASK;
        const PakGuid_t currentGuid = g_pPakGlobals->m_assets[currentIndex].guid;

        const int64_t v9 = 2i64 * InterlockedExchangeAdd(v5, 1u);
        *reinterpret_cast<PakGuid_t*>(const_cast<uint32_t*>(&v5[2 * v9 + 2])) = targetGuid;
        *reinterpret_cast<PakGuid_t*>(const_cast<uint32_t*>(&v5[2 * v9 + 4])) = asset->guid;

        if (currentGuid != targetGuid)
        {
            // are we some special asset with the guid 2?
            if (!Pak_ResolveAssetDependency(pak, currentGuid, targetGuid, currentIndex, true))
            {
                PakAsset_t* assetEntries = pak->memoryData.assetEntries;
                uint64_t a = 0;

                for (; assetEntries->guid != targetGuid; a++, assetEntries++)
                {
                    if (a >= pak->memoryData.pakHeader.assetCount)
                    {
                        if (!Pak_ResolveAssetDependency(pak, currentGuid, targetGuid, currentIndex, false))
                        {
                            // the dependency couldn't be resolved, this state is irrecoverable;
                            // error out
                            Error(eDLL_T::RTECH, EXIT_FAILURE, "Failed to resolve asset dependency %u of %u\n"
                                "pak: '%s'\n"
                                "asset: '0x%llX'\n"
                                "target: '0x%llX'\n"
                                "current: '0x%llX'\n",
                                i, asset->dependenciesCount,
                                pak->memoryData.fileName,
                                asset->guid,
                                targetGuid,
                                currentGuid);
                        }

                        break;
                    }
                }

                currentIndex = pak->memoryData.qword2E0[a];
            }
        }

        // finally write the pointer to the guid entry
        *pCurrentGuid = g_pPakGlobals->m_assets[currentIndex].m_head;
    }
}

uint32_t Pak_ProcessRemainingPagePointers(PakFile_t* const pak)
{
    uint32_t processedPointers = 0;

    for (processedPointers = pak->numProcessedPointers; processedPointers < pak->GetPointerCount(); ++processedPointers)
    {
        PakPage_t* const curPage = &pak->memoryData.virtualPointers[processedPointers];
        int curCount = curPage->index - pak->firstPageIdx;

        if (curCount < 0)
            curCount += pak->memoryData.pakHeader.memPageCount;

        if (curCount >= pak->processedPageCount)
            break;

        PakPage_t* const ptr = reinterpret_cast<PakPage_t*>(pak->GetPointerForPageOffset(curPage));
        ptr->ptr = pak->memoryData.memPageBuffers[ptr->index] + ptr->offset;
    }

    return processedPointers;
}

void __fastcall Rebuild_14043E030(PakFile_t* const pak)
{
    __int64 numAssets; // rsi
    PakAsset_t* pakAsset; // rdi
    __int64 _numAssets; // r14
    unsigned int assetBind; // ebp
    __int64 v13; // rcx
    __int64 qword2D8_low; // rdi
    JobID_t qword2D8_high; // ebp
    JobTypeID_t v16; // si

    pak->numProcessedPointers = Pak_ProcessRemainingPagePointers(pak);

    numAssets = (unsigned int)pak->processedAssetCount;
    if ((_DWORD)numAssets != pak->memoryData.pakHeader.assetCount)
    {
        pakAsset = &pak->memoryData.assetEntries[numAssets];
        _numAssets = (unsigned int)numAssets;
        if ((int)pakAsset->pageEnd <= pak->processedPageCount)
        {
            while ((unsigned __int16)*word_167ED7BDE <= 0xC8u)
            {
                assetBind = pakAsset->HashTableIndexForAssetType();
                pak->memoryData.qword2E0[_numAssets] = (int)sub_14043D3C0(pak, pakAsset);
                _InterlockedIncrement16(word_167ED7BDE);
                v13 = assetBind;
                if (g_pPakGlobals->m_assetBindings[(unsigned __int64)assetBind].loadAssetFunc)
                {
                    qword2D8_low = pak->memoryData.pakId;
                    qword2D8_high = pak->memoryData.unkJobID;
                    v16 = *((_BYTE*)&*g_pPakGlobals + v13 + 13207872);

                    JTGuts_AddJob(v16, qword2D8_high, (void*)qword2D8_low, (void*)_numAssets);
                }
                else
                {
                    if (_InterlockedExchangeAdd16((volatile signed __int16*)&pakAsset->numRemainingDependencies, 0xFFFFu) == 1)
                        sub_14043D150(pak, pakAsset, (unsigned int)numAssets, assetBind);
                    _InterlockedDecrement16(word_167ED7BDE);
                }
                numAssets = (unsigned int)++pak->processedAssetCount;
                if ((_DWORD)numAssets == pak->memoryData.pakHeader.assetCount)
                {
                    JT_EndJobGroup(pak->memoryData.unkJobID);
                    return;
                }
                _numAssets = (unsigned int)numAssets;
                pakAsset = &pak->memoryData.assetEntries[numAssets];

                if (pakAsset->pageEnd > pak->processedPageCount)
                    return;
            }
        }
    }
}

//-----------------------------------------------------------------------------
// load user-requested pak files on-demand
//-----------------------------------------------------------------------------
PakHandle_t Pak_LoadAsync(const char* const fileName, CAlignedMemAlloc* const allocator, const int nIdx, const bool bUnk)
{
    PakHandle_t pakId = INVALID_PAK_HANDLE;

    if (Pak_FileExists(fileName))
    {
        Msg(eDLL_T::RTECH, "Loading pak file: '%s'\n", fileName);
        pakId = v_Pak_LoadAsync(fileName, allocator, nIdx, bUnk);

        if (pakId == INVALID_PAK_HANDLE)
            Error(eDLL_T::RTECH, NO_ERROR, "%s: Failed read '%s' results '%d'\n", __FUNCTION__, fileName, pakId);
    }
    else
    {
        Error(eDLL_T::RTECH, NO_ERROR, "%s: Failed; file '%s' doesn't exist\n", __FUNCTION__, fileName);
    }

    return pakId;
}

//-----------------------------------------------------------------------------
// unloads loaded pak files
//-----------------------------------------------------------------------------
void Pak_UnloadAsync(PakHandle_t handle)
{
    const PakLoadedInfo_t* const pakInfo = Pak_GetPakInfo(handle);

    if (pakInfo && pakInfo->fileName)
        Msg(eDLL_T::RTECH, "Unloading pak file: '%s'\n", pakInfo->fileName);

    v_Pak_UnloadAsync(handle);
}

#define CMD_INVALID -1

// only patch cmds 4,5,6 use this array to determine their data size
static const int s_patchCmdToBytesToProcess[] = { CMD_INVALID, CMD_INVALID, CMD_INVALID, CMD_INVALID, 3, 7, 6, 0 };
#undef CMD_INVALID
//----------------------------------------------------------------------------------
// loads and processes a pak file (handles decompression and patching)
// TODO: !!! FINISH REBUILD !!!
//----------------------------------------------------------------------------------
bool Pak_ProcessPakFile(PakFile_t* const pak)
{
    PakFileHeader_t* pakHeader; // r8
    PakFileStream_t* fileStream; // rsi
    PakMemoryData_t* memoryData; // r14
    __int64 dwordB8; // rcx
    unsigned int v6; // eax // [was: int]
    __int64 v7; // rax
    char v8; // r13
    //signed __int64 index_Maybe; // rdi
    //char v10; // r15
    //__int64 v11; // rdx
    //const char* v12; // rbp
    size_t bytesProcessed; // eax
    char byteBF; // al
    unsigned __int64 v16; // r9
    unsigned __int8 v17; // cl
    unsigned __int64 v18; // r8
    uint8_t byte1F8; // al
    uint8_t byte1FD; // cl
    PakFileStream_t::Descriptor* v22; // rdi
    size_t dataOffset; // rax
    PakDecoder_t* decodeContext; // rbp
    size_t decompressedSize; // rax
    size_t compressedSize; // rdx
    uint64_t qword1D0; // rcx
    __int64 v28; // rax
    unsigned int numBitsRemaining; // r8d
    int v35; // ecx
    int v39; // r10d
    int v40; // r9d
    unsigned int v42; // ecx
    unsigned int v43; // r8d
    unsigned int v44; // r12d
    char byteBC; // r15
    __int64 v46; // rbp
    __int64 v47; // r8
    unsigned __int64 v48; // rbp
    unsigned __int64 qword8; // rax
    __int64 v50; // rdi
    char c; // al
    char* it; // rcx
    char* i; // rdx
    int v56; // edi
    unsigned int patchCount; // r15
    unsigned __int64 v58; // rdx
    char pakPatchPath[MAX_PATH]; // [rsp+40h] [rbp-148h] BYREF
    unsigned __int64 v62; // [rsp+190h] [rbp+8h]
    size_t numBytesToProcess; // [rsp+198h] [rbp+10h] BYREF

    fileStream = &pak->fileStream;
    memoryData = &pak->memoryData;
    dwordB8 = (unsigned int)pak->fileStream.dwordB8;

    if ((_DWORD)dwordB8)
        v62 = dwordB8 << 19;
    else
        v62 = sizeof(PakFileHeader_t);

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

        const char* statusMsg = "(no reason)";
        const uint8_t currentStatus = g_pakLoadApi->CheckAsyncRequest((unsigned char)fileStream->gap14[v7], &bytesProcessed, &statusMsg);

        if (currentStatus == AsyncHandleStatus_t::FS_ASYNC_PENDING)
            goto LABEL_18;

        if (currentStatus == AsyncHandleStatus_t::FS_ASYNC_ERROR)
            Error(eDLL_T::RTECH, EXIT_FAILURE, "Error reading pak file \"%s\" -- %s\n", pak->memoryData.fileName, statusMsg);

        fileStream->qword1D0 += bytesProcessed;
        if (v8)
        {
            byteBF = fileStream->byteBF++;
            pakHeader = &pak->memoryData.pakHeader;
            v16 = (unsigned __int64)fileStream->unsigned_intB4 << 19;

            if (v8 == 2)
            {
                v18 = v16 & fileStream->qword1C8;
                fileStream->qword1D0 = bytesProcessed + v16;
                pakHeader = (PakFileHeader_t*)&fileStream->buffer[v18];
            }

            v17 = byteBF & 7;

            fileStream->m_descriptors[v17].dataOffset = v16 + sizeof(PakFileHeader_t);
            fileStream->m_descriptors[v17].compressedSize = v16 + pakHeader->compressedSize;
            fileStream->m_descriptors[v17].decompressedSize = pakHeader->decompressedSize;
            fileStream->m_descriptors[v17].isCompressed = pakHeader->IsCompressed();
        }
        goto LABEL_17;
    }
LABEL_18:
    byte1F8 = pak->byte1F8;
    if (byte1F8 != fileStream->byteBF)
    {
        const bool usesCustomCompression = pak->GetHeader().flags & PAK_HEADER_FLAGS_ZSTD;

        byte1FD = pak->byte1FD;
        do
        {
            v22 = &fileStream->m_descriptors[byte1F8 & 7];

            if (byte1FD)
            {
                pak->byte1FD = 0;
                pak->inputBytePos = v22->dataOffset;

                if (v22->isCompressed)
                {
                    pak->isOffsetted_MAYBE = 0;
                    pak->isCompressed = 1;
                    dataOffset = sizeof(PakFileHeader_t);
                }
                else
                {
                    pak->isOffsetted_MAYBE = 1;
                    pak->isCompressed = 0;
                    dataOffset = v22->dataOffset;
                }

                memoryData->processedPatchedDataSize = dataOffset;

                if (!pak->isCompressed)
                {
                LABEL_35:
                    compressedSize = v22->compressedSize;
                    qword1D0 = compressedSize;
                    if (fileStream->qword1D0 < compressedSize)
                        qword1D0 = fileStream->qword1D0;
                    goto LABEL_41;
                }

                decodeContext = &pak->pakDecoder;

                decompressedSize = Pak_InitDecoder(&pak->pakDecoder, fileStream->buffer,
                    PAK_DECODE_IN_BUFFER_MASK, v22->compressedSize - (v22->dataOffset - sizeof(PakFileHeader_t)),
                    v22->dataOffset - sizeof(PakFileHeader_t), sizeof(PakFileHeader_t), usesCustomCompression);

                if (decompressedSize != v22->decompressedSize)
                    Error(eDLL_T::RTECH, EXIT_FAILURE,
                        "Error reading pak file \"%s\" -- decompressed size %zu doesn't match expected value %zu\n",
                        pak->memoryData.fileName,
                        decompressedSize,
                        pak->memoryData.pakHeader.decompressedSize);

                pak->pakDecoder.outputBuf = pak->decompBuffer;
                pak->pakDecoder.outputMask = PAK_DECODE_OUT_BUFFER_SIZE_MASK;
            }
            else
            {
                decodeContext = &pak->pakDecoder;
            }

            if (!pak->isCompressed)
                goto LABEL_35;

            qword1D0 = pak->pakDecoder.outBufBytePos;

            if (qword1D0 != pak->pakDecoder.decompSize)
            {
                const bool didDecode = Pak_StreamToBufferDecode(decodeContext, fileStream->qword1D0, memoryData->processedPatchedDataSize + PAK_DECODE_OUT_BUFFER_SIZE, usesCustomCompression);

                qword1D0 = pak->pakDecoder.outBufBytePos;
                pak->inputBytePos = pak->pakDecoder.inBufBytePos;

                if (didDecode)
                {
                    if (usesCustomCompression && decodeContext->zstreamContext)
                    {
                        ZSTD_freeDStream(decodeContext->zstreamContext);
                        decodeContext->zstreamContext = nullptr;
                    }
                }
            }

            compressedSize = v22->compressedSize;

        LABEL_41:
            if (pak->inputBytePos != compressedSize || memoryData->processedPatchedDataSize != qword1D0)
                goto LABEL_45;

            byte1FD = 1;
            byte1F8 = pak->byte1F8 + 1;
            pak->byte1FD = 1;
            pak->byte1F8 = byte1F8;
        } while (byte1F8 != fileStream->byteBF);
    }

    qword1D0 = memoryData->processedPatchedDataSize;
LABEL_45:
    v28 = memoryData->field_2A8;
    numBytesToProcess = qword1D0 - memoryData->processedPatchedDataSize;

    if (memoryData->patchSrcSize + v28)
    {
        do
        {
            // if there are no bytes left to process in this patch operation
            if (!memoryData->numBytesToProcess_maybe)
            {
                RBitRead& bitbuf = memoryData->bitBuf;

                numBitsRemaining = bitbuf.m_bitsRemaining; // number of "free" bits in the bitbuf (how many to fetch)

                // fetch remaining bits
                bitbuf.m_dataBuf |= *(_QWORD*)memoryData->patchData << (64 - (unsigned __int8)numBitsRemaining);

                // advance patch data buffer by the number of bytes that have just been fetched
                memoryData->patchData = &memoryData->patchData[numBitsRemaining >> 3];

                // store the number of bits remaining to complete the data read
                bitbuf.m_bitsRemaining = numBitsRemaining & 7; // number of bits above a whole byte

                const unsigned __int8 index1 = static_cast<unsigned __int8>(bitbuf.ReadBits(6));
                v35 = memoryData->PATCH_field_68[index1]; // number of bits to discard from bitbuf
                __int8 cmd = memoryData->patchCommands[index1];

                bitbuf.DiscardBits(v35);

                // get the next patch function to execute
                memoryData->patchFunc = g_pakPatchApi[cmd];

                if (cmd <= 3u)
                {
                    const unsigned __int8 index2 = static_cast<unsigned __int8>(bitbuf.ReadBits(8));
                    v39 = memoryData->PATCH_unk3[index2];
                    v40 = memoryData->PATCH_unk2[index2]; // number of stored bits for the data size

                    bitbuf.DiscardBits(v39);

                    memoryData->numBytesToProcess_maybe = (1ull << v40) + bitbuf.ReadBits(v40);

                    bitbuf.DiscardBits(v40);
                }
                else
                {
                    memoryData->numBytesToProcess_maybe = s_patchCmdToBytesToProcess[cmd];
                }
            }

        } while (pak->memoryData.patchFunc(pak, &numBytesToProcess) && memoryData->patchSrcSize + memoryData->field_2A8);
    }

    if (pak->isOffsetted_MAYBE)
        pak->inputBytePos = memoryData->processedPatchedDataSize;

    if (!fileStream->finishedLoadingPatches)
    {
        v42 = fileStream->unsigned_intB4;
        v43 = fileStream->dwordB8;

        if ((unsigned int)(pak->inputBytePos >> 19) < v42)
            v42 = (unsigned int)pak->inputBytePos >> 19; // New cast added

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
                    fileStream->gap14[(unsigned int)v47] = v_FS_ReadAsyncFile(
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

                if (pak->patchCount >= pak->memoryData.pakHeader.patchIndex)
                {
                    FS_CloseAsyncFile((short)fileStream->fileHandle);
                    fileStream->fileHandle = INVALID_PAK_HANDLE;
                    fileStream->qword0 = 0i64;
                    fileStream->finishedLoadingPatches = true;

                    return memoryData->patchSrcSize == 0;
                }

                if (!pak->dword14)
                    return memoryData->patchSrcSize == 0;

                sprintf(pakPatchPath, PLATFORM_PAK_PATH"%s", pak->memoryData.fileName);
                patchCount = pak->patchCount++;

                // get path of next patch rpak to load
                if (pak->memoryData.patchIndices[patchCount])
                {
                    c = pakPatchPath[0];
                    it = pakPatchPath;

                    for (i = nullptr; c; ++it)
                    {
                        if (c == '.')
                        {
                            i = it;
                        }
                        else if (c == '\\' || c == '/')
                        {
                            i = nullptr;
                        }
                        c = it[1];
                    }
                    if (i)
                        it = i;

                    // replace extension '.rpak' with '(xx).rpak'
                    snprintf(it, &pakPatchPath[sizeof(pakPatchPath)] - it,
                        "(%02u).rpak", pak->memoryData.patchIndices[patchCount]);
                }

                v56 = FS_OpenAsyncFile(pakPatchPath, 5i64, &numBytesToProcess);

                if (v56 == FS_ASYNC_FILE_INVALID)
                    Error(eDLL_T::RTECH, EXIT_FAILURE, "Couldn't open file \"%s\".\n", pakPatchPath);

                if (numBytesToProcess < pak->memoryData.patchHeaders[patchCount].m_sizeDisk)
                    Error(eDLL_T::RTECH, EXIT_FAILURE, "File \"%s\" appears truncated; read size: %zu < expected size: %zu.\n",
                        pakPatchPath, numBytesToProcess, pak->memoryData.patchHeaders[patchCount].m_sizeDisk);

                FS_CloseAsyncFile((short)fileStream->fileHandle);

                v43 = fileStream->dwordB8;
                fileStream->fileHandle = v56;
                v58 = (unsigned __int64)((v43 + 7) & 0xFFFFFFF8) << 19;
                fileStream->qword0 = v58;
                fileStream->byteBC = (v43 == ((v43 + 7) & 0xFFFFFFF8)) + 1;
                fileStream->qword8 = v58 + pak->memoryData.patchHeaders[patchCount].m_sizeDisk;
            LABEL_84:
                if (v43 == v44)
                    return memoryData->patchSrcSize == 0;
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

    return memoryData->patchSrcSize == 0;
}

// sets patch variables for copying the next unprocessed page into the relevant segment buffer
// if this is a header page, fetch info from the next unprocessed asset and copy over the asset's header
bool SetupNextPageForPatching(PakLoadedInfo_t* a1, PakFile_t* pak)
{
    Rebuild_14043E030(pak);

    // numProcessedPointers has just been set in the above function call
    pak->memoryData.numShiftedPointers = pak->numProcessedPointers;

    if (pak->processedPageCount == pak->GetPageCount())
        return false;
    //break;

    const uint32_t highestProcessedPageIdx = pak->processedPageCount + pak->firstPageIdx;
    pak->processedPageCount++;

    int v26 = highestProcessedPageIdx - pak->GetPageCount();
    if (highestProcessedPageIdx < pak->GetPageCount())
        v26 = highestProcessedPageIdx;

    const PakPageHeader_t* const nextMemPageHeader = &pak->memoryData.pageHeaders[v26];
    if ((pak->memoryData.segmentHeaders[nextMemPageHeader->segmentIdx].typeFlags & (SF_TEMP | SF_CPU)) != 0)
    {
        pak->memoryData.patchSrcSize = nextMemPageHeader->dataSize;
        pak->memoryData.patchDstPtr = reinterpret_cast<char*>(pak->memoryData.memPageBuffers[v26]);

        return true;
        //continue;
    }

    // headers
    PakAsset_t* pakAsset = pak->memoryData.ppAssetEntries[pak->memoryData.someAssetCount];

    pak->memoryData.patchSrcSize = pakAsset->headerSize;
    int assetTypeIdx = pakAsset->HashTableIndexForAssetType();

    pak->memoryData.patchDstPtr = reinterpret_cast<char*>(a1->segmentBuffers[0]) + pak->memoryData.unkAssetTypeBindingSizes[assetTypeIdx];
    pak->memoryData.unkAssetTypeBindingSizes[assetTypeIdx] += g_pPakGlobals->m_assetBindings[assetTypeIdx].nativeClassSize;

    return true;
}

bool Pak_ProcessAssets(PakLoadedInfo_t* const a1)
{
    PakFile_t* const pak = a1->pakFile;
    while (pak->processedAssetCount != pak->GetAssetCount())
    {
        // TODO: invert condition and make the branch encompass the whole loop
        if (!(pak->memoryData.patchSrcSize + pak->memoryData.field_2A8))
        {
            const bool res = SetupNextPageForPatching(a1, pak);

            if (res)
                continue;
            else
                break;
        }

        if (!Pak_ProcessPakFile(pak))
            return false;

        // processedPageCount must be greater than 0 here otherwise the page index will be negative and cause a crash
        // if this happens, something probably went wrong with the patch data condition at the start of the loop, as that
        // function call should increment processedPageCount if it succeeded
        assert(pak->processedPageCount > 0);

        const uint32_t pageCount = pak->GetPageCount();
        const uint32_t v4 = (pak->firstPageIdx - 1) + pak->processedPageCount;

        uint32_t shiftedPageIndex = v4;

        if (v4 >= pageCount)
            shiftedPageIndex -= pageCount;

        // if "temp_" segment
        if ((pak->memoryData.segmentHeaders[pak->memoryData.pageHeaders[shiftedPageIndex].segmentIdx].typeFlags & (SF_TEMP | SF_CPU)) != 0)
        {
            const bool res = SetupNextPageForPatching(a1, pak);

            if (res)
                continue;
            else
                break;
        }

        PakAsset_t* asset = pak->memoryData.ppAssetEntries[pak->memoryData.someAssetCount];
        const uint32_t headPageOffset = asset->headPtr.offset;
        char* v8 = pak->memoryData.patchDstPtr - asset->headerSize;

        uint32_t newOffsetFromSegmentBufferToHeader = LODWORD(pak->memoryData.patchDstPtr)
            - asset->headerSize
            - LODWORD(a1->segmentBuffers[0]);
        asset->headPtr.offset = newOffsetFromSegmentBufferToHeader;

        uint32_t offsetSize = newOffsetFromSegmentBufferToHeader - headPageOffset;

        for (uint32_t i = pak->memoryData.numShiftedPointers; i < pak->GetPointerCount(); pak->memoryData.numShiftedPointers = i)
        {
            PakPage_t* ptr = &pak->memoryData.virtualPointers[i];

            ASSERT_PAKPTR_VALID(pak, ptr);

            if (ptr->index != shiftedPageIndex)
                break;

            const uint32_t offsetToPointer = ptr->offset - headPageOffset;
            if (offsetToPointer >= asset->headerSize)
                break;

            PakPage_t* pagePtr = reinterpret_cast<PakPage_t*>(v8 + offsetToPointer);

            ASSERT_PAKPTR_VALID(pak, ptr);

            ptr->offset += offsetSize;

            if (pagePtr->index == shiftedPageIndex)
                pagePtr->offset += offsetSize;

            i = pak->memoryData.numShiftedPointers + 1;
        }

        for (uint32_t j = 0; j < asset->dependenciesCount; ++j)
        {
            PakPage_t* descriptor = &pak->memoryData.guidDescriptors[asset->dependenciesIndex + j];

            if (descriptor->index == shiftedPageIndex)
                descriptor->offset += offsetSize;
        }

        const uint32_t v16 = ++pak->memoryData.someAssetCount;

        PakAsset_t* v17 = nullptr;
        if (v16 < pak->GetAssetCount() && (v17 = pak->memoryData.ppAssetEntries[v16], v17->headPtr.index == shiftedPageIndex))
        {
            pak->memoryData.field_2A8 = v17->headPtr.offset - headPageOffset - asset->headerSize;
            pak->memoryData.patchSrcSize = v17->headerSize;
            const uint8_t assetTypeIdx = v17->HashTableIndexForAssetType();

            pak->memoryData.patchDstPtr = reinterpret_cast<char*>(a1->segmentBuffers[0]) + pak->memoryData.unkAssetTypeBindingSizes[assetTypeIdx];

            pak->memoryData.unkAssetTypeBindingSizes[assetTypeIdx] += g_pPakGlobals->m_assetBindings[assetTypeIdx].nativeClassSize;
        }
        else
        {
            bool res = SetupNextPageForPatching(a1, pak);

            if (res)
                continue;
            else
                break;
        }
    }

    if (!JT_IsJobDone(pak->memoryData.unkJobID))
        return false;

    uint32_t i = 0;
    PakAsset_t* pAsset = nullptr;

    for (int j = pak->memoryData.pakId & PAK_MAX_HANDLES_MASK; i < pak->GetHeader().assetCount; a1->assetGuids[i - 1] = pAsset->guid)
    {
        pAsset = &pak->memoryData.assetEntries[i];
        if (pAsset->numRemainingDependencies)
        {
            //printf("[%s] processing deps for %llX (%.4s)\n", pak->GetName(), pAsset->guid, (char*)&pAsset->magic);
            Pak_ResolveAssetRelations(pak, pAsset);
            const int v36 = pak->memoryData.qword2E0[i];

            if (dword_167A40B3C[6 * g_pPakGlobals->m_assets[v36].unk_8] == j)
            {
                if (*qword_167ED7BC8)
                {
                    uint64_t v38 = 0;
                    if ((*qword_167ED7BC8)->unk_0)
                    {
                        int* v39 = (*qword_167ED7BC8)->unk_array_9D410;
                        while (*v39 != v36)
                        {
                            ++v38;
                            ++v39;
                            if (v38 >= (*qword_167ED7BC8)->unk_0)
                                goto LABEL_41;
                        }
                        goto LABEL_42;
                    }
                }
                else
                {
                    //printf("allocating thing\n");
                    *qword_167ED7BC8 = reinterpret_cast<UnknownPakStruct_t*>(AlignedMemAlloc()->Alloc(0x11D410, 8));
                    (*qword_167ED7BC8)->unk_0 = 0;
                    (*qword_167ED7BC8)->unk_4 = 0;
                    (*qword_167ED7BC8)->unk_8 = 0;
                }
            LABEL_41:
                (*qword_167ED7BC8)->unk_array_9D410[(*qword_167ED7BC8)->unk_0] = v36;
                ++(*qword_167ED7BC8)->unk_0;
            }
        }
    LABEL_42:
        ++i;
    }
    if (*qword_167ED7BC8)
        sub_14043D870(a1, 0);
    a1->status = EPakStatus::PAK_STATUS_LOADED;

    return true;
}

void Pak_StubInvalidAssetBinds(PakFile_t* const pak, PakSegmentDescriptor_t* const desc)
{
    for (uint32_t i = 0; i < pak->GetAssetCount(); ++i)
    {
        PakAsset_t* const asset = &pak->memoryData.assetEntries[i];
        pak->memoryData.ppAssetEntries[i] = asset;

        const uint8_t assetTypeIndex = asset->HashTableIndexForAssetType();
        desc->assetTypeCount[assetTypeIndex]++;

        PakAssetBinding_t* const assetBinding = &g_pPakGlobals->m_assetBindings[assetTypeIndex];

        if (assetBinding->type == PakAssetBinding_t::NONE)
        {
            assetBinding->extension = asset->magic;
            assetBinding->version = asset->version;
            assetBinding->description = "<unknown>";
            assetBinding->loadAssetFunc = nullptr;
            assetBinding->unloadAssetFunc = nullptr;
            assetBinding->replaceAssetFunc = nullptr;
            assetBinding->allocator = AlignedMemAlloc();
            assetBinding->headerSize = asset->headerSize;
            assetBinding->nativeClassSize = asset->headerSize;
            assetBinding->headerAlignment = pak->memoryData.pageHeaders[asset->headPtr.index].pageAlignment;
            assetBinding->type = PakAssetBinding_t::STUB;
        }

        // this is dev only because it could spam a lot on older paks
        // which isn't much help to the average user that can't rebuild other people's paks
        if (asset->version != assetBinding->version)
        {
            DevWarning(eDLL_T::RTECH,
                "Unexpected asset version for \"%s\" (%.4s) asset with guid 0x%llX (asset %i in pakfile '%s'). Expected %i, found %i.\n",
                assetBinding->description,
                reinterpret_cast<char*>(&asset->magic),
                asset->guid,
                i, pak->GetName(),
                assetBinding->version, asset->version
            );
        }
    }
}

bool Pak_StartLoadingPak(PakLoadedInfo_t* const loadedInfo)
{
    PakFile_t* const pakFile = loadedInfo->pakFile;

    if (pakFile->memoryData.patchSrcSize && !Pak_ProcessPakFile(pakFile))
        return false;

    PakSegmentDescriptor_t pakDescriptor = {};

    Pak_StubInvalidAssetBinds(pakFile, &pakDescriptor);

    const uint32_t numAssets = pakFile->GetAssetCount();

    if (pakFile->memoryData.pakHeader.patchIndex)
        pakFile->firstPageIdx = pakFile->memoryData.patchDataHeader->m_pageCount;

    sub_140442740(pakFile->memoryData.ppAssetEntries, &pakFile->memoryData.ppAssetEntries[numAssets], numAssets, pakFile);

    // pak must have no more than PAK_MAX_SEGMENTS segments as otherwise we will overrun the above "segmentSizes" array
    // and write to arbitrary locations on the stack
    if (pakFile->GetSegmentCount() > PAK_MAX_SEGMENTS)
    {
        Error(eDLL_T::RTECH, EXIT_FAILURE, "Too many segments in pakfile '%s'. Max %i, found %i.\n", pakFile->GetName(), PAK_MAX_SEGMENTS, pakFile->GetSegmentCount());
        return false;
    }

    Pak_AlignSegmentHeaders(pakFile, &pakDescriptor);
    Pak_AlignSegments(pakFile, &pakDescriptor);

    // allocate segment buffers with predetermined alignments; pages will be
    // copied into here
    for (int8_t i = 0; i < PAK_SEGMENT_BUFFER_TYPES; ++i)
    {
        if (pakDescriptor.segmentSizeForType[i])
            loadedInfo->segmentBuffers[i] = AlignedMemAlloc()->Alloc(pakDescriptor.segmentSizeForType[i], pakDescriptor.segmentAlignmentForType[i]);
    }

    Pak_CopyPagesToSegments(pakFile, loadedInfo, &pakDescriptor);

    const PakFileHeader_t& pakHdr = pakFile->GetHeader();

    if (*g_pUseAssetStreamingSystem)
    {
        Pak_LoadStreamingData(loadedInfo);
    }

    const __int64 v106 = pakHdr.descriptorCount + 2 * (pakHdr.patchIndex + pakHdr.assetCount + 4ull * pakHdr.assetCount + pakHdr.virtualSegmentCount);
    const __int64 patchDestOffset = pakHdr.GetTotalHeaderSize() + 2 * (pakHdr.patchIndex + 6ull * pakHdr.memPageCount + 4 * v106);

    pakFile->dword14 = 1;

    PakMemoryData_t& memoryData = pakFile->memoryData;

    memoryData.patchSrcSize = pakFile->memoryData.qword2D0 - patchDestOffset;
    memoryData.patchDstPtr = (char*)&pakHdr + patchDestOffset;

    loadedInfo->status = EPakStatus::PAK_STATUS_LOAD_PAKHDR;

    return true;
}


void V_PakParse::Detour(const bool bAttach) const
{
    DetourSetup(&v_Pak_LoadAsync, &Pak_LoadAsync, bAttach);
    DetourSetup(&v_Pak_UnloadAsync, &Pak_UnloadAsync, bAttach);

    DetourSetup(&v_Pak_StartLoadingPak, &Pak_StartLoadingPak, bAttach);

    DetourSetup(&v_Pak_ProcessPakFile, &Pak_ProcessPakFile, bAttach);
    DetourSetup(&v_Pak_ResolveAssetRelations, &Pak_ResolveAssetRelations, bAttach);
    DetourSetup(&v_Pak_ProcessAssets, &Pak_ProcessAssets, bAttach);

    DetourSetup(&sub_14043E030, &Rebuild_14043E030, bAttach);
}

// Symbols taken from R2 dll's.
PakLoadFuncs_t* g_pakLoadApi = nullptr;
