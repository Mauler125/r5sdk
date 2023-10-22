#pragma once
#include "download_surface.h"

typedef CUtlMap<CUtlString, CUtlVector<CUtlString>*> DepotChangedList_t;

extern float g_flUpdateCheckRate;

void SDKLauncher_Restart();

bool SDKLauncher_CreateDepotDirectories();
bool SDKLauncher_ClearDepotDirectories();

bool SDKLauncher_ExtractZipFile(nlohmann::json& manifest, const CUtlString& filePath, DepotChangedList_t* changedList, CProgressPanel* pProgress);
bool SDKLauncher_BeginInstall(const bool bPreRelease, const bool bOptionalDepots,
	CUtlVector<CUtlString>& zipList, CUtlString* errorMessage, CProgressPanel* pProgress);

bool SDKLauncher_IsManifestValid(const nlohmann::json& depotManifest);
bool SDKLauncher_IsDepositoryValid(const nlohmann::json& depotAssetList);

bool SDKLauncher_DownloadDepotList(nlohmann::json& manifest, CUtlVector<CUtlString>& depotList,
	CUtlVector<CUtlString>& outZipList, CUtlString* errorMessage, CProgressPanel* pProgress, const char* pPath,
	const bool bOptionalDepots);

bool SDKLauncher_InstallDepotList(nlohmann::json& manifest, CUtlVector<CUtlString>& depotList,
	DepotChangedList_t* fileList, CProgressPanel* pProgress);

bool SDKLauncher_GetRemoteManifest(const char* url, string& responseMessage, nlohmann::json& remoteManifest, const bool bPreRelease);
bool SDKLauncher_GetLocalManifest(nlohmann::json& localManifest);
bool SDKLauncher_WriteLocalManifest(const nlohmann::json& localManifest);

bool SDKLauncher_CheckDiskSpace(const int minRequiredSpace, int* const availableSize = nullptr);
bool SDKLauncher_CheckForUpdate(const bool bPreRelease);

bool SDKLauncher_ForceExistingInstanceOnTop();


