#pragma once
#include "download_surface.h"

void SDKLauncher_Restart();

bool SDKLauncher_CreateDepotDirectories();
bool SDKLauncher_ClearDepotDirectories();

bool SDKLauncher_ExtractZipFile(const char* pZipFile, const char* pDestPath, CProgressPanel* pProgress = nullptr);
void SDKLauncher_BeginDownload(const bool bPreRelease, const bool bOptionalAssets, CUtlVector<CUtlString>& fileList, CProgressPanel* pProgress = nullptr);

void SDKLauncher_DownloadAssetList(CUtlVector<CUtlString>& fileList, nlohmann::json& assetList,
	std::set<string>& blackList, const char* pPath, CProgressPanel* pProgress);

void SDKLauncher_InstallAssetList(const bool bOptionalAssets,
	CUtlVector<CUtlString>& fileList, CProgressPanel* pProgress);

bool SDKLauncher_CheckDiskSpace(const int minRequiredSpace, int* const availableSize = nullptr);
