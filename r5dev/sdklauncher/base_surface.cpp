//=============================================================================//
//
// Purpose: Launcher user interface implementation.
//
//=============================================================================//
#include "base_surface.h"
#include "advanced_surface.h"
#include "download_surface.h"
#include "tier1/xorstr.h"
#include "tier1/utlmap.h"
#include "tier2/curlutils.h"
#include "zip/src/ZipFile.h"

// Gigabytes.
// TODO: obtain these from the repo...
#define MIN_REQUIRED_DISK_SPACE 20
#define MIN_REQUIRED_DISK_SPACE_OPT 55 // Optional textures
#define DEFAULT_DEPOT_DOWNLOAD_DIR "platform\\depot\\"

// Files that need to be installed during launcher restart,
// have to go here!!
#define RESTART_DEPOT_DOWNLOAD_DIR DEFAULT_DEPOT_DOWNLOAD_DIR "temp\\"


#define WINDOW_SIZE_X 400
#define WINDOW_SIZE_Y 194

CBaseSurface::CBaseSurface()
{
	this->SuspendLayout();
	this->SetAutoScaleDimensions({ 6, 13 });
	this->SetAutoScaleMode(Forms::AutoScaleMode::Font);
	this->SetText(XorStr("R5Reloaded"));
	this->SetClientSize({ WINDOW_SIZE_X, WINDOW_SIZE_Y });
	this->SetFormBorderStyle(Forms::FormBorderStyle::FixedSingle);
	this->SetStartPosition(Forms::FormStartPosition::CenterScreen);
	this->SetMinimizeBox(true);
	this->SetMaximizeBox(false);
	this->SetBackColor(Drawing::Color(47, 54, 61));

	this->m_BaseGroup = new UIX::UIXGroupBox();
	this->m_ManageButton = new UIX::UIXButton();
	this->m_RepairButton = new UIX::UIXButton();
	this->m_DonateButton = new UIX::UIXButton();
	this->m_JoinButton = new UIX::UIXButton();
	this->m_AdvancedButton = new UIX::UIXButton();

	const INT BASE_GROUP_OFFSET = 12;

	this->m_BaseGroup = new UIX::UIXGroupBox();
	this->m_BaseGroup->SetSize({ WINDOW_SIZE_X - (BASE_GROUP_OFFSET * 2), WINDOW_SIZE_Y - (BASE_GROUP_OFFSET * 2) });
	this->m_BaseGroup->SetLocation({ BASE_GROUP_OFFSET, BASE_GROUP_OFFSET });
	this->m_BaseGroup->SetTabIndex(0);
	this->m_BaseGroup->SetText("");
	this->m_BaseGroup->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->AddControl(this->m_BaseGroup);

	const bool isInstalled = fs::exists("r5apex.exe");

	this->m_ManageButton = new UIX::UIXButton();
	this->m_ManageButton->SetSize({ 168, 70 });
	this->m_ManageButton->SetLocation({ 10, 10 });
	this->m_ManageButton->SetTabIndex(9);
	this->m_ManageButton->SetText(isInstalled ? XorStr("Launch Apex") : XorStr("Install Apex"));
	this->m_ManageButton->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left);
	this->m_ManageButton->Click += isInstalled ? &OnAdvancedClick : &OnInstallClick;
	m_BaseGroup->AddControl(this->m_ManageButton);

	this->m_RepairButton = new UIX::UIXButton();
	this->m_RepairButton->SetSize({ 168, 70 });
	this->m_RepairButton->SetLocation({ 10, 90 });
	this->m_RepairButton->SetTabIndex(9);
	this->m_RepairButton->SetEnabled(isInstalled);
	this->m_RepairButton->SetText(XorStr("Repair Apex"));
	this->m_RepairButton->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left);
	this->m_RepairButton->Click += &OnAdvancedClick;
	m_BaseGroup->AddControl(this->m_RepairButton);

	this->m_DonateButton = new UIX::UIXButton();
	this->m_DonateButton->SetSize({ 178, 43 });
	this->m_DonateButton->SetLocation({ 188, 10 });
	this->m_DonateButton->SetTabIndex(9);
	this->m_DonateButton->SetText(XorStr("Support Amos (The Creator)"));
	this->m_DonateButton->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left);
	this->m_DonateButton->Click += &OnSupportClick;
	m_BaseGroup->AddControl(this->m_DonateButton);

	this->m_JoinButton = new UIX::UIXButton();
	this->m_JoinButton->SetSize({ 178, 43 });
	this->m_JoinButton->SetLocation({ 188, 63 });
	this->m_JoinButton->SetTabIndex(9);
	this->m_JoinButton->SetText(XorStr("Join our Discord"));
	this->m_JoinButton->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left);
	this->m_JoinButton->Click += &OnJoinClick;
	m_BaseGroup->AddControl(this->m_JoinButton);

	this->m_AdvancedButton = new UIX::UIXButton();
	this->m_AdvancedButton->SetSize({ 178, 43 });
	this->m_AdvancedButton->SetLocation({ 188, 116 });
	this->m_AdvancedButton->SetTabIndex(9);
	this->m_AdvancedButton->SetEnabled(isInstalled);
	this->m_AdvancedButton->SetText(XorStr("Advanced Options"));
	this->m_AdvancedButton->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left);
	this->m_AdvancedButton->Click += &OnAdvancedClick;
	m_BaseGroup->AddControl(this->m_AdvancedButton);

	// TODO: Use a toggle item instead; remove this field.
	m_bPartialInstall = false;
}

bool QueryServer(const char* url, string& outResponse, string& outMessage, CURLINFO& outStatus)
{
	curl_slist* sList = nullptr;

	CURLParams params;

	params.writeFunction = CURLWriteStringCallback;
	params.timeout = 15;
	params.verifyPeer = true;
	params.verbose = false;

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

void CreateDownloadDirectories()
{
	// Make sure the download path exists.
	CreateDirectories(DEFAULT_DEPOT_DOWNLOAD_DIR);
	CreateDirectories(RESTART_DEPOT_DOWNLOAD_DIR);
}


int DownloadStatusCallback(CURLProgress* progessData, double dltotal, double dlnow, double ultotal, double ulnow)
{
	CDownloadProgress* pDownloadSurface = (CDownloadProgress*)progessData->cust;

	if (pDownloadSurface->IsCanceled())
		return -1;

	double downloaded;
	curl_easy_getinfo(progessData->curl, CURLINFO_SIZE_DOWNLOAD, &downloaded);

	size_t percentage = ((size_t)downloaded * 100) / progessData->size;
	pDownloadSurface->UpdateProgress((uint32_t)percentage, false);

	return 0;
}

void DownloadAsset(CDownloadProgress* pProgress, const char* url, const char* path,
	const char* fileName, const size_t fileSize, const char* options)
{
	CURLParams params;

	params.writeFunction = CURLWriteFileCallback;
	params.statusFunction = DownloadStatusCallback;

	CURLDownloadFile(url, path, fileName, options, fileSize, pProgress, params);
}

void DownloadAssetList(CDownloadProgress* pProgress, CUtlVector<CUtlString>& fileList, nlohmann::json& assetList, std::set<string>& blackList, const char* pPath)
{
	int i = 1;
	for (auto& asset : assetList)
	{
		if (pProgress->IsCanceled())
		{
			pProgress->Close();
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

		// Asset is filtered, don't download.
		if (blackList.find(fileName) != blackList.end())
			continue;

		const string downloadLink = asset["browser_download_url"];
		const size_t fileSize = asset["size"];

		pProgress->SetExportLabel(Format("%s (%i of %i)", fileName.c_str(), i, assetList.size()).c_str());
		DownloadAsset(pProgress, downloadLink.c_str(), pPath, fileName.c_str(), fileSize, "wb+");

		CUtlString filePath = pPath;
		filePath.Append(fileName.c_str());

		fileList.AddToTail(filePath);
		i++;
	}
}

bool DownloadLatestGitHubReleaseManifest(const char* url, string& responseMessage, nlohmann::json& outManifest, const bool preRelease)
{
	string responseBody;
	CURLINFO status;

	if (!QueryServer(url, responseBody, responseMessage, status))
	{
		// TODO: Error dialog...
		printf("Query error!\n");
		return false;
	}

	if (status != 200)
	{
		// TODO: Error dialog...
		printf("Query error; status not 200!\n%s\n", responseBody.c_str());
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
			else if (!preRelease && !release["prerelease"])
			{
				outManifest = release["assets"];
				break;
			}

			if (i == responseJson.size()-1 && outManifest.empty())
				release[0]["assets"]; // Just take the first one then.
		}
	}
	catch (const std::exception& ex)
	{
		printf("%s - Exception while parsing response:\n%s\n", __FUNCTION__, ex.what());

		//Warning(eDLL_T::ENGINE, "%s - Exception while parsing response:\n%s\n", __FUNCTION__, ex.what());
		// TODO: Error dialog; report to amos or something like that.

		return false;
	}

	return !outManifest.empty();
}

bool ExtractZipFile(CDownloadProgress* pProgress, const char* pZipFile, const char* pDestPath)
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
		pProgress->SetExportLabel(Format("%s (%i of %i)", fileName.Get(), i+1, fileList.Count()).c_str());

		int percentage = (i * 100) / fileList.Count();
		pProgress->UpdateProgress(percentage, false);

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


void BeginInstall(CDownloadProgress* pProgress, const bool bPreRelease, const bool bOptionalAssets)
{
	CreateDownloadDirectories();

	string responseMesage;
	nlohmann::json manifest;

	// These files will NOT be downloaded from the release depots.
	std::set<string> blackList;
	blackList.insert("symbols.zip");

	// All the downloaded files.
	CUtlVector<CUtlString> fileList;

	CUtlString test = DEFAULT_DEPOT_DOWNLOAD_DIR;
	test.Append("depot.zip");

	fileList.AddToTail(test);

	// Download core game files.
	//if (!DownloadLatestGitHubReleaseManifest(XorStr("https://api.github.com/repos/SlaveBuild/N1094_CL456479/releases"), responseMesage, manifest, bPreRelease))
	//{
	//	// TODO: Error dialog.
	//	return;
	//}
	//DownloadAssetList(pProgress, fileList, manifest, blackList, DEFAULT_DEPOT_DOWNLOAD_DIR);

	//if (pProgress->IsCanceled())
	//	return;

	//// Download SDK files.
	//if (!DownloadLatestGitHubReleaseManifest(XorStr("https://api.github.com/repos/Mauler125/r5sdk/releases"), responseMesage, manifest, bPreRelease))
	//{
	//	// TODO: Error dialog.
	//	return;
	//}
	//DownloadAssetList(pProgress, fileList, manifest, blackList, DEFAULT_DEPOT_DOWNLOAD_DIR);

	//if (pProgress->IsCanceled())
	//	return;

	// Install process cannot be canceled.
	pProgress->SetCanCancel(false);

	FOR_EACH_VEC(fileList, i)
	{
		pProgress->SetText(Format("Installing package %i of %i...", i+1, fileList.Count()).c_str());

		CUtlString& fileName = fileList[i];
		ExtractZipFile(pProgress, fileName.Get(), "");
	}

	pProgress->Close();
}


void RestartLauncher()
{
	char currentPath[MAX_PATH];
	BOOL getResult = GetCurrentDirectoryA(sizeof(currentPath), currentPath);

	if (!getResult)
	{
		// TODO: dialog box and instruct user to manually open the launcher again.
		printf("%s: Failed to update SDK: error code = %08x\n", "GetCurrentDirectory", GetLastError());
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
		commandLine,                                     // lpCommandLine
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
		printf("%s: Failed to update SDK: error code = %08x\n", "CreateProcess", GetLastError());
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

void CBaseSurface::OnInstallClick(Forms::Control* Sender)
{
	CBaseSurface* pSurf = (CBaseSurface*)Sender;
	const bool bPartial = pSurf->m_bPartialInstall;

	//----------------------------------------------------------------------------
	// Disk space check
	//----------------------------------------------------------------------------
	char currentDir[MAX_PATH]; // Get current dir.
	GetCurrentDirectoryA(sizeof(currentDir), currentDir);

	// Does this disk have enough space?
	ULARGE_INTEGER avaliableSize;
	GetDiskFreeSpaceEx(currentDir, &avaliableSize, nullptr, nullptr);

	const int minRequiredSpace = bPartial ? MIN_REQUIRED_DISK_SPACE : MIN_REQUIRED_DISK_SPACE_OPT;
	const int currentAvailSpace = (int)(avaliableSize.QuadPart / uint64_t(1024 * 1024 * 1024));

	//if (currentAvailSpace < minRequiredSpace)
	//{
	//	// TODO: Make dialog box.
	//	printf("There is not enough space available on the disk to install R5Reloaded; you need at least %iGB, you currently have %iGB\n", minRequiredSpace, currentAvailSpace);
	//	return;
	//}

	auto downloadSurface = std::make_unique<CDownloadProgress>();
	CDownloadProgress* pProgress = downloadSurface.get();

	pProgress->SetAutoClose(true);

	Threading::Thread([pProgress] {

		BeginInstall(pProgress, false, false);

		}).Start();

	pProgress->ShowDialog();

	// Restart the launcher process from here through updater.exe!
	RestartLauncher();
}

void CBaseSurface::OnAdvancedClick(Forms::Control* Sender)
{
	auto pAdvancedSurface = std::make_unique<CAdvancedSurface>();
	pAdvancedSurface->ShowDialog((Forms::Form*)Sender->FindForm());
}

void CBaseSurface::OnSupportClick(Forms::Control* /*Sender*/)
{
	ShellExecute(0, 0, XorStr("https://www.paypal.com/donate/?hosted_button_id=S28DHC2TF6UV4"), 0, 0, SW_SHOW);
}

void CBaseSurface::OnJoinClick(Forms::Control* /*Sender*/)
{
	ShellExecute(0, 0, XorStr("https://discord.com/invite/jqMkUdXrBr"), 0, 0, SW_SHOW);
}
