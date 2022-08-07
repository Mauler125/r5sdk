#pragma once
#include "sdklauncher_const.h"

struct ModManager_t;

struct LogList_t
{
	LogList_t(spdlog::level::level_enum nLevel, String svText)
	{
		m_nLevel = nLevel;
		m_svText = svText;
	}

	spdlog::level::level_enum m_nLevel;
	String m_svText;
};

enum sdkDownloadState {
	idle,
	downloading,
	download_complete,
	download_failed,
	download_cancelled,
	download_paused, // Idk how the user will pause it, but hey. It's here
};

class CUIBaseSurface : public Forms::Form
{
public:
	CUIBaseSurface();
	virtual ~CUIBaseSurface() = default;

	std::vector<LogList_t> m_LogList;
	UIX::UIXListView* m_ConsoleListView;
	std::vector<ModManager_t> ModList;
	UIX::UIXListView* ModsListView;

	void logText(spdlog::level::level_enum color, std::string text);
	void logText(std::string text);

	sdkDownloadState sdkDownState = idle;
	std::string sdkDownloadLocation = "";

	std::vector<fs::path> GameFileDiff;

	//Drawing::Color traceColor = Drawing::Color(255, 255, 255);
	//Drawing::Color debugColor = Drawing::Color(0, 120, 215);
	//Drawing::Color infoColor = Drawing::Color(92, 236, 89);
	//Drawing::Color warnColor = Drawing::Color(236, 203, 0);
	//Drawing::Color errorColor = Drawing::Color(236, 28, 0);
	//Drawing::Color criticalColor = Drawing::Color(236, 28, 0);
	//Drawing::Color generalColor = Drawing::Color(255, 255, 255);

private:
	int validMods = 0;
	int enabledMods = 0;
	bool bInstallGame = false;
	bool bSdkChecked = false;
	std::string newVersion = "";
	std::vector < std::pair < std::string, std::string >> v_LauncherValues{ {"version", ""}, {"magnet", "magnet:?xt=urn:btih:KCQJQT6DV2V4XWCOKCRM4EJELRLHQKI5&dn=R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM&tr=udp%3A%2F%2Fwambo.club%3A1337%2Fannounce"}};

	void Init();
	void Setup();
	void ParseMaps();
	void ParsePlaylists();
	void ReadModJson();
	void AdjustValues();
	void InstallGameCheck();
	std::string GetConfigCFG(const std::string& first);
	void SetConfigCFG(const std::string& first, const std::string& second);

	static void LaunchGame(Forms::Control* pSender);
	static void CleanSDK(Forms::Control* pSender);
	static void ReloadPlaylists(Forms::Control* pSender);
	static void VirtualItemToClipboard(const std::unique_ptr<MouseEventArgs>& pEventArgs, Forms::Control* pSender);
	static void ModManagerClick(const std::unique_ptr<MouseEventArgs>& pEventArgs, Forms::Control* pSender);
	static void ModManagerEnabledToggle(Forms::Control* pSender);
	static void GetVirtualItem(const std::unique_ptr<Forms::RetrieveVirtualItemEventArgs>& pEventArgs, Forms::Control* pSender);
	static void GetVirtItemMod(const std::unique_ptr<Forms::RetrieveVirtualItemEventArgs>& pEventArgs, Forms::Control* pSender);
	static void UnfocusedManager(Forms::Control* pSender);
	static void ForwardCommandToGame(Forms::Control* pSender);
	eLaunchMode BuildParameter(string& svParameter);
	
	static void InstallGame(Forms::Control* pSender);
	static void MoveGameFiles(Forms::Control* pSender);
	static void LatestSDKCompare(Forms::Control* pSender);
	static void UpdateSDKChecker(Forms::Control* pSender);
	static void DownloadSDK(Forms::Control* pSender);
	static size_t ProgressCallback(CUIBaseSurface* pSurface, double dltotal, double dlnow, double ultotal, double ulnow);

	CSimpleIniA* ini;

	enum class eMode
	{
		NONE = -1,
		HOST,
		SERVER,
		CLIENT,
	};
	enum class eVisibility
	{
		PUBLIC,
		HIDDEN,
	};

	UIX::UIXTextBox* m_WidthTextBox;
	UIX::UIXTextBox* m_HeightTextBox;
	UIX::UIXTextBox* m_WorkerThreadsTextBox;
	UIX::UIXTextBox* m_ReservedCoresTextBox;
	UIX::UIXTextBox* m_FpsTextBox;
	UIX::UIXTextBox* m_PlaylistFileTextBox;
	UIX::UIXTextBox* m_HostNameTextBox;
	UIX::UIXTextBox* m_LaunchArgsTextBox;
	UIX::UIXTextBox* m_ConsoleCommandTextBox;
	UIX::UIXTextBox* m_ManagerControlsValidText;
	UIX::UIXTextBox* m_ManagerControlsEnabledText;
	UIX::UIXTextBox* m_ManagerViewerDescText;
	UIX::UIXTextBox* m_ManagerViewerCoverText;
	// Labels
	UIX::UIXLabel* m_WorkerThreadsLabel;
	UIX::UIXLabel* m_ReservedCoresLabel;
	UIX::UIXLabel* m_MapLabel;
	UIX::UIXLabel* m_PlaylistLabel;
	UIX::UIXLabel* m_ModeLabel;
	UIX::UIXLabel* m_FpsLabel;
	UIX::UIXLabel* m_ResolutionLabel;
	UIX::UIXLabel* m_PlaylistFileLabel;
	UIX::UIXLabel* m_HostNameLabel;
	UIX::UIXLabel* m_VisibilityLabel;
	UIX::UIXLabel* m_LaunchArgsLabel;
	UIX::UIXLabel* m_ManagerViewerNameLabel;
	UIX::UIXLabel* m_ManagerViewerAuthorLabel;
	UIX::UIXLabel* m_ManagerViewerVersionLabel;
	UIX::UIXLabel* m_ManagerViewerAppidLabel;
	UIX::UIXLabel* m_ManagerEnabledLabel;
	// Boxes
	UIX::UIXGroupBox* m_GameGroup;
	UIX::UIXGroupBox* m_MainGroup;
	UIX::UIXGroupBox* m_GameGroupExt;
	UIX::UIXGroupBox* m_MainGroupExt;
	UIX::UIXGroupBox* m_ConsoleGroupExt;
	UIX::UIXGroupBox* m_ConsoleGroup;
	UIX::UIXGroupBox* m_EngineBaseGroup;
	UIX::UIXGroupBox* m_EngineNetworkGroup;
	UIX::UIXGroupBox* m_EngineVideoGroup;
	UIX::UIXGroupBox* m_ManagerSurroundingBox;
	UIX::UIXGroupBox* m_ManagerGroupExt;
	UIX::UIXGroupBox* m_ManagerGroup;
	UIX::UIXGroupBox* m_ManagerControlsGroup;
	UIX::UIXGroupBox* m_ManagerViewerBox;
	UIX::UIXGroupBox* m_ManagerEnabledBox;
	// Toggles
	UIX::UIXCheckBox* m_CheatsToggle;
	UIX::UIXCheckBox* m_DevelopmentToggle;
	UIX::UIXCheckBox* m_ConsoleToggle;
	UIX::UIXCheckBox* m_WindowedToggle;
	UIX::UIXCheckBox* m_NoBorderToggle;
	UIX::UIXCheckBox* m_SingleCoreDediToggle;
	UIX::UIXCheckBox* m_NoAsyncJobsToggle;
	UIX::UIXCheckBox* m_NetEncryptionToggle;
	UIX::UIXCheckBox* m_NetRandomKeyToggle;
	UIX::UIXCheckBox* m_NoQueuedPacketThread;
	UIX::UIXCheckBox* m_NoTimeOutToggle;
	UIX::UIXCheckBox* m_ColorConsoleToggle;
	UIX::UIXCheckBox* m_ManagerEnabledToggle;
	UIX::UIXCheckBox* m_ManagerUseModsToggle;
	// Combo
	UIX::UIXComboBox* m_MapCombo;
	UIX::UIXComboBox* m_PlaylistCombo;
	UIX::UIXComboBox* m_ModeCombo;
	UIX::UIXComboBox* m_VisibilityCombo;
	// Buttons
	UIX::UIXButton* m_CleanSDK;
	UIX::UIXButton* m_UpdateSDK;
	UIX::UIXButton* m_LaunchSDK;
	UIX::UIXButton* m_ConsoleSendCommand;
};
