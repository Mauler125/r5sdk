#include "sdklauncher_utils.h"
#include "tier1/xorstr.h"
#include "tier1/utlmap.h"
#include "tier2/curlutils.h"
#include "zip/src/ZipFile.h"

// !TODO: perhaps this should be a core utility shared across
// the entire SDK to allow processes to restart them selfs.
void SDKLauncher_Restart()
{
	char currentPath[MAX_PATH];
	BOOL getResult = GetCurrentDirectoryA(sizeof(currentPath), currentPath);

	if (!getResult)
	{
		// TODO: dialog box and instruct user to manually open the launcher again.
		printf("%s: Failed to obtain current directory: error code = %08x\n", __FUNCTION__, GetLastError());
		ExitProcess(0xBADC0DE);
	}

	///////////////////////////////////////////////////////////////////////////
	STARTUPINFOA StartupInfo = { 0 };
	PROCESS_INFORMATION ProcInfo = { 0 };

	// Initialize startup info struct.
	StartupInfo.cb = sizeof(STARTUPINFOA);

	DWORD processId = GetProcessId(GetCurrentProcess());

	char commandLine[MAX_PATH];
	sprintf(commandLine, "%lu %s %s", processId, RESTART_DEPOT_DOWNLOAD_DIR, currentPath);

	BOOL createResult = CreateProcessA(
		"bin\\updater.exe",                            // lpApplicationName
		commandLine,                                   // lpCommandLine
		NULL,                                          // lpProcessAttributes
		NULL,                                          // lpThreadAttributes
		FALSE,                                         // bInheritHandles
		CREATE_SUSPENDED,                              // dwCreationFlags
		NULL,                                          // lpEnvironment
		currentPath,                                   // lpCurrentDirectory
		&StartupInfo,                                  // lpStartupInfo
		&ProcInfo                                      // lpProcessInformation
	);

	if (!createResult)
	{
		// TODO: dialog box and instruct user to update again.
		printf("%s: Failed to create update process: error code = %08x\n", __FUNCTION__, GetLastError());
		ExitProcess(0xBADC0DE);
	}

	///////////////////////////////////////////////////////////////////////////
	// Resume the process.
	ResumeThread(ProcInfo.hThread);

	///////////////////////////////////////////////////////////////////////////
	// Close the process and thread handles.
	CloseHandle(ProcInfo.hProcess);
	CloseHandle(ProcInfo.hThread);

	ExitProcess(EXIT_SUCCESS);
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
bool SDKLauncher_CreateDepotDirectories()
{
	if ((!CreateDirectoryA(BASE_PLATFORM_DIR, NULL)          && GetLastError() != ERROR_ALREADY_EXISTS) ||
	    (!CreateDirectoryA(DEFAULT_DEPOT_DOWNLOAD_DIR, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) ||
		(!CreateDirectoryA(RESTART_DEPOT_DOWNLOAD_DIR, NULL) && GetLastError() != ERROR_ALREADY_EXISTS))
	{
		return false;
	}

	return true;
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
bool SDKLauncher_ClearDepotDirectories()
{
	// Clear all depot files.
	if (!RemoveDirectoryA(RESTART_DEPOT_DOWNLOAD_DIR) ||
		!RemoveDirectoryA(DEFAULT_DEPOT_DOWNLOAD_DIR))
	{
		return false;
	}

	return true;
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
bool SDKLauncher_ExtractZipFile(const char* pZipFile, const char* pDestPath, CProgressPanel* pProgress)
{
	ZipArchive::Ptr archive = ZipFile::Open(pZipFile);
	size_t entries = archive->GetEntriesCount();

	CUtlMap<CUtlString, bool> fileList(UtlStringLessFunc);

	for (size_t i = 0; i < entries; ++i)
	{
		auto entry = archive->GetEntry(int(i));

		if (entry->IsDirectory())
			continue;

		string fullName = entry->GetFullName();

		// TODO: ideally there will be a list in a json
		// that the launcher downloads to determine what
		// has to be installed during the restart.
		bool installDuringRestart = false;
		if (fullName.compare("launcher.exe") == NULL)
		{
			installDuringRestart = true;
		}

		fileList.Insert(fullName.c_str(), installDuringRestart);
		printf("Added: %s\n", fullName.c_str());
	}

	printf("Num files: %d\n", fileList.Count());

	FOR_EACH_MAP(fileList, i)
	{
		CUtlString& fileName = fileList.Key(i);
		CUtlString absDirName = fileName.AbsPath();
		CUtlString dirName = absDirName.DirName();

		CreateDirectories(absDirName.Get());

		if (pProgress)
		{
			pProgress->SetExportLabel(Format("%s (%i of %i)", fileName.Get(), i + 1, fileList.Count()).c_str());

			int percentage = (i * 100) / fileList.Count();
			pProgress->UpdateProgress(percentage, false);
		}

		printf("Extracting: %s to %s\n", fileName.Get(), dirName.Get());

		if (fileList[i])
		{
			printf("File %s has to be installed after a restart!\n", fileName.Get());

			CUtlString tempDir = RESTART_DEPOT_DOWNLOAD_DIR;
			tempDir.Append(fileName);

			ZipFile::ExtractFile(pZipFile, fileName.Get(), tempDir.Get());
		}
		else
		{
			ZipFile::ExtractFile(pZipFile, fileName.Get(), fileName.Get());
		}
	}

	return true;
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
bool SDKLauncher_QueryServer(const char* url, string& outResponse, string& outMessage, CURLINFO& outStatus)
{
	curl_slist* sList = nullptr;
	CURLParams params;

	params.writeFunction = CURLWriteStringCallback;
	params.timeout = QUERY_TIMEOUT;
	params.verifyPeer = true;
	params.verbose = IsDebug();

	CURL* curl = CURLInitRequest(url, nullptr, outResponse, sList, params);

	if (!curl)
	{
		return false;
	}

	CURLcode res = CURLSubmitRequest(curl, sList);

	if (!CURLHandleError(curl, res, outMessage, true))
	{
		return false;
	}

	outStatus = CURLRetrieveInfo(curl);
	return true;
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
bool SDKLauncher_GetLatestReleaseManifest(const char* url, string& responseMessage,
	nlohmann::json& outManifest, const bool preRelease)
{
	string responseBody;
	CURLINFO status;

	if (!SDKLauncher_QueryServer(url, responseBody, responseMessage, status))
	{
		return false;
	}

	if (status != 200)
	{
		return false;
	}

	try
	{
		nlohmann::json responseJson = nlohmann::json::parse(responseBody);

		for (size_t i = 0; i < responseJson.size(); i++)
		{
			auto& release = responseJson[i];

			if (preRelease && release["prerelease"])
			{
				outManifest = release["assets"];
				break;
			}
			else if (!release["prerelease"])
			{
				outManifest = release["assets"];
				break;
			}

			if (i == responseJson.size() - 1 && outManifest.empty())
				release[0]["assets"]; // Just take the first one then.
		}
	}
	catch (const std::exception& ex)
	{
		printf("%s - Exception while parsing response:\n%s\n", __FUNCTION__, ex.what());
		return false;
	}

	Assert(!outManifest.empty());
	return true;
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
int SDKLauncher_ProgressCallback(CURLProgress* progessData, double dltotal,
	double dlnow, double ultotal, double ulnow)
{
	CProgressPanel* pDownloadSurface = (CProgressPanel*)progessData->cust;

	if (pDownloadSurface->IsCanceled())
		return -1;

	double downloaded;
	curl_easy_getinfo(progessData->curl, CURLINFO_SIZE_DOWNLOAD, &downloaded);

	pDownloadSurface->SetExportLabel(Format("%s (%llu of %llu)", progessData->name, (size_t)downloaded, progessData->size).c_str());

	size_t percentage = ((size_t)downloaded * 100) / progessData->size;
	pDownloadSurface->UpdateProgress((uint32_t)percentage, false);

	return 0;
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
void SDKLauncher_DownloadAsset(const char* url, const char* path, const char* fileName,
	const size_t fileSize, const char* options, CProgressPanel* pProgress)
{
	CURLParams params;

	params.writeFunction = CURLWriteFileCallback;
	params.statusFunction = SDKLauncher_ProgressCallback;

	CURLDownloadFile(url, path, fileName, options, fileSize, pProgress, params);
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
void SDKLauncher_BeginDownload(const bool bPreRelease, const bool bOptionalAssets,
	CUtlVector<CUtlString>& fileList, CProgressPanel* pProgress)
{
	string responseMesage;
	nlohmann::json manifest;

	// These files will NOT be downloaded from the release depots.
	std::set<string> blackList;
	blackList.insert("symbols.zip");

	//fileList.AddToTail("audio_0.zip");
	//fileList.AddToTail("audio_1.zip");
	//fileList.AddToTail("binaries.zip");
	//fileList.AddToTail("materials.zip");
	//fileList.AddToTail("media.zip");
	//fileList.AddToTail("paks.zip");
	//fileList.AddToTail("starpak_0.zip");
	//fileList.AddToTail("starpak_1.zip");
	//fileList.AddToTail("stbsp.zip");
	//fileList.AddToTail("depot.zip");

	//// Download core game files.
	//if (!SDKLauncher_GetLatestReleaseManifest(XorStr("https://api.github.com/repos/SlaveBuild/N1094_CL456479/releases"), responseMesage, manifest, bPreRelease))
	//{
	//	// TODO: Error dialog.
	//	return;
	//}
	//SDKLauncher_DownloadAssetList(fileList, manifest, blackList, DEFAULT_DEPOT_DOWNLOAD_DIR, pProgress);

	//if (pProgress->IsCanceled())
	//	return;

	// Download SDK files.
	if (!SDKLauncher_GetLatestReleaseManifest(XorStr("https://api.github.com/repos/Mauler125/r5sdk/releases"), responseMesage, manifest, bPreRelease))
	{
		// TODO: Error dialog.
		return;
	}
	SDKLauncher_DownloadAssetList(fileList, manifest, blackList, DEFAULT_DEPOT_DOWNLOAD_DIR, pProgress);

	if (pProgress->IsCanceled())
		return;
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
void SDKLauncher_DownloadAssetList(CUtlVector<CUtlString>& fileList, nlohmann::json& assetList,
	std::set<string>& blackList, const char* pPath, CProgressPanel* pProgress)
{
	int i = 1;
	for (auto& asset : assetList)
	{
		if (pProgress->IsCanceled())
		{
			break;
		}

		if (!asset.contains("browser_download_url") ||
			!asset.contains("name") ||
			!asset.contains("size"))
		{
			Assert(0);
			return;
		}

		const string fileName = asset["name"];
		const char* const pFileName = fileName.c_str();

		// Asset is filtered, don't download.
		if (blackList.find(fileName) != blackList.end())
			continue;

		const string downloadLink = asset["browser_download_url"];
		const size_t fileSize = asset["size"];

		pProgress->SetText(Format("Downloading package %i of %i...", i, assetList.size()).c_str());
		SDKLauncher_DownloadAsset(downloadLink.c_str(), pPath, pFileName, fileSize, "wb+", pProgress);

		// Check if its a zip file, as these are
		// the only files that will be installed.
		if (V_strcmp(V_GetFileExtension(pFileName), "zip") == NULL)
		{
			CUtlString filePath = pPath;

			filePath.AppendSlash();
			filePath.Append(fileName.c_str());

			fileList.AddToTail(filePath);
		}

		i++;
	}
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
void SDKLauncher_InstallAssetList(const bool bOptionalAssets,
	CUtlVector<CUtlString>& fileList, CProgressPanel* pProgress)
{
	// Install process cannot be canceled.
	pProgress->SetCanCancel(false);

	FOR_EACH_VEC(fileList, i)
	{
		pProgress->SetText(Format("Installing package %i of %i...", i + 1, fileList.Count()).c_str());

		CUtlString& fileName = fileList[i];
		SDKLauncher_ExtractZipFile(fileName.Get(), "", pProgress);
	}
}

//----------------------------------------------------------------------------
// Purpose: checks if client has enough disk space
// Input  : minRequiredSpace - 
//        : *availableSize   - 
// Output : true if space is sufficient, false otherwise
//----------------------------------------------------------------------------
bool SDKLauncher_CheckDiskSpace(const int minRequiredSpace, int* const availableSize)
{
	char currentDir[MAX_PATH]; // Get current dir.
	GetCurrentDirectoryA(sizeof(currentDir), currentDir);

	// Does this disk have enough space?
	ULARGE_INTEGER avaliableSize;
	GetDiskFreeSpaceEx(currentDir, &avaliableSize, nullptr, nullptr);

	const int currentAvailSpace = (int)(avaliableSize.QuadPart / uint64_t(1024 * 1024 * 1024));

	if (availableSize)
	{
		*availableSize = currentAvailSpace;
	}

	if (currentAvailSpace < minRequiredSpace)
	{
		return false;
	}

	return true;
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
bool SDKLauncher_CheckForUpdate()
{
	return true;
}
