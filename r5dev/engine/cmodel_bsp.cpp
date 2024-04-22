//=============================================================================//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "core/stdafx.h"
#include "tier0/memstd.h"
#include "tier0/jobthread.h"
#include "tier1/fmtstr.h"
#include "tier1/keyvalues.h"
#include "tier2/fileutils.h"
#include "engine/sys_dll2.h"
#include "engine/host_cmd.h"
#include "engine/cmodel_bsp.h"

#include "rtech/pak/pakstate.h"
#include "rtech/pak/pakparse.h"
#include "rtech/pak/paktools.h"
#include "rtech/pak/pakstream.h"

#include "vpklib/packedstore.h"
#include "datacache/mdlcache.h"
#include "filesystem/filesystem.h"
#ifndef DEDICATED
#include "client/clientstate.h"
#endif // !DEDICATED

CUtlVector<CUtlString> g_InstalledMaps;
CFmtStrN<MAX_MAP_NAME> s_CurrentLevelName;

static CustomPakData_t s_customPakData;
static KeyValues* s_pLevelSetKV = nullptr;

//-----------------------------------------------------------------------------
// Purpose: load a custom pak and add it to the list
//-----------------------------------------------------------------------------
PakHandle_t CustomPakData_t::LoadAndAddPak(const char* const pakFile)
{
    if (numHandles >= MAX_CUSTOM_PAKS)
    {
        Error(eDLL_T::ENGINE, NO_ERROR, "Tried to load pak '%s', but already reached the SDK's limit of %d!\n", pakFile, MAX_CUSTOM_PAKS);
        return PAK_INVALID_HANDLE;
    }

    const PakHandle_t pakId = g_pakLoadApi->LoadAsync(pakFile, AlignedMemAlloc(), 4, 0);

    // failure, don't add and return the invalid handle.
    if (pakId == PAK_INVALID_HANDLE)
        return pakId;

    handles[numHandles++] = pakId;
    return pakId;
}

//-----------------------------------------------------------------------------
// Purpose: unloads all active custom pak handles
//-----------------------------------------------------------------------------
void CustomPakData_t::UnloadAndRemoveAll()
{
    for (; numHandles-1 >= CustomPakData_t::PAK_TYPE_COUNT; numHandles--)
    {
        const PakHandle_t pakId = handles[numHandles-1];

        if (pakId == PAK_INVALID_HANDLE)
        {
            assert(0); // invalid handles should not be inserted
            return;
        }

        g_pakLoadApi->UnloadAsync(pakId);
        handles[numHandles-1] = PAK_INVALID_HANDLE;
    }
}

//-----------------------------------------------------------------------------
// Purpose: loads the base SDK pak file by type
//-----------------------------------------------------------------------------
PakHandle_t CustomPakData_t::LoadBasePak(const char* const pakFile, const EPakType type)
{
    const PakHandle_t pakId = g_pakLoadApi->LoadAsync(pakFile, AlignedMemAlloc(), 4, 0);

    // the file is most likely missing
    assert(pakId != PAK_INVALID_HANDLE);
    handles[type] = pakId;

    return pakId;
}

//-----------------------------------------------------------------------------
// Purpose: unload the SDK base pak file by type
//-----------------------------------------------------------------------------
void CustomPakData_t::UnloadBasePak(const EPakType type)
{
    const PakHandle_t pakId = handles[type];

    // only unload if it was actually successfully loaded
    if (pakId != PAK_INVALID_HANDLE)
    {
        g_pakLoadApi->UnloadAsync(pakId);
        handles[type] = PAK_INVALID_HANDLE;
    }
}

//-----------------------------------------------------------------------------
// Purpose: checks if level has changed
// Input  : *pszLevelName - 
// Output : true if level name deviates from previous level
//-----------------------------------------------------------------------------
bool Mod_LevelHasChanged(const char* const pszLevelName)
{
    return (V_strcmp(pszLevelName, s_CurrentLevelName.String()) != NULL);
}

//-----------------------------------------------------------------------------
// Purpose: gets all installed maps
//-----------------------------------------------------------------------------
void Mod_GetAllInstalledMaps()
{
    CUtlVector<CUtlString> fileList;
    AddFilesToList(fileList, "vpk", "vpk", nullptr, '/');

    std::cmatch regexMatches;
    std::lock_guard<std::mutex> l(g_InstalledMapsMutex);

    g_InstalledMaps.Purge(); // Clear current list.

    FOR_EACH_VEC(fileList, i)
    {
        const CUtlString& filePath = fileList[i];
        const char* pFileName = strrchr(filePath.Get(), '/')+1;

        // Should always point right in front of the last
        // slash, as the files are loaded from 'vpk/'.
        Assert(pFileName);

        std::regex_search(pFileName, regexMatches, g_VpkDirFileRegex);

        if (!regexMatches.empty())
        {
            const std::sub_match<const char*>& match = regexMatches[2];

            if (match.compare("frontend") == 0)
                continue; // Frontend contains no BSP's.

            else if (match.compare("mp_common") == 0)
            {
                if (!g_InstalledMaps.HasElement("mp_lobby"))
                    g_InstalledMaps.AddToTail("mp_lobby");

                continue; // Common contains mp_lobby.
            }
            else
            {
                const string mapName = match.str();

                if (!g_InstalledMaps.HasElement(mapName.c_str()))
                    g_InstalledMaps.AddToTail(mapName.c_str());
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Purpose: processes queued pak files
//-----------------------------------------------------------------------------
void Mod_QueuedPakCacheFrame()
{
#ifndef DEDICATED
    bool bUnconnected = !(*g_pClientState_Shifted)->IsConnected();
#else // !DEDICATED
    bool bUnconnected = true; // Always true for dedicated.
#endif

    bool startFromFirst = false;

    if (Pak_StreamingDownloadFinished() && Pak_GetNumStreamableAssets() && bUnconnected)
    {
        *g_pPakPrecacheJobFinished = false;
        startFromFirst = true;
    }
    else if (*g_pPakPrecacheJobFinished)
    {
        return;
    }

    if (!FileSystem()->ResetItemCache() || *g_pNumPrecacheItemsMTVTF)
    {
        return;
    }

    const char** pPakName = &g_commonPakData[0].basePakName;
    int i;

    for (i = 0; i < 5; ++i)
    {
        if (*((_BYTE*)pPakName - 268))
            break;

        const char* pakName = g_commonPakData[i].pakName;
        const int64_t v4 = *pPakName - pakName;

        int v5;
        int v6;

        do
        {
            v5 = (unsigned __int8)pakName[v4];
            v6 = (unsigned __int8)*pakName - v5;
            if (v6)
                break;

            ++pakName;
        } while (v5);

        if (v6)
            break;

        pPakName += 35;
    }

    int startIndex = 0;

    if (!startFromFirst)
        startIndex = i; // start from last pre-cached

    const int numToProcess = startIndex;

    if (startIndex <= 4)
    {
        int numLeftToProcess = 4;
        CommonPakData_t* data = &g_commonPakData[4];

        do
        {
            if (*data->pakName)
            {
                PakLoadedInfo_s* const pakInfo = Pak_GetPakInfo(data->pakId);
                PakStatus_e status;

                // TODO: revisit this, this appears incorrect but also the way
                // respawn does this. it this always supposed to be true on
                // retail builds?
                bool keepLoaded = true;
                data->keepLoaded = true;

                if (pakInfo->handle == data->pakId)
                {
                    status = pakInfo->status;
                    keepLoaded = data->keepLoaded;
                }
                else
                {
                    status = PAK_STATUS_INVALID_PAKHANDLE;
                    keepLoaded = true;
                }

                if (!keepLoaded || status == PAK_STATUS_LOADED)
                {
                    // SDK pak files must be unloaded before the engine pak files,
                    // as we use assets within engine pak files.
                    switch (numLeftToProcess)
                    {
#ifndef DEDICATED
                    case CommonPakData_t::PAK_TYPE_UI_GM:
                        s_customPakData.UnloadBasePak(CustomPakData_t::PAK_TYPE_UI_SDK);
                        break;
#endif // !DEDICATED

                    case CommonPakData_t::PAK_TYPE_COMMON:
                        g_StudioMdlFallbackHandler.Clear();
                        break;

                    case CommonPakData_t::PAK_TYPE_COMMON_GM:
                        s_customPakData.UnloadBasePak(CustomPakData_t::PAK_TYPE_COMMON_SDK);
                        break;

                    case CommonPakData_t::PAK_TYPE_LOBBY:
                        s_customPakData.basePaksLoaded = false;
                        break;

                    default:
                        break;
                    }

                    // The old gather props is set if a model couldn't be
                    // loaded properly. If we unload level assets, we just
                    // enable the new implementation again and re-evaluate
                    // on the next level load. If we load a missing/bad
                    // model again, we toggle the old implementation as
                    // otherwise the fallback models won't render; the new
                    // gather props solution does not attempt to obtain
                    // studio hardware data on bad mdl handles. See
                    // 'GatherStaticPropsSecondPass_PreInit()' for details.
                    g_StudioMdlFallbackHandler.DisableLegacyGatherProps();

                    g_pakLoadApi->UnloadAsync(data->pakId);

                    Mod_UnloadPakFile(); // Unload mod pak files.

                    if (s_pLevelSetKV)
                    {
                        // Delete current level settings if we drop all paks..
                        s_pLevelSetKV->DeleteThis();
                        s_pLevelSetKV = nullptr;
                    }
                }

                if (status && (unsigned int)(status - 13) > 1)
                    return;

                data->keepLoaded = false;
                data->pakName[0] = '\0';

                data->pakId = PAK_INVALID_HANDLE;
            }
            --numLeftToProcess;
            --data;
        } while (numLeftToProcess >= numToProcess);
    }

    *g_pPakPrecacheJobFinished = true;
    CommonPakData_t* commonData = g_commonPakData;

    int it = 0;

    char* name;
    char* nameIt;

    while (true)
    {
        name = g_commonPakData[it].pakName;
        nameIt = name;
        char c;
        int v20;
        do
        {
            c = (unsigned __int8)nameIt[(unsigned __int64)(commonData->basePakName - (const char*)name)];
            v20 = (unsigned __int8)*nameIt - c;
            if (v20)
                break;

            ++nameIt;
        } while (c);

        if (!v20)
            goto CHECK_FOR_FAILURE;

        V_strncpy(name, commonData->basePakName, MAX_PATH);

        if (*commonData->pakName)
            break;

        commonData->pakId = PAK_INVALID_HANDLE;
    LOOP_AGAIN_OR_FINISH:

        ++it;
        ++commonData;
        if (it >= 5)
        {
            if (*g_pPakPrecacheJobFinished)
            {
                __int64 pMTVFTaskItem = *g_pMTVFTaskItem;
                if (pMTVFTaskItem)
                {
                    if (!*(_BYTE*)(pMTVFTaskItem + 4))
                    {
                        JobFifoLock_s* const pakFifoLock = &g_pakGlobals->fifoLock;

                        if (g_pakGlobals->hasPendingUnloadJobs || g_pakGlobals->loadedPakCount != g_pakGlobals->requestedPakCount)
                        {
                            if (!JT_AcquireFifoLockOrHelp(pakFifoLock)
                                && !JT_HelpWithJobTypes(g_pPakFifoLockWrapper, pakFifoLock, -1i64, 0i64))
                            {
                                JT_HelpWithJobTypesOrSleep(g_pPakFifoLockWrapper, pakFifoLock, -1i64, 0i64, 0i64, 1);
                            }

                            Mod_UnloadPendingAndPrecacheRequestedPaks();

                            if (ThreadInMainThread())
                            {
                                if (*g_bPakFifoLockAcquired)
                                {
                                    *g_bPakFifoLockAcquired = 0;
                                    JT_ReleaseFifoLock(pakFifoLock);
                                }
                            }

                            JT_ReleaseFifoLock(pakFifoLock);

                            pMTVFTaskItem = *g_pMTVFTaskItem;
                        }

                        FileSystem()->ResetItemCacheSize(256);
                        FileSystem()->PrecacheTaskItem(pMTVFTaskItem);
                    }
                }
            }
            return;
        }
    }

    if (it == CommonPakData_t::PAK_TYPE_LOBBY)
        s_customPakData.basePaksLoaded = true;

    if (s_customPakData.basePaksLoaded && !s_customPakData.levelResourcesLoaded)
    {
        Mod_PreloadLevelPaks(s_CurrentLevelName.String());
        s_customPakData.levelResourcesLoaded = true;
    }

    commonData->pakId = g_pakLoadApi->LoadAsync(name, AlignedMemAlloc(), 4, 0);

#ifndef DEDICATED
    if (it == CommonPakData_t::PAK_TYPE_UI_GM)
        s_customPakData.LoadBasePak("ui_sdk.rpak", CustomPakData_t::PAK_TYPE_UI_SDK);
#endif // !DEDICATED
    if (it == CommonPakData_t::PAK_TYPE_COMMON_GM)
        s_customPakData.LoadBasePak("common_sdk.rpak", CustomPakData_t::PAK_TYPE_COMMON_SDK);

CHECK_FOR_FAILURE:

    if (commonData->pakId != PAK_INVALID_HANDLE)
    {
        const PakLoadedInfo_s* const pli = Pak_GetPakInfo(commonData->pakId);

        if (pli->handle != commonData->pakId || ((pli->status - 9) & 0xFFFFFFFB) != 0)
        {
            *g_pPakPrecacheJobFinished = false;
            return;
        }
    }

    goto LOOP_AGAIN_OR_FINISH;
}

//-----------------------------------------------------------------------------
// Purpose: load assets for level with fifolock.
// Input  : *szLevelName - 
// Output : true on success, false on failure
//-----------------------------------------------------------------------------
void Mod_LoadPakForMap(const char* const pszLevelName)
{
	if (Mod_LevelHasChanged(pszLevelName))
        s_customPakData.levelResourcesLoaded = false;

	s_CurrentLevelName = pszLevelName;

	// Dedicated should not load loadscreens.
#ifndef DEDICATED
	v_Mod_LoadPakForMap(pszLevelName);
#endif // !DEDICATED
}

//-----------------------------------------------------------------------------
// Purpose: loads the level settings file, returns current if level hasn't changed.
// Input  : *pszLevelName - 
// Output : KeyValues*
//-----------------------------------------------------------------------------
KeyValues* Mod_GetLevelSettings(const char* const pszLevelName)
{
    if (s_pLevelSetKV)
    {
        // If we didn't change the level, return the current one
        if (s_customPakData.levelResourcesLoaded)
            return s_pLevelSetKV;

        s_pLevelSetKV->DeleteThis();
    }

    char szPathBuffer[MAX_PATH];
    snprintf(szPathBuffer, sizeof(szPathBuffer), "scripts/levels/settings/%s.kv", pszLevelName);

    s_pLevelSetKV = FileSystem()->LoadKeyValues(IFileSystem::TYPE_LEVELSETTINGS, szPathBuffer, "GAME");
    return s_pLevelSetKV;
}

//-----------------------------------------------------------------------------
// Purpose: loads required pakfile assets for specified BSP level
// Input  : &svSetFile - 
//-----------------------------------------------------------------------------
void Mod_PreloadLevelPaks(const char* const pszLevelName)
{
    KeyValues* const pSettingsKV = Mod_GetLevelSettings(pszLevelName);

    if (!pSettingsKV)
        return;

    KeyValues* const pPakListKV = pSettingsKV->FindKey("PakList");

    if (!pPakListKV)
        return;

    char szPathBuffer[MAX_PATH];

    for (KeyValues* pSubKey = pPakListKV->GetFirstSubKey(); pSubKey != nullptr; pSubKey = pSubKey->GetNextKey())
    {
        if (!pSubKey->GetBool())
            continue;

        snprintf(szPathBuffer, sizeof(szPathBuffer), "%s.rpak", pSubKey->GetName());
        const PakHandle_t nPakId = s_customPakData.LoadAndAddPak(szPathBuffer);

        if (nPakId == PAK_INVALID_HANDLE)
            Error(eDLL_T::ENGINE, NO_ERROR, "%s: unable to load pak '%s' results '%d'\n", __FUNCTION__, szPathBuffer, nPakId);
    }
}

//-----------------------------------------------------------------------------
// Purpose: unloads all pakfiles loaded by the SDK
//-----------------------------------------------------------------------------
void Mod_UnloadPakFile(void)
{
    s_customPakData.UnloadAndRemoveAll();

    g_StudioMdlFallbackHandler.ClearBadModelHandleCache();
    g_StudioMdlFallbackHandler.ClearSuppresionList();
}

void VModel_BSP::Detour(const bool bAttach) const
{
	DetourSetup(&v_Mod_LoadPakForMap, &Mod_LoadPakForMap, bAttach);
	DetourSetup(&v_Mod_QueuedPakCacheFrame, &Mod_QueuedPakCacheFrame, bAttach);
}
