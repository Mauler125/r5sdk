#include "sdklauncher_utils.h"
#include "windows/window.h"
#include "tier1/xorstr.h"
#include "tier1/utlmap.h"
#include "tier2/curlutils.h"
#include "zip/src/ZipFile.h"

bool g_bPartialInstall = false;
//bool g_bExperimentalBuilds = false;
float g_flUpdateCheckRate = 64;

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
	params.verbose = 0;// IsDebug();

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
		responseMessage = responseBody;
		return false;
	}

	if (status != 200)
	{
		responseMessage = responseBody;
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
				outManifest = release;
				break;
			}
			else if (!release["prerelease"])
			{
				outManifest = release;
				break;
			}

			if (i == responseJson.size() - 1 && outManifest.empty())
				release[0]; // Just take the first one then.
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
	{
		pDownloadSurface->Close();
		return -1;
	}

	double downloaded;
	curl_easy_getinfo(progessData->curl, CURLINFO_SIZE_DOWNLOAD, &downloaded);

	pDownloadSurface->SetExportLabel(Format("%s (%s of %s)", progessData->name,
		FormatBytes((size_t)downloaded).c_str(), FormatBytes(progessData->size).c_str()).c_str());

	size_t percentage = ((size_t)downloaded * 100) / progessData->size;
	pDownloadSurface->UpdateProgress((uint32_t)percentage, false);

	return 0;
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
bool SDKLauncher_DownloadAsset(const char* url, const char* path, const char* fileName,
	const size_t fileSize, const char* options, CProgressPanel* pProgress)
{
	CURLParams params;

	params.writeFunction = CURLWriteFileCallback;
	params.statusFunction = SDKLauncher_ProgressCallback;

	return CURLDownloadFile(url, path, fileName, options, fileSize, pProgress, params);
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
void SDKLauncher_BeginDownload(const bool bPreRelease, const bool bOptionalAssets,
	const bool bSdkOnly/*!!! REFACTOR ME MAYBE !!!*/, CUtlVector<CUtlString>& fileList, CProgressPanel* pProgress)
{
	string responseMessage;
	nlohmann::json manifest;

	// These files will NOT be downloaded from the release depots.
	std::set<string> blackList;
	blackList.insert("symbols.zip");


	// DEBUG CODE!!!
	//fileList.AddToTail("audio_0.zip");
	//fileList.AddToTail("audio_1.zip");
	//fileList.AddToTail("binaries.zip");
	//fileList.AddToTail("materials.zip");
	//fileList.AddToTail("media.zip");
	//fileList.AddToTail("paks.zip");
	//fileList.AddToTail("starpak_0.zip");
	//fileList.AddToTail("starpak_1.zip");
	//fileList.AddToTail("stbsp.zip");
	//fileList.AddToTail("/depot.zip");

	//FOR_EACH_VEC(fileList, i)
	//{
	//	CUtlString& filePath = fileList[i];
	//	filePath = filePath.Replace("/", DEFAULT_DEPOT_DOWNLOAD_DIR);
	//}

	if (!bSdkOnly)
	{
		// Download core game files.
		if (!SDKLauncher_GetLatestReleaseManifest(XorStr(GAME_DEPOT_VENDOR), responseMessage, manifest, bPreRelease))
		{
			// TODO: Error dialog.
			return;
		}
		SDKLauncher_DownloadAssetList(fileList, manifest, blackList, DEFAULT_DEPOT_DOWNLOAD_DIR, pProgress);

		if (pProgress->IsCanceled())
			return;
	}

	// Download SDK files.
	if (!SDKLauncher_GetLatestReleaseManifest(XorStr(SDK_DEPOT_VENDOR), responseMessage, manifest, bPreRelease))
	{
		// TODO: Error dialog.
		return;
	}
	SDKLauncher_DownloadAssetList(fileList, manifest, blackList, DEFAULT_DEPOT_DOWNLOAD_DIR, pProgress);

	if (pProgress->IsCanceled())
	{
		return;
	}
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
bool SDKLauncher_DownloadAssetList(CUtlVector<CUtlString>& fileList, nlohmann::json& assetList,
	std::set<string>& blackList, const char* pPath, CProgressPanel* pProgress)
{
	if (!assetList.contains("assets"))
	{
		Assert(0);
		return false;
	}

	int i = 1;
	const auto assetListArray = assetList["assets"];

	for (auto& asset : assetListArray)
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
			return false;
		}

		const string fileName = asset["name"];
		const char* const pFileName = fileName.c_str();

		// Asset is filtered, don't download.
		if (blackList.find(fileName) != blackList.end())
		{
			continue;
		}

		const string downloadLink = asset["browser_download_url"];
		const size_t fileSize = asset["size"];

		pProgress->SetText(Format("Downloading package %i of %i...", i, assetListArray.size()).c_str());
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

	return true;
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
bool SDKLauncher_InstallAssetList(const bool bOptionalAssets,
	CUtlVector<CUtlString>& fileList, CProgressPanel* pProgress)
{
	// Install process cannot be canceled.
	pProgress->SetCanCancel(false);

	FOR_EACH_VEC(fileList, i)
	{
		pProgress->SetText(Format("Installing package %i of %i...", i + 1, fileList.Count()).c_str());

		CUtlString& fileName = fileList[i];
		if (!SDKLauncher_ExtractZipFile(fileName.Get(), "", pProgress))
			return false;
	}

	return true;
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

bool SDKLauncher_GetLocalManifest(nlohmann::json& localManifest)
{
	const char* pManifestFileName = BASE_PLATFORM_DIR DEPOT_MANIFEST_FILE;

	if (!fs::exists(pManifestFileName))
		return false;

	ifstream localFile(pManifestFileName);

	if (!localFile.good())
		return false;

	try
	{
		localManifest = nlohmann::json::parse(localFile);
	}
	catch (const std::exception& ex)
	{
		printf("%s - Exception while parsing manifest:\n%s\n", __FUNCTION__, ex.what());
		return false;
	}

	return !localManifest.empty();
}

//bool SDKLauncher_WriteLocalManifest()
//{
//
//}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
bool SDKLauncher_CheckForUpdate(const bool bPreRelease)
{
	nlohmann::json remoteManifest;
	string responseMessage;

	if (!SDKLauncher_GetLatestReleaseManifest(XorStr(SDK_DEPOT_VENDOR), responseMessage, remoteManifest, bPreRelease))
	{
		printf("%s: Failed to obtain remote manifest: %s\n", __FUNCTION__, responseMessage.c_str());
		return true; // Can't determine if there is an update or not; skip...
	}

	nlohmann::json localManifest;

	if (!SDKLauncher_GetLocalManifest(localManifest))
	{
		// Failed to load a local one; assume an update is required.
		printf("%s: Failed to obtain local manifest\n", __FUNCTION__);
		return true;
	}

	if (!localManifest.contains("version"))
	{
		// No version information; assume an update is required.
		printf("%s: local manifest does not contain field '%s'!\n", __FUNCTION__, "version");
		return true;
	}

	// This evaluates to '0' if the version tags are equal.
	return !(localManifest["version"] == remoteManifest["tag_name"]);
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
bool SDKLauncher_ForceExistingInstanceOnTop()
{
	HWND existingApp = FindWindowA(FORM_DEFAULT_CLASS_NAME, NULL);

	if (existingApp)
	{
		ForceForegroundWindow(existingApp);
		return true;
	}

	return false;
}
