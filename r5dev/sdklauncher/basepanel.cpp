
#include "core/stdafx.h"
#include "basepanel.h"
#include <objidl.h>
#include "gdiplus.h"
#include "shellapi.h"

void CUIBasePanel::Init()
{
	const INT WindowX = 800;
	const INT WindowY = 350;

	this->SuspendLayout();
	this->SetAutoScaleDimensions({ 6, 13 });
	this->SetAutoScaleMode(Forms::AutoScaleMode::Font);
	this->SetText("SDK Launcher");
	this->SetClientSize({ WindowX, WindowY });
	this->SetFormBorderStyle(Forms::FormBorderStyle::FixedSingle);
	this->SetStartPosition(Forms::FormStartPosition::CenterParent);
	this->SetMinimizeBox(false);
	this->SetMaximizeBox(false);

	// #################################################################################################
	//	
	// #################################################################################################
	this->m_GameGroup = new UIX::UIXGroupBox();
	this->m_GameGroup->SetSize({ 458, 84 });
	this->m_GameGroup->SetLocation({ 12, 10 });
	this->m_GameGroup->SetTabIndex(0);
	this->m_GameGroup->SetText("Game");
	this->m_GameGroup->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->AddControl(this->m_GameGroup);

	this->m_GameGroupExt = new UIX::UIXGroupBox();
	this->m_GameGroupExt->SetSize({ 458, 55 });
	this->m_GameGroupExt->SetLocation({ 12, 93 });
	this->m_GameGroupExt->SetTabIndex(0);
	this->m_GameGroupExt->SetText("");
	this->m_GameGroupExt->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->AddControl(this->m_GameGroupExt);

	this->m_MapLabel = new UIX::UIXLabel();
	this->m_MapLabel->SetSize({ 50, 25 });
	this->m_MapLabel->SetLocation({ 365, 28 });
	this->m_MapLabel->SetTabIndex(0);
	this->m_MapLabel->SetText("Map");
	this->m_MapLabel->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_MapLabel->SetTextAlign(Drawing::ContentAlignment::TopLeft);
	this->m_GameGroup->AddControl(this->m_MapLabel);

	this->m_MapCombo = new UIX::UIXComboBox();
	this->m_MapCombo->SetSize({ 347, 25 });
	this->m_MapCombo->SetLocation({ 15, 25 });
	this->m_MapCombo->SetTabIndex(0);
	this->m_MapCombo->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_MapCombo->SetDropDownStyle(Forms::ComboBoxStyle::DropDownList);
	std::regex rgArchiveRegex{ R"([^_]*_(.*)(.bsp.pak000_dir).*)" };
	std::smatch smRegexMatches;

	for (const auto& dEntry : fs::directory_iterator("vpk"))
	{
		std::string svFileName = dEntry.path().string();
		std::regex_search(svFileName, smRegexMatches, rgArchiveRegex);

		if (smRegexMatches.size() > 0)
		{
			if (strcmp(smRegexMatches[1].str().c_str(), "frontend") == 0)
			{
				continue;
			}
			else if (strcmp(smRegexMatches[1].str().c_str(), "mp_common") == 0)
			{
				this->m_MapCombo->Items.Add("mp_lobby");
				continue;
			}

			this->m_MapCombo->Items.Add(smRegexMatches[1].str().c_str());
		}
	}
	this->m_GameGroup->AddControl(this->m_MapCombo);

	this->m_PlaylistLabel = new UIX::UIXLabel();
	this->m_PlaylistLabel->SetSize({ 50, 25 });
	this->m_PlaylistLabel->SetLocation({ 365, 53 });
	this->m_PlaylistLabel->SetTabIndex(0);
	this->m_PlaylistLabel->SetText("Playlist");
	this->m_PlaylistLabel->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_PlaylistLabel->SetTextAlign(Drawing::ContentAlignment::TopLeft);
	this->m_GameGroup->AddControl(this->m_PlaylistLabel);

	this->m_PlaylistCombo = new UIX::UIXComboBox();
	this->m_PlaylistCombo->SetSize({ 347, 25 });
	this->m_PlaylistCombo->SetLocation({ 15, 50 });
	this->m_PlaylistCombo->SetTabIndex(0);
	this->m_PlaylistCombo->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_PlaylistCombo->SetDropDownStyle(Forms::ComboBoxStyle::DropDownList);
	this->m_GameGroup->AddControl(this->m_PlaylistCombo);

	this->m_CheatsToggle = new UIX::UIXCheckBox();
	this->m_CheatsToggle->SetSize({ 110, 18 });
	this->m_CheatsToggle->SetLocation({ 15, 7 });
	this->m_CheatsToggle->SetTabIndex(0);
	this->m_CheatsToggle->SetText("Enable cheats");
	this->m_CheatsToggle->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_GameGroupExt->AddControl(this->m_CheatsToggle);

	this->m_DevelopmentToggle = new UIX::UIXCheckBox();
	this->m_DevelopmentToggle->SetSize({ 150, 18 });
	this->m_DevelopmentToggle->SetLocation({ 130, 7 });
	this->m_DevelopmentToggle->SetTabIndex(0);
	this->m_DevelopmentToggle->SetText("Enable development");
	this->m_DevelopmentToggle->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_GameGroupExt->AddControl(this->m_DevelopmentToggle);

	this->m_ConsoleToggle = new UIX::UIXCheckBox();
	this->m_ConsoleToggle->SetSize({ 150, 18 });
	this->m_ConsoleToggle->SetLocation({ 290, 7 });
	this->m_ConsoleToggle->SetTabIndex(0);
	this->m_ConsoleToggle->SetText("Show console");
	this->m_ConsoleToggle->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_GameGroupExt->AddControl(this->m_ConsoleToggle);

	this->m_ColorConsoleToggle = new UIX::UIXCheckBox();
	this->m_ColorConsoleToggle->SetSize({ 105, 18 });
	this->m_ColorConsoleToggle->SetLocation({ 15, 30 });
	this->m_ColorConsoleToggle->SetTabIndex(0);
	this->m_ColorConsoleToggle->SetChecked(true);
	this->m_ColorConsoleToggle->SetText("Color console");
	this->m_ColorConsoleToggle->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_GameGroupExt->AddControl(this->m_ColorConsoleToggle);

	this->m_PlaylistFileTextBox = new UIX::UIXTextBox();
	this->m_PlaylistFileTextBox->SetSize({ 178, 18 });
	this->m_PlaylistFileTextBox->SetLocation({ 130, 30 });
	this->m_PlaylistFileTextBox->SetTabIndex(0);
	this->m_PlaylistFileTextBox->SetText("playlists_r5_patch.txt");
	this->m_PlaylistFileTextBox->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_GameGroupExt->AddControl(this->m_PlaylistFileTextBox);

	this->m_PlaylistFileLabel = new UIX::UIXLabel();
	this->m_PlaylistFileLabel->SetSize({ 50, 18 });
	this->m_PlaylistFileLabel->SetLocation({ 311, 32 });
	this->m_PlaylistFileLabel->SetTabIndex(0);
	this->m_PlaylistFileLabel->SetText("Playlist file");
	this->m_PlaylistFileLabel->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left);
	this->m_GameGroupExt->AddControl(this->m_PlaylistFileLabel);

	// #################################################################################################
	//	
	// #################################################################################################
	this->m_MainGroup = new UIX::UIXGroupBox();
	this->m_MainGroup->SetSize({ 308, 84 });
	this->m_MainGroup->SetLocation({ 480, 10 });
	this->m_MainGroup->SetTabIndex(0);
	this->m_MainGroup->SetText("Main");
	this->m_MainGroup->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->AddControl(this->m_MainGroup);

	this->m_MainGroupExt = new UIX::UIXGroupBox();
	this->m_MainGroupExt->SetSize({ 308, 55 });
	this->m_MainGroupExt->SetLocation({ 480, 93 });
	this->m_MainGroupExt->SetTabIndex(0);
	this->m_MainGroupExt->SetText("");
	this->m_MainGroupExt->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->AddControl(this->m_MainGroupExt);

	this->m_ModeCombo = new UIX::UIXComboBox();
	this->m_ModeCombo->SetSize({ 82, 25 });
	this->m_ModeCombo->SetLocation({ 15, 25 });
	this->m_ModeCombo->SetTabIndex(0);
	this->m_ModeCombo->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_ModeCombo->SetDropDownStyle(Forms::ComboBoxStyle::DropDownList);
	this->m_ModeCombo->Items.Add("Host");
	this->m_ModeCombo->Items.Add("Server");
	this->m_ModeCombo->Items.Add("Client");
	this->m_MainGroup->AddControl(this->m_ModeCombo);

	this->m_ModeLabel = new UIX::UIXLabel();
	this->m_ModeLabel->SetSize({ 50, 25 });
	this->m_ModeLabel->SetLocation({ 100, 28 });
	this->m_ModeLabel->SetTabIndex(0);
	this->m_ModeLabel->SetText("Mode");
	this->m_ModeLabel->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_ModeLabel->SetTextAlign(Drawing::ContentAlignment::TopLeft);
	this->m_MainGroup->AddControl(this->m_ModeLabel);

	this->m_CustomDllTextBox = new UIX::UIXTextBox();
	this->m_CustomDllTextBox->SetSize({ 80, 21 });
	this->m_CustomDllTextBox->SetLocation({ 150, 25 });
	this->m_CustomDllTextBox->SetTabIndex(0);
	this->m_CustomDllTextBox->SetText("");
	this->m_CustomDllTextBox->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_MainGroup->AddControl(this->m_CustomDllTextBox);

	this->m_CustomDllLabel = new UIX::UIXLabel();
	this->m_CustomDllLabel->SetSize({ 70, 21 });
	this->m_CustomDllLabel->SetLocation({ 233, 28 });
	this->m_CustomDllLabel->SetTabIndex(0);
	this->m_CustomDllLabel->SetText("Additional dll's");
	this->m_CustomDllLabel->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_MainGroup->AddControl(this->m_CustomDllLabel);

	this->m_LaunchArgsTextBox = new UIX::UIXTextBox();
	this->m_LaunchArgsTextBox->SetSize({ 215, 21 });
	this->m_LaunchArgsTextBox->SetLocation({ 15, 50 });
	this->m_LaunchArgsTextBox->SetTabIndex(0);
	this->m_LaunchArgsTextBox->SetText("");
	this->m_LaunchArgsTextBox->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_MainGroup->AddControl(this->m_LaunchArgsTextBox);

	this->m_LaunchArgsLabel = new UIX::UIXLabel();
	this->m_LaunchArgsLabel->SetSize({ 70, 21 });
	this->m_LaunchArgsLabel->SetLocation({ 233, 53 });
	this->m_LaunchArgsLabel->SetTabIndex(0);
	this->m_LaunchArgsLabel->SetText("Launch flags");
	this->m_LaunchArgsLabel->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_MainGroup->AddControl(this->m_LaunchArgsLabel);

	this->m_CleanSDK = new UIX::UIXButton();
	this->m_CleanSDK->SetSize({ 110, 18 });
	this->m_CleanSDK->SetLocation({ 15, 7 });
	this->m_CleanSDK->SetTabIndex(0);
	this->m_CleanSDK->SetText("Clean SDK");
	this->m_CleanSDK->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_MainGroupExt->AddControl(this->m_CleanSDK);

	this->m_UpdateSDK = new UIX::UIXButton();
	this->m_UpdateSDK->SetSize({ 110, 18 });
	this->m_UpdateSDK->SetLocation({ 15, 30 });
	this->m_UpdateSDK->SetTabIndex(0);
	this->m_UpdateSDK->SetText("Update SDK");
	this->m_UpdateSDK->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_MainGroupExt->AddControl(this->m_UpdateSDK);

	this->m_LaunchSDK = new UIX::UIXButton();
	this->m_LaunchSDK->SetSize({ 170, 41 });
	this->m_LaunchSDK->SetLocation({ 130, 7 });
	this->m_LaunchSDK->SetTabIndex(0);
	this->m_LaunchSDK->SetText("Launch game");
	this->m_LaunchSDK->SetBackColor(Drawing::Color(3, 102, 214));
	this->m_LaunchSDK->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_MainGroupExt->AddControl(this->m_LaunchSDK);

	// #################################################################################################
	//	
	// #################################################################################################
	this->m_EngineBaseGroup = new UIX::UIXGroupBox();
	this->m_EngineBaseGroup->SetSize({ 337, 73 });
	this->m_EngineBaseGroup->SetLocation({ 12, 158 });
	this->m_EngineBaseGroup->SetTabIndex(0);
	this->m_EngineBaseGroup->SetText("Engine");
	this->m_EngineBaseGroup->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->AddControl(this->m_EngineBaseGroup);

	this->m_EngineNetworkGroup = new UIX::UIXGroupBox();
	this->m_EngineNetworkGroup->SetSize({ 337, 55 });
	this->m_EngineNetworkGroup->SetLocation({ 12, 230 });
	this->m_EngineNetworkGroup->SetTabIndex(0);
	this->m_EngineNetworkGroup->SetText("");
	this->m_EngineNetworkGroup->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->AddControl(this->m_EngineNetworkGroup);

	this->m_EngineVideoGroup = new UIX::UIXGroupBox();
	this->m_EngineVideoGroup->SetSize({ 337, 55 });
	this->m_EngineVideoGroup->SetLocation({ 12, 284 });
	this->m_EngineVideoGroup->SetTabIndex(0);
	this->m_EngineVideoGroup->SetText("");
	this->m_EngineVideoGroup->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->AddControl(this->m_EngineVideoGroup);

	this->m_ReservedCoresTextBox = new UIX::UIXTextBox();
	this->m_ReservedCoresTextBox->SetSize({ 18, 18 });
	this->m_ReservedCoresTextBox->SetLocation({ 15, 25 });
	this->m_ReservedCoresTextBox->SetTabIndex(0);
	this->m_ReservedCoresTextBox->SetReadOnly(false);
	this->m_ReservedCoresTextBox->SetText("0");
	this->m_ReservedCoresTextBox->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_EngineBaseGroup->AddControl(this->m_ReservedCoresTextBox);

	this->m_ReservedCoresLabel = new UIX::UIXLabel();
	this->m_ReservedCoresLabel->SetSize({ 125, 18 });
	this->m_ReservedCoresLabel->SetLocation({ 36, 27 });
	this->m_ReservedCoresLabel->SetTabIndex(0);
	this->m_ReservedCoresLabel->SetText("Reserved cores");
	this->m_ReservedCoresLabel->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_ReservedCoresLabel->SetTextAlign(Drawing::ContentAlignment::TopLeft);
	this->m_EngineBaseGroup->AddControl(this->m_ReservedCoresLabel);

	this->m_WorkerThreadsTextBox = new UIX::UIXTextBox();
	this->m_WorkerThreadsTextBox->SetSize({ 18, 18 });
	this->m_WorkerThreadsTextBox->SetLocation({ 155, 25 });
	this->m_WorkerThreadsTextBox->SetTabIndex(0);
	this->m_WorkerThreadsTextBox->SetReadOnly(false);
	this->m_WorkerThreadsTextBox->SetText("28");
	this->m_WorkerThreadsTextBox->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_EngineBaseGroup->AddControl(this->m_WorkerThreadsTextBox);

	this->m_WorkerThreadsLabel = new UIX::UIXLabel();
	this->m_WorkerThreadsLabel->SetSize({ 125, 18 });
	this->m_WorkerThreadsLabel->SetLocation({ 176, 27 });
	this->m_WorkerThreadsLabel->SetTabIndex(0);
	this->m_WorkerThreadsLabel->SetText("Worker threads");
	this->m_WorkerThreadsLabel->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_WorkerThreadsLabel->SetTextAlign(Drawing::ContentAlignment::TopLeft);
	this->m_EngineBaseGroup->AddControl(this->m_WorkerThreadsLabel);

	this->m_SingleCoreDediToggle = new UIX::UIXCheckBox();
	this->m_SingleCoreDediToggle->SetSize({ 125, 18 });
	this->m_SingleCoreDediToggle->SetLocation({ 15, 48 });
	this->m_SingleCoreDediToggle->SetTabIndex(0);
	this->m_SingleCoreDediToggle->SetText("Single-core server");
	this->m_SingleCoreDediToggle->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_EngineBaseGroup->AddControl(this->m_SingleCoreDediToggle);

	this->m_NoAsyncJobsToggle = new UIX::UIXCheckBox();
	this->m_NoAsyncJobsToggle->SetSize({ 125, 18 });
	this->m_NoAsyncJobsToggle->SetLocation({ 155, 48 });
	this->m_NoAsyncJobsToggle->SetTabIndex(2);
	this->m_NoAsyncJobsToggle->SetText("Synchronize jobs");
	this->m_NoAsyncJobsToggle->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_EngineBaseGroup->AddControl(this->m_NoAsyncJobsToggle);

	this->m_NetEncryptionToggle = new UIX::UIXCheckBox();
	this->m_NetEncryptionToggle->SetSize({ 125, 18 });
	this->m_NetEncryptionToggle->SetLocation({ 15, 7 });
	this->m_NetEncryptionToggle->SetTabIndex(0);
	this->m_NetEncryptionToggle->SetChecked(true);
	this->m_NetEncryptionToggle->SetText("Net encryption");
	this->m_NetEncryptionToggle->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_EngineNetworkGroup->AddControl(this->m_NetEncryptionToggle);

	this->m_NetRandomKeyToggle = new UIX::UIXCheckBox();
	this->m_NetRandomKeyToggle->SetSize({ 125, 18 });
	this->m_NetRandomKeyToggle->SetLocation({ 155, 7 });
	this->m_NetRandomKeyToggle->SetTabIndex(0);
	this->m_NetRandomKeyToggle->SetChecked(true);
	this->m_NetRandomKeyToggle->SetText("Net random key");
	this->m_NetRandomKeyToggle->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_EngineNetworkGroup->AddControl(this->m_NetRandomKeyToggle);


	this->m_QueuedPacketThread = new UIX::UIXCheckBox();
	this->m_QueuedPacketThread->SetSize({ 125, 18 });
	this->m_QueuedPacketThread->SetLocation({ 15, 30 });
	this->m_QueuedPacketThread->SetTabIndex(2);
	this->m_QueuedPacketThread->SetText("No queued packets");
	this->m_QueuedPacketThread->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_EngineNetworkGroup->AddControl(this->m_QueuedPacketThread);

	this->m_NoTimeOut = new UIX::UIXCheckBox();
	this->m_NoTimeOut->SetSize({ 125, 18 });
	this->m_NoTimeOut->SetLocation({ 155, 30 });
	this->m_NoTimeOut->SetTabIndex(0);
	this->m_NoTimeOut->SetText("No time out");
	this->m_NoTimeOut->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_EngineNetworkGroup->AddControl(this->m_NoTimeOut);


	this->m_WindowedToggle = new UIX::UIXCheckBox();
	this->m_WindowedToggle->SetSize({ 105, 18 });
	this->m_WindowedToggle->SetLocation({ 15, 7 });
	this->m_WindowedToggle->SetTabIndex(0);
	this->m_WindowedToggle->SetChecked(true);
	this->m_WindowedToggle->SetText("Windowed");
	this->m_WindowedToggle->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_EngineVideoGroup->AddControl(this->m_WindowedToggle);

	this->m_BorderlessToggle = new UIX::UIXCheckBox();
	this->m_BorderlessToggle->SetSize({ 150, 18 });
	this->m_BorderlessToggle->SetLocation({ 155, 7 });
	this->m_BorderlessToggle->SetTabIndex(0);
	this->m_BorderlessToggle->SetText("No border");
	this->m_BorderlessToggle->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_EngineVideoGroup->AddControl(this->m_BorderlessToggle);

	this->m_FpsTextBox = new UIX::UIXTextBox();
	this->m_FpsTextBox->SetSize({ 25, 18 });
	this->m_FpsTextBox->SetLocation({ 15, 30 });
	this->m_FpsTextBox->SetTabIndex(0);
	this->m_FpsTextBox->SetReadOnly(false);
	this->m_FpsTextBox->SetText("-1");
	this->m_FpsTextBox->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_EngineVideoGroup->AddControl(this->m_FpsTextBox);

	this->m_FpsLabel = new UIX::UIXLabel();
	this->m_FpsLabel->SetSize({ 125, 18 });
	this->m_FpsLabel->SetLocation({ 43, 32 });
	this->m_FpsLabel->SetTabIndex(0);
	this->m_FpsLabel->SetText("Max FPS");
	this->m_FpsLabel->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_FpsLabel->SetTextAlign(Drawing::ContentAlignment::TopLeft);
	this->m_EngineVideoGroup->AddControl(this->m_FpsLabel);


	this->m_WidthTextBox = new UIX::UIXTextBox();
	this->m_WidthTextBox->SetSize({ 50, 18 });
	this->m_WidthTextBox->SetLocation({ 100, 30 });
	this->m_WidthTextBox->SetTabIndex(0);
	this->m_WidthTextBox->SetReadOnly(false);
	this->m_WidthTextBox->SetText("");
	this->m_WidthTextBox->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Right);
	this->m_EngineVideoGroup->AddControl(this->m_WidthTextBox);

	this->m_HeightTextBox = new UIX::UIXTextBox();
	this->m_HeightTextBox->SetSize({ 50, 18 });
	this->m_HeightTextBox->SetLocation({ 149, 30 });
	this->m_HeightTextBox->SetTabIndex(0);
	this->m_HeightTextBox->SetReadOnly(false);
	this->m_HeightTextBox->SetText("");
	this->m_HeightTextBox->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Right);
	this->m_EngineVideoGroup->AddControl(this->m_HeightTextBox);

	this->m_ResolutionLabel = new UIX::UIXLabel();
	this->m_ResolutionLabel->SetSize({ 125, 18 });
	this->m_ResolutionLabel->SetLocation({ 202, 32 });
	this->m_ResolutionLabel->SetTabIndex(0);
	this->m_ResolutionLabel->SetText("Resolution (width | height)");
	this->m_ResolutionLabel->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_ResolutionLabel->SetTextAlign(Drawing::ContentAlignment::TopLeft);
	this->m_EngineVideoGroup->AddControl(this->m_ResolutionLabel);

	// #################################################################################################
	//	
	// #################################################################################################
	this->m_ConsoleGroup = new UIX::UIXGroupBox();
	this->m_ConsoleGroup->SetSize({ 429, 181 });
	this->m_ConsoleGroup->SetLocation({ 359, 158 });
	this->m_ConsoleGroup->SetTabIndex(0);
	this->m_ConsoleGroup->SetText("Console");
	this->m_ConsoleGroup->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left | Forms::AnchorStyles::Right);
	this->AddControl(this->m_ConsoleGroup);

	this->m_ConsoleListView = new UIX::UIXListView();
	this->m_ConsoleListView->SetSize({ 427, 165 });
	this->m_ConsoleListView->SetLocation({ 1, 15 });
	this->m_ConsoleListView->SetTabIndex(0);
	this->m_ConsoleListView->SetText("0");
	this->m_ConsoleListView->SetBackColor(Drawing::Color(29, 33, 37));
	this->m_ConsoleListView->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_ConsoleGroup->AddControl(this->m_ConsoleListView);

	this->ResumeLayout(false);
	this->PerformLayout();
	// END DESIGNER CODE

	this->SetBackColor({ 47, 54, 61 });
}

CUIBasePanel::CUIBasePanel() : Forms::Form()
{
	g_pMainUI = this;
	this->Init();
}
CUIBasePanel* g_pMainUI;