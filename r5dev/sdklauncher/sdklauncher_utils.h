#pragma once
#include "download_surface.h"

extern float g_flUpdateCheckRate;

void SDKLauncher_Restart();

bool SDKLauncher_CreateDepotDirectories();
bool SDKLauncher_ClearDepotDirectories();

bool SDKLauncher_ExtractZipFile(const char* pZipFile, const char* pDestPath, CProgressPanel* pProgress = nullptr);
void SDKLauncher_BeginDownload(const bool bPreRelease, const bool bOptionalAssets, const bool bSdkOnly, CUtlVector<CUtlString>& fileList, CProgressPanel* pProgress = nullptr);

bool SDKLauncher_DownloadAssetList(CUtlVector<CUtlString>& fileList, nlohmann::json& assetList,
	std::set<string>& blackList, const char* pPath, CProgressPanel* pProgress);

bool SDKLauncher_InstallAssetList(const bool bOptionalAssets,
	CUtlVector<CUtlString>& fileList, CProgressPanel* pProgress);

bool SDKLauncher_CheckDiskSpace(const int minRequiredSpace, int* const availableSize = nullptr);
bool SDKLauncher_CheckForUpdate(const bool bPreRelease);
