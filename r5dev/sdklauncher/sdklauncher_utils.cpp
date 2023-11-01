#include "sdklauncher_utils.h"
#include "windows/window.h"
#include "tier0/binstream.h"
#include "tier1/xorstr.h"
#include "tier1/utlmap.h"
#include "tier2/curlutils.h"
#include "zip/src/ZipFile.h"

bool g_bPartialInstall = false;
//bool g_bExperimentalBuilds = false;
float g_flUpdateCheckRate = 64;

double s_flLastProgressUpdate = 0.0;

#define PROGRESS_CALLBACK_UPDATE_DELTA 0.1

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
bool GetDepotList(const nlohmann::json& manifest, nlohmann::json& outDepotList)
{
	if (manifest.contains("depot"))
	{
		const nlohmann::json& depotListArray = manifest["depot"];

		if (!depotListArray.empty())
		{
			outDepotList = depotListArray;
			return true;
		}
	}

	return false;
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
bool GetDepotEntry(const nlohmann::json& manifest, const char* targetDepotName, nlohmann::json& outDepotEntry)
{
	nlohmann::json depotList;

	if (GetDepotList(manifest, depotList))
	{
		printf("*** -<[DEPOT_LIST]>- ***\n%s\n", depotList.dump(4).c_str());

		for (const auto& depot : depotList)
		{
			if (!depot.contains("name"))
				continue;

			const string depotName = depot["name"].get<string>();

			if (depotName.compare(targetDepotName) == NULL)
			{
				outDepotEntry = depot;
				return true;
			}
		}
	}

	printf("%s: Failed on target(%s):\n%s\n", __FUNCTION__, targetDepotName, manifest.dump(4).c_str());
	return false;
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
bool GetDepotAssetList(const nlohmann::json& manifest, const char* targetDepotName, nlohmann::json& outAssetList)
{
	nlohmann::json depotEntry;

	if (GetDepotEntry(manifest, targetDepotName, depotEntry))
	{
		printf("*** -<[DEPOT_ENTRY]>- ***\n%s\n", depotEntry.dump(4).c_str());

		if (depotEntry.contains("assets"))
		{
			const nlohmann::json& assetList = depotEntry["assets"];

			if (!assetList.empty())
			{
				outAssetList = depotEntry["assets"];
				return true;
			}
		}
	}

	printf("%s: Failed on target(%s):\n%s\n", __FUNCTION__, targetDepotName, manifest.dump(4).c_str());
	return false;
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
bool SDKLauncher_ExtractZipFile(nlohmann::json& manifest, const CUtlString& depotFilePath, DepotChangedList_t* changedList, CProgressPanel* pProgress)
{
	ZipArchive::Ptr archive = ZipFile::Open(depotFilePath.Get());
	size_t entries = archive->GetEntriesCount();

	nlohmann::json assetList;
	CUtlString depotFileName = depotFilePath.UnqualifiedFilename();

	const bool assetListRet = GetDepotAssetList(manifest, depotFileName.String(), assetList);
	CUtlVector<CUtlString>* changedAssetList = nullptr;

	// If a file list is provided, only install what it contains.
	if (changedList)
	{
		unsigned short mapIdx = changedList->Find(depotFileName);

		if (mapIdx != changedList->InvalidIndex())
		{
			CUtlVector<CUtlString>* candidate = changedList->Element(mapIdx);

			if (!candidate->IsEmpty())
			{
				changedAssetList = candidate;
			}
		}
	}

	for (size_t i = 0; i < entries; ++i)
	{
		auto entry = archive->GetEntry(int(i));

		if (entry->IsDirectory())
			continue;

		const CUtlString fullName = entry->GetFullName().c_str();
		const char* const pFullName = fullName.Get();

		// Asset hasn't changed, don't replace it.
		if (changedAssetList && !changedAssetList->HasElement(pFullName))
		{
			printf("Asset \"%s\" not in changed list; ignoring...\n", pFullName);
			continue;
		}

		bool installDuringRestart = false;

		// Determine whether or not the asset needs
		// to be installed during a restart.
		if (assetListRet && assetList.contains(pFullName))
		{
			const nlohmann::json& assetEntry = assetList[pFullName];

			if (assetEntry.contains("restart"))
			{
				installDuringRestart = assetEntry["restart"].get<bool>();
			}
		}

		CUtlString absDirName = fullName.AbsPath();
		CUtlString dirName = absDirName.DirName();

		CreateDirectories(absDirName.Get());

		if (pProgress)
		{
			pProgress->SetExportLabel(Format("%s (%llu of %llu)", pFullName, i+1, entries).c_str());

			size_t percentage = (i * 100) / entries;
			pProgress->UpdateProgress((uint32_t)percentage, false);
		}

		printf("Extracting: %s to %s\n", pFullName, dirName.Get());

		if (installDuringRestart)
		{
			CUtlString tempDir = RESTART_DEPOT_DOWNLOAD_DIR;
			tempDir.Append(pFullName);

			ZipFile::ExtractFile(depotFilePath.Get(), pFullName, tempDir.Get());
		}
		else
		{
			ZipFile::ExtractFile(depotFilePath.Get(), pFullName, pFullName);
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
	params.followRedirect = true;
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
bool SDKLauncher_AcquireReleaseManifest(const char* url, string& responseMessage,
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
			const auto& release = responseJson[i];
			const bool isPreRelease = release["prerelease"].get<bool>();

			if (preRelease)
			{
				if (isPreRelease)
				{
					outManifest = release;
					break;
				}
			}
			else if (!isPreRelease)
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
	CProgressPanel* pDownloadSurface = (CProgressPanel*)progessData->user;

	if (pDownloadSurface->IsCanceled())
	{
		pDownloadSurface->Close();
		return -1;
	}

	// This gets called often, prevent this callback from eating all CPU.
	const double curTime = Plat_FloatTime();
	if ((curTime - s_flLastProgressUpdate) < PROGRESS_CALLBACK_UPDATE_DELTA)
	{
		return 0;
	}
	s_flLastProgressUpdate = curTime;

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
	const size_t fileSize, const char* options, CUtlString* errorMessage, CProgressPanel* pProgress)
{
	CURLParams params;

	params.writeFunction = CURLWriteFileCallback;
	params.statusFunction = SDKLauncher_ProgressCallback;
	params.followRedirect = true;

	bool ret = CURLDownloadFile(url, path, fileName, options, fileSize, pProgress, params);

	if (!ret && errorMessage)
		errorMessage->Set(params.errorBuffer);

	return ret;
}

bool SDKLauncher_BuildUpdateList(const nlohmann::json& localManifest,
	const nlohmann::json& remoteManifest, CUtlVector<CUtlString>& outDepotList, DepotChangedList_t& outFileList)
{
	try
	{
		const nlohmann::json& remoteDepotArray = remoteManifest["depot"];
		const nlohmann::json& localDepotArray = localManifest["depot"];

		for (const auto& remoteDepot : remoteDepotArray)
		{
			const string& remoteDepotName = remoteDepot["name"];

			bool containsDepot = false;
			bool digestMatch = false;

			for (const auto& localDepot : localDepotArray)
			{
				if (localDepot["name"] == remoteDepotName)
				{
					containsDepot = true;

					if (remoteDepot["digest"].get<string>() == localDepot["digest"].get<string>())
					{
						digestMatch = true;
					}
					else
					{
						// Check which files have been changed, and only add these to the update list.
						const auto& remoteAssets = remoteDepot["assets"];
						const auto& localAssets = localDepot["assets"];

						// Vector containing all changed files.
						CUtlVector<CUtlString>* changeFileVec = new CUtlVector<CUtlString>();
						outFileList.InsertOrReplace(remoteDepotName.c_str(), changeFileVec);

						for (auto rit = remoteAssets.begin(); rit != remoteAssets.end(); ++rit)
						{
							const string& assetName = rit.key();
							const auto& lit = localAssets.find(assetName.c_str());

							if (lit != localAssets.end())
							{
								const auto& remoteAsset = rit.value();
								const auto& localAsset = lit.value();

								// Digest mismatch; this file needs to be replaced.
								if (remoteAsset["digest"].get<string>() != localAsset["digest"].get<string>())
								{
									printf("Digest mismatch for asset \"%s\"; added to changed list\n", assetName.c_str());
									changeFileVec->AddToTail(assetName.c_str());
								}
							}
							else // Newly added file.
							{
								printf("Local manifest does not contain asset \"%s\"; added to changed list\n", assetName.c_str());
								changeFileVec->AddToTail(assetName.c_str());
							}
						}
					}

					break;
				}
			}

			if (containsDepot)
			{
				if (!digestMatch)
				{
					// Digest mismatch, the file has been changed,
					// add it to the list so we are installing it.
					outDepotList.AddToTail(remoteDepotName.c_str());
				}
			}
			else
			{
				// Local manifest does not contain the asset,
				// add it to the list so we are installing it.
				outDepotList.AddToTail(remoteDepotName.c_str());
			}
		}
	}
	catch (const std::exception& ex)
	{
		printf("%s - Exception while building update list:\n%s\n", __FUNCTION__, ex.what());

		outDepotList.Purge();
		outFileList.Purge();

		return false;
	}

	return true;
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
bool SDKLauncher_BeginInstall(const bool bPreRelease, const bool bOptionalDepots,
	CUtlVector<CUtlString>& zipList, CUtlString* errorMessage, CProgressPanel* pProgress)
{
	string responseMessage;
	nlohmann::json remoteManifest;

	if (!SDKLauncher_GetRemoteManifest(XorStr(SDK_DEPOT_VENDOR), responseMessage, remoteManifest, bPreRelease))
	{
		printf("%s: Failed! %s\n", "SDKLauncher_GetRemoteManifest", responseMessage.c_str());

		// TODO: make more comprehensive...
		errorMessage->Set("Failed to obtain remote manifest");
		return false;
	}

	CUtlVector<CUtlString> depotList;
	DepotChangedList_t fileList(UtlStringLessFunc);

	nlohmann::json localManifest;

	if (SDKLauncher_GetLocalManifest(localManifest))
	{
		SDKLauncher_BuildUpdateList(localManifest, remoteManifest, depotList, fileList);
	}
	else
	{
		// Leave the vector empty, this will download everything.
		Assert(depotList.IsEmpty());
	}

	FOR_EACH_VEC(depotList, i)
	{
		const CUtlString& depotName = depotList[i];
		printf("CUtlVector< CUtlString >::depotList[ %d ] = %s\n", i, depotName.Get());
	}

	if (!SDKLauncher_DownloadDepotList(remoteManifest, depotList,
		zipList, errorMessage, pProgress, DEFAULT_DEPOT_DOWNLOAD_DIR, bOptionalDepots))
	{
		// Error message is set by previous function.
		return false;
	}

	// Canceling returns true, as the function didn't fail.
	// We should however still delete all the downloaded files.
	if (pProgress->IsCanceled())
		return true;


	if (!SDKLauncher_InstallDepotList(remoteManifest, zipList, &fileList, pProgress))
	{
		return false;
	}

	if (!SDKLauncher_WriteLocalManifest(remoteManifest))
	{
		errorMessage->Set("Failed to write local manifest file (insufficient rights?)");
		return false;
	}

	return true;
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
bool SDKLauncher_DownloadDepotList(nlohmann::json& manifest, CUtlVector<CUtlString>& depotList,
	CUtlVector<CUtlString>& outZipList, CUtlString* errorMessage, CProgressPanel* pProgress, const char* pPath,
	const bool bOptionalDepots)
{
	if (!manifest.contains("depot"))
	{
		Assert(0);
		errorMessage->Set("Invalid manifest");
		return false;
	}

	const auto depotListArray = manifest["depot"];

	if (depotListArray.empty())
	{
		errorMessage->Set("No depots found in manifest");
		return false;
	}

	int i = 1;

	for (auto& depot : depotListArray)
	{
		if (pProgress->IsCanceled())
		{
			break;
		}

		if (!SDKLauncher_IsDepositoryValid(depot))
		{
			// Invalid manifest format.
			Assert(0);
			continue;
		}

		if ((!bOptionalDepots &&
			depot["optional"].get<bool>()))
		{
			// Optional depots disabled.
			continue;
		}

		const string fileName = depot["name"].get<string>();

		if (!depotList.IsEmpty())
		{
			if (!depotList.HasElement(fileName.c_str()))
			{
				continue;
			}
		}

		const size_t fileSize = depot["size"].get<size_t>();
		string downloadLink = depot["vendor"].get<string>();

		AppendSlash(downloadLink, '/');
		downloadLink.append(fileName);

		pProgress->SetText(Format("Downloading package %i of %i...", i,
			!depotList.IsEmpty() ? depotList.Count() : (int)depotListArray.size()).c_str());

		if (!SDKLauncher_DownloadAsset(downloadLink.c_str(), pPath, fileName.c_str(), fileSize, "wb+", errorMessage, pProgress))
		{
			// Error message is set by previous function.
			return false;
		}

		// Check if its a zip file, as these are
		// the only files that will be installed.
		if (V_strcmp(V_GetFileExtension(fileName.c_str()), "zip") == NULL)
		{
			CUtlString filePath;

			filePath = pPath;
			filePath.AppendSlash('/');
			filePath.Append(fileName.c_str());

			outZipList.AddToTail(filePath);
		}

		i++;
	}

	return true;
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
bool SDKLauncher_InstallDepotList(nlohmann::json& manifest, CUtlVector<CUtlString>& depotList,
	DepotChangedList_t* fileList, CProgressPanel* pProgress)
{
	// Install process cannot be canceled.
	pProgress->SetCanCancel(false);

	FOR_EACH_VEC(depotList, i)
	{
		pProgress->SetText(Format("Installing package %i of %i...", i + 1, depotList.Count()).c_str());
		CUtlString& depotFilePath = depotList[i];

		// TODO[ AMOS ]: make the ZIP lib return error codes instead
		if (!SDKLauncher_ExtractZipFile(manifest, depotFilePath, fileList, pProgress))
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

bool SDKLauncher_IsManifestValid(const nlohmann::json& depotManifest)
{
	return (!depotManifest.empty() &&
		depotManifest.contains("version") &&
		depotManifest.contains("depot"));
}

bool SDKLauncher_IsDepositoryValid(const nlohmann::json& depot)
{
	return (!depot.empty() &&
		depot.contains("optional") &&
		depot.contains("vendor") &&
		depot.contains("size") &&
		depot.contains("name"));
}

bool SDKLauncher_GetRemoteManifest(const char* url, string& responseMessage, nlohmann::json& remoteManifest, const bool bPreRelease)
{
	nlohmann::json responseJson;

	if (!SDKLauncher_AcquireReleaseManifest(url, responseMessage, responseJson, bPreRelease))
	{
		// TODO: Error dialog.

		printf("%s: failed!\n", "SDKLauncher_AcquireReleaseManifest");
		return false;
	}

	if (!responseJson.contains("assets"))
	{
		printf("%s: failed!\n", "responseJson.contains(\"assets\")");
		return false;
	}

	try
	{
		string depotManifestUrl;

		{
			nlohmann::json& assetList = responseJson["assets"];

			for (const auto& asset : assetList)
			{
				if (asset["name"] == DEPOT_MANIFEST_FILE)
				{
					depotManifestUrl = asset["browser_download_url"];
					break;
				}
			}
		}

		if (depotManifestUrl.empty())
		{
			printf("depotManifestUrl.empty()==true\n");
			return false;
		}

		string responseBody;
		CURLINFO status;

		if (!SDKLauncher_QueryServer(depotManifestUrl.c_str(), responseBody, responseMessage, status) || 
			status != 200)
		{
			printf("%s: failed! %s, status=%d\n", "SDKLauncher_QueryServer", responseMessage.c_str(), status);

			responseMessage = responseBody;
			return false;
		}

		remoteManifest = nlohmann::json::parse(responseBody);
	}
	catch (const std::exception& ex)
	{
		printf("%s - Exception while parsing response:\n%s\n", __FUNCTION__, ex.what());
		return false;
	}

	Assert(!remoteManifest.empty());
	return true;
}

bool SDKLauncher_GetLocalManifest(nlohmann::json& localManifest)
{
	if (!fs::exists(DEPOT_MANIFEST_FILE_PATH))
		return false;

	ifstream localFile(DEPOT_MANIFEST_FILE_PATH);

	if (!localFile.good())
		return false;

	try
	{
		localManifest = nlohmann::json::parse(localFile);

		if (!SDKLauncher_IsManifestValid(localManifest))
		{
			return false;
		}
	}
	catch (const std::exception& ex)
	{
		printf("%s - Exception while parsing manifest:\n%s\n", __FUNCTION__, ex.what());
		return false;
	}

	return true;
}

bool SDKLauncher_WriteLocalManifest(const nlohmann::json& localManifest)
{
	CIOStream writer;
	if (!writer.Open(DEPOT_MANIFEST_FILE_PATH, CIOStream::Mode_t::WRITE))
	{
		return false;
	}

	const string manifestBuf = localManifest.dump(4);
	writer.Write(manifestBuf.c_str(), manifestBuf.size());

	return true;
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
bool SDKLauncher_CheckForUpdate(const bool bPreRelease)
{
	nlohmann::json remoteManifest;
	string responseMessage;

	if (!SDKLauncher_AcquireReleaseManifest(XorStr(SDK_DEPOT_VENDOR), responseMessage, remoteManifest, bPreRelease))
	{
		printf("%s: Failed to obtain remote manifest: %s\n", __FUNCTION__, responseMessage.c_str());
		return false; // Can't determine if there is an update or not; skip...
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
		printf("%s: Local manifest does not contain field '%s'!\n", __FUNCTION__, "version");
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
