//=============================================================================//
//
// Purpose: async file loading, unloading and the management thereof
//
//=============================================================================//
#include "rtech/ipakfile.h"
#include "rtech/pak/paktools.h"
#include "asyncio.h"

//----------------------------------------------------------------------------------
// open a file and add it to the async file handle array
//----------------------------------------------------------------------------------
int FS_OpenAsyncFile(const char* const filePath, const int logLevel, size_t* const fileSizeOut)
{
    const CHAR* fileToLoad = filePath;
    char overridePath[1024];

    // is this a pak file and do we have an override
    if (strstr(fileToLoad, PLATFORM_PAK_PATH) &&
        Pak_FileOverrideExists(fileToLoad, overridePath, sizeof(overridePath)))
    {
        fileToLoad = overridePath;
    }

    const HANDLE hFile = CreateFileA(fileToLoad, GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_DELETE, 0, OPEN_EXISTING, FILE_SUPPORTS_GHOSTING, 0);

    if (hFile == INVALID_HANDLE_VALUE)
        return FS_ASYNC_FILE_INVALID;

    if (fileSizeOut)
    {
        // get the size of the file we just opened
        LARGE_INTEGER fileSize;

        if (GetFileSizeEx(hFile, &fileSize))
            *fileSizeOut = fileSize.QuadPart;
    }

    const int fileIdx = g_pAsyncFileSlotMgr->FindSlot();
    const int slotNum = (fileIdx & ASYNC_MAX_FILE_HANDLES_MASK);

    AsyncHandleTracker_t& tracker = g_pAsyncFileSlots[slotNum];

    tracker.m_nFileNumber = fileIdx;
    tracker.m_hFileHandle = hFile;
    tracker.m_nCurOfs = 1;

    if (async_debug_level->GetInt() >= logLevel)
        Msg(eDLL_T::RTECH, "%s: Opened file: '%s' to slot #%d\n", __FUNCTION__, fileToLoad, slotNum);

    return fileIdx;
}

//----------------------------------------------------------------------------------
// close a file and remove it from the async file handle array
//----------------------------------------------------------------------------------
void FS_CloseAsyncFile(const short fileHandle)
{
    const int slotNum = fileHandle & ASYNC_MAX_FILE_HANDLES_MASK;
    AsyncHandleTracker_t& tracker = g_pAsyncFileSlots[slotNum];

    if (ThreadInterlockedExchangeAdd(&tracker.m_nCurOfs, 0xFFFFFFFF) <= 1)
    {
        CloseHandle(tracker.m_hFileHandle);
        tracker.m_hFileHandle = INVALID_HANDLE_VALUE;

        g_pAsyncFileSlotMgr->FreeSlot(slotNum);

        if (async_debug_close->GetBool())
            Msg(eDLL_T::RTECH, "%s: Closed file from slot #%d\n", __FUNCTION__, slotNum);
    }
}

///////////////////////////////////////////////////////////////////////////////
void V_AsyncIO::Detour(const bool bAttach) const
{
    DetourSetup(&v_FS_OpenAsyncFile, &FS_OpenAsyncFile, bAttach);
    DetourSetup(&v_FS_CloseAsyncFile, &FS_CloseAsyncFile, bAttach);
}
