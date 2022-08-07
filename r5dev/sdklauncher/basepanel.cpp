//=============================================================================//
//
// Purpose:
//
//=============================================================================//
#include "core/stdafx.h"
#include "modManager.h"
#include "sdklauncher.h"
#include "basepanel.h"
#include "gameFilesLoc.h"

Drawing::Color bgColor = Drawing::Color(47, 54, 61);
Drawing::Color tickColor = Drawing::Color(3, 102, 214);
Drawing::Color logListColor = Drawing::Color(29, 33, 37);
Drawing::Color tickColor2 = Drawing::Color(3, 102, 214);
ModManager manager;

//-----------------------------------------------------------------------------
// Purpose: creates the surface layout
//-----------------------------------------------------------------------------
void CUIBaseSurface::Init()
{
	// START DESIGNER CODE
	const INT WindowX = 800;
	const INT WindowY = 560;

	this->SuspendLayout();
	this->SetAutoScaleDimensions({ 6, 13 });
	this->SetAutoScaleMode(Forms::AutoScaleMode::Font);
	this->SetText("SDK Launcher");
	this->SetClientSize({ WindowX, WindowY });
	this->SetFormBorderStyle(Forms::FormBorderStyle::FixedSingle);
	this->SetStartPosition(Forms::FormStartPosition::CenterParent);
	this->SetMinimizeBox(false);
	this->SetMaximizeBox(false);
	this->SetBackColor(bgColor);

	// ########################################################################
	//	GAME
	// ########################################################################
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
	this->m_MapCombo->SetSelectedIndex(-1);
	this->m_MapCombo->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_MapCombo->SetDropDownStyle(Forms::ComboBoxStyle::DropDownList);
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
	this->m_PlaylistCombo->SetSelectedIndex(-1);
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
	this->m_PlaylistFileTextBox->LostFocus += &ReloadPlaylists;
	this->m_GameGroupExt->AddControl(this->m_PlaylistFileTextBox);

	this->m_PlaylistFileLabel = new UIX::UIXLabel();
	this->m_PlaylistFileLabel->SetSize({ 50, 18 });
	this->m_PlaylistFileLabel->SetLocation({ 311, 32 });
	this->m_PlaylistFileLabel->SetTabIndex(0);
	this->m_PlaylistFileLabel->SetText("Playlist file");
	this->m_PlaylistFileLabel->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left);
	this->m_GameGroupExt->AddControl(this->m_PlaylistFileLabel);

	// ########################################################################
	//	MAIN
	// ########################################################################
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
	this->m_ModeCombo->SetSelectedIndex(0);
	this->m_ModeCombo->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_ModeCombo->SetDropDownStyle(Forms::ComboBoxStyle::DropDownList);
	this->m_MainGroup->AddControl(this->m_ModeCombo);

	this->m_ModeLabel = new UIX::UIXLabel();
	this->m_ModeLabel->SetSize({ 50, 25 });
	this->m_ModeLabel->SetLocation({ 100, 28 });
	this->m_ModeLabel->SetTabIndex(0);
	this->m_ModeLabel->SetText("Mode");
	this->m_ModeLabel->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_ModeLabel->SetTextAlign(Drawing::ContentAlignment::TopLeft);
	this->m_MainGroup->AddControl(this->m_ModeLabel);

	this->m_HostNameTextBox = new UIX::UIXTextBox();
	this->m_HostNameTextBox->SetSize({ 80, 21 });
	this->m_HostNameTextBox->SetLocation({ 150, 25 });
	this->m_HostNameTextBox->SetTabIndex(0);
	this->m_HostNameTextBox->SetText("");
	this->m_HostNameTextBox->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_MainGroup->AddControl(this->m_HostNameTextBox);

	this->m_HostNameLabel = new UIX::UIXLabel();
	this->m_HostNameLabel->SetSize({ 70, 21 });
	this->m_HostNameLabel->SetLocation({ 233, 28 });
	this->m_HostNameLabel->SetTabIndex(0);
	this->m_HostNameLabel->SetText("Host name");
	this->m_HostNameLabel->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_MainGroup->AddControl(this->m_HostNameLabel);

	this->m_VisibilityCombo = new UIX::UIXComboBox();
	this->m_VisibilityCombo->SetSize({ 82, 25 });
	this->m_VisibilityCombo->SetLocation({ 15, 50 });
	this->m_VisibilityCombo->SetTabIndex(0);
	this->m_VisibilityCombo->SetSelectedIndex(0);
	this->m_VisibilityCombo->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_VisibilityCombo->SetDropDownStyle(Forms::ComboBoxStyle::DropDownList);
	this->m_MainGroup->AddControl(this->m_VisibilityCombo);

	this->m_VisibilityLabel = new UIX::UIXLabel();
	this->m_VisibilityLabel->SetSize({ 70, 21 });
	this->m_VisibilityLabel->SetLocation({ 100, 53 });
	this->m_VisibilityLabel->SetTabIndex(0);
	this->m_VisibilityLabel->SetText("Visibility");
	this->m_VisibilityLabel->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_MainGroup->AddControl(this->m_VisibilityLabel);

	this->m_LaunchArgsTextBox = new UIX::UIXTextBox();
	this->m_LaunchArgsTextBox->SetSize({ 80, 21 });
	this->m_LaunchArgsTextBox->SetLocation({ 150, 50 });
	this->m_LaunchArgsTextBox->SetTabIndex(0);
	this->m_LaunchArgsTextBox->SetText("");
	this->m_LaunchArgsTextBox->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_MainGroup->AddControl(this->m_LaunchArgsTextBox);

	this->m_LaunchArgsLabel = new UIX::UIXLabel();
	this->m_LaunchArgsLabel->SetSize({ 70, 21 });
	this->m_LaunchArgsLabel->SetLocation({ 233, 53 });
	this->m_LaunchArgsLabel->SetTabIndex(0);
	this->m_LaunchArgsLabel->SetText("Command line");
	this->m_LaunchArgsLabel->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_MainGroup->AddControl(this->m_LaunchArgsLabel);

	this->m_CleanSDK = new UIX::UIXButton();
	this->m_CleanSDK->SetSize({ 110, 18 });
	this->m_CleanSDK->SetLocation({ 15, 7 });
	this->m_CleanSDK->SetTabIndex(0);
	this->m_CleanSDK->SetText("Clean SDK");
	this->m_CleanSDK->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_CleanSDK->Click += &CleanSDK;
	this->m_MainGroupExt->AddControl(this->m_CleanSDK);

	this->m_UpdateSDK = new UIX::UIXButton();
	this->m_UpdateSDK->SetSize({ 110, 18 });
	this->m_UpdateSDK->SetLocation({ 15, 30 });
	this->m_UpdateSDK->SetTabIndex(0);
	this->m_UpdateSDK->SetEnabled(false); // !TODO: Implement updater
	this->m_UpdateSDK->SetText("Update SDK");
	this->m_UpdateSDK->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_UpdateSDK->Click += UpdateSDKChecker;
	this->m_MainGroupExt->AddControl(this->m_UpdateSDK);

	this->m_LaunchSDK = new UIX::UIXButton();
	this->m_LaunchSDK->SetSize({ 170, 41 });
	this->m_LaunchSDK->SetLocation({ 130, 7 });
	this->m_LaunchSDK->SetTabIndex(0);
	this->m_LaunchSDK->SetText("Launch game");
	this->m_LaunchSDK->SetBackColor(tickColor);
	this->m_LaunchSDK->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_LaunchSDK->Click += &LaunchGame;
	this->m_MainGroupExt->AddControl(this->m_LaunchSDK);

	// ########################################################################
	//	ENGINE
	// ########################################################################
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

	this->m_NoQueuedPacketThread = new UIX::UIXCheckBox();
	this->m_NoQueuedPacketThread->SetSize({ 125, 18 });
	this->m_NoQueuedPacketThread->SetLocation({ 15, 30 });
	this->m_NoQueuedPacketThread->SetTabIndex(2);
	this->m_NoQueuedPacketThread->SetText("No queued packets");
	this->m_NoQueuedPacketThread->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_EngineNetworkGroup->AddControl(this->m_NoQueuedPacketThread);

	this->m_NoTimeOutToggle = new UIX::UIXCheckBox();
	this->m_NoTimeOutToggle->SetSize({ 125, 18 });
	this->m_NoTimeOutToggle->SetLocation({ 155, 30 });
	this->m_NoTimeOutToggle->SetTabIndex(0);
	this->m_NoTimeOutToggle->SetText("No time out");
	this->m_NoTimeOutToggle->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_EngineNetworkGroup->AddControl(this->m_NoTimeOutToggle);

	this->m_WindowedToggle = new UIX::UIXCheckBox();
	this->m_WindowedToggle->SetSize({ 105, 18 });
	this->m_WindowedToggle->SetLocation({ 15, 7 });
	this->m_WindowedToggle->SetTabIndex(0);
	this->m_WindowedToggle->SetChecked(true);
	this->m_WindowedToggle->SetText("Windowed");
	this->m_WindowedToggle->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_EngineVideoGroup->AddControl(this->m_WindowedToggle);

	this->m_NoBorderToggle = new UIX::UIXCheckBox();
	this->m_NoBorderToggle->SetSize({ 150, 18 });
	this->m_NoBorderToggle->SetLocation({ 155, 7 });
	this->m_NoBorderToggle->SetTabIndex(0);
	this->m_NoBorderToggle->SetText("No border");
	this->m_NoBorderToggle->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_EngineVideoGroup->AddControl(this->m_NoBorderToggle);

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

	// ########################################################################
	//	CONSOLE
	// ########################################################################
	this->m_ConsoleGroup = new UIX::UIXGroupBox();
	this->m_ConsoleGroup->SetSize({ 429, 15 });
	this->m_ConsoleGroup->SetLocation({ 359, 158 });
	this->m_ConsoleGroup->SetTabIndex(0);
	this->m_ConsoleGroup->SetText("Console");
	this->m_ConsoleGroup->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left | Forms::AnchorStyles::Right);
	this->AddControl(this->m_ConsoleGroup);

	this->m_ConsoleGroupExt = new UIX::UIXGroupBox();
	this->m_ConsoleGroupExt->SetSize({ 429, 167 });
	this->m_ConsoleGroupExt->SetLocation({ 359, 172 });
	this->m_ConsoleGroupExt->SetTabIndex(0);
	this->m_ConsoleGroupExt->SetText("");
	this->m_ConsoleGroupExt->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left | Forms::AnchorStyles::Right);
	this->AddControl(this->m_ConsoleGroupExt);

	this->m_ConsoleCommandTextBox = new UIX::UIXTextBox();
	this->m_ConsoleCommandTextBox->SetSize({ 351, 18 });
	this->m_ConsoleCommandTextBox->SetLocation({ 0, 149 });
	this->m_ConsoleCommandTextBox->SetTabIndex(0);
	this->m_ConsoleCommandTextBox->SetReadOnly(false);
	this->m_ConsoleCommandTextBox->SetText("");
	this->m_ConsoleCommandTextBox->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left);
	this->m_ConsoleGroupExt->AddControl(this->m_ConsoleCommandTextBox);

	this->m_ConsoleSendCommand = new UIX::UIXButton();
	this->m_ConsoleSendCommand->SetSize({ 79, 18 });
	this->m_ConsoleSendCommand->SetLocation({ 350, 149 });
	this->m_ConsoleSendCommand->SetTabIndex(0);
	this->m_ConsoleSendCommand->SetText("Send");
	this->m_ConsoleSendCommand->SetBackColor(tickColor2);
	this->m_ConsoleSendCommand->SetAnchor(Forms::AnchorStyles::None);
	this->m_ConsoleSendCommand->Click += &ForwardCommandToGame;
	this->m_ConsoleGroupExt->AddControl(this->m_ConsoleSendCommand);

	// Moved to the bottom so that the scroll bars are hidden. Giving it a cleaner look || If someone doesn't have a scrollwheel, sucks to be them
	this->m_ConsoleListView = new UIX::UIXListView();
	this->m_ConsoleListView->SetSize({ 445, 174 });
	this->m_ConsoleListView->SetLocation({ 1, -23 }); // Hide columns
	this->m_ConsoleListView->SetTabIndex(0);
	this->m_ConsoleListView->SetBackColor(logListColor);
	this->m_ConsoleListView->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left | Forms::AnchorStyles::Right);
	this->m_ConsoleListView->SetView(Forms::View::Details);
	this->m_ConsoleListView->SetVirtualMode(true);
	this->m_ConsoleListView->SetFullRowSelect(true);
	this->m_ConsoleListView->Columns.Add({ "index", 40 });
	this->m_ConsoleListView->Columns.Add({ "buffer", 387 });
	this->m_ConsoleListView->MouseClick += &VirtualItemToClipboard;
	this->m_ConsoleListView->RetrieveVirtualItem += &GetVirtualItem;
	this->m_ConsoleGroupExt->AddControl(this->m_ConsoleListView);

	// ########################################################################
	//	MOD MANAGER
	// ########################################################################
	this->m_ManagerSurroundingBox = new UIX::UIXGroupBox();
	this->m_ManagerSurroundingBox->SetSize({ 780, 205 });
	this->m_ManagerSurroundingBox->SetLocation({ 12, 350 });
	this->m_ManagerSurroundingBox->SetTabIndex(0);
	this->m_ManagerSurroundingBox->SetText("Mod Manager");
	this->m_ManagerSurroundingBox->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left | Forms::AnchorStyles::Right);
	this->AddControl(this->m_ManagerSurroundingBox);

	this->m_ManagerGroupExt = new UIX::UIXGroupBox();
	this->m_ManagerGroupExt->SetSize({ 780, 172 });
	this->m_ManagerGroupExt->SetLocation({ 0, 15 });
	this->m_ManagerGroupExt->SetTabIndex(0);
	this->m_ManagerGroupExt->SetText("");
	this->m_ManagerGroupExt->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left | Forms::AnchorStyles::Right);
	this->m_ManagerSurroundingBox->AddControl(this->m_ManagerGroupExt);

	this->m_ManagerGroup = new UIX::UIXGroupBox();
	this->m_ManagerGroup->SetSize({ 780, 190 });
	this->m_ManagerGroup->SetLocation({ 0, 15 });
	this->m_ManagerGroup->SetTabIndex(0);
	this->m_ManagerGroup->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left | Forms::AnchorStyles::Right);
	this->m_ManagerSurroundingBox->AddControl(this->m_ManagerGroup);

	this->m_ManagerControlsGroup = new UIX::UIXGroupBox();
	this->m_ManagerControlsGroup->SetSize({ 779, 18 });
	this->m_ManagerControlsGroup->SetLocation({ 1, 172 });
	this->m_ManagerGroup->SetText("");
	this->m_ManagerControlsGroup->SetTabIndex(0);
	this->m_ManagerControlsGroup->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left | Forms::AnchorStyles::Right);
	this->m_ManagerGroup->AddControl(this->m_ManagerControlsGroup);

	this->m_ManagerControlsValidText = new UIX::UIXTextBox();
	this->m_ManagerControlsValidText->SetSize({ 80, 18 });
	this->m_ManagerControlsValidText->SetLocation({ 0, 0 });
	this->m_ManagerControlsValidText->SetTabIndex(0);
	this->m_ManagerControlsValidText->SetReadOnly(true);
	this->m_ManagerControlsValidText->SetForeColor(Drawing::Color(92, 236, 89));
	this->m_ManagerControlsValidText->SetText(std::string("Valid Mods: " + std::to_string(validMods)).c_str());
	this->m_ManagerControlsValidText->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left | Forms::AnchorStyles::Right);
	this->m_ManagerControlsGroup->AddControl(this->m_ManagerControlsValidText);

	this->m_ManagerControlsEnabledText = new UIX::UIXTextBox();
	this->m_ManagerControlsEnabledText->SetSize({ 93, 18 });
	this->m_ManagerControlsEnabledText->SetLocation({ 79, 0 });
	this->m_ManagerControlsEnabledText->SetTabIndex(0);
	this->m_ManagerControlsEnabledText->SetReadOnly(true);
	this->m_ManagerControlsEnabledText->SetForeColor(Drawing::Color(0, 120, 215));
	this->m_ManagerControlsEnabledText->SetText(std::string("Enabled Mods: " + std::to_string(enabledMods)).c_str());
	this->m_ManagerControlsEnabledText->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left | Forms::AnchorStyles::Right);
	this->m_ManagerControlsGroup->AddControl(this->m_ManagerControlsEnabledText);

	this->m_ManagerUseModsToggle = new UIX::UIXCheckBox();
	this->m_ManagerUseModsToggle->SetSize({ 80, 16 });
	this->m_ManagerUseModsToggle->SetLocation({ 180, 1 });
	this->m_ManagerUseModsToggle->SetTabIndex(0);
	this->m_ManagerUseModsToggle->SetText("Use Mods");
	this->m_ManagerUseModsToggle->SetChecked(true);
	this->m_ManagerUseModsToggle->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left | Forms::AnchorStyles::Right);
	this->m_ManagerUseModsToggle->SetTextAlign(Drawing::ContentAlignment::MiddleLeft);
	this->m_ManagerControlsGroup->AddControl(this->m_ManagerUseModsToggle);

	this->ModsListView = new UIX::UIXListView();
	this->ModsListView->SetSize({ 350, 194 });
	this->ModsListView->SetLocation({ 1, -24 }); // Hide columns
	this->ModsListView->SetTabIndex(0);
	this->ModsListView->SetBackColor(logListColor);
	this->ModsListView->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left | Forms::AnchorStyles::Right);
	this->ModsListView->SetView(Forms::View::Details);
	this->ModsListView->SetVirtualMode(true);
	this->ModsListView->SetFullRowSelect(true);
	this->ModsListView->Columns.Add({ "index", 50 });
	this->ModsListView->Columns.Add({ "buffer", 300 });
	this->ModsListView->MouseClick += &ModManagerClick;
	//this->ModsListView->LostFocus += &UnfocusedManager;
	this->ModsListView->RetrieveVirtualItem += &GetVirtItemMod;
	this->m_ManagerGroupExt->AddControl(this->ModsListView);

	this->m_ManagerViewerBox = new UIX::UIXGroupBox();
	this->m_ManagerViewerBox->SetSize({ 429, 194 });
	this->m_ManagerViewerBox->SetLocation({ 351, 0 });
	this->m_ManagerViewerBox->SetForeColor(bgColor);
	this->m_ManagerGroupExt->AddControl(this->m_ManagerViewerBox);

	HFONT boldFontHf = CreateFontA(36, 0, 0, 0, FW_BLACK, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial");
	HFONT italicsFontHf = CreateFontA(18, 0, 0, 0, FW_MEDIUM, TRUE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial");
	HFONT defaultFontHf = CreateFontA(16, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial");

	Drawing::Font* boldFont = new Drawing::Font(this->GetHandle(), boldFontHf);
	Drawing::Font* italicsFont = new Drawing::Font(this->GetHandle(), italicsFontHf);
	Drawing::Font* defaultFont = new Drawing::Font(this->GetHandle(), defaultFontHf);

	this->m_ManagerViewerNameLabel = new UIX::UIXLabel();
	this->m_ManagerViewerNameLabel->SetSize({ 420, 32 });
	this->m_ManagerViewerNameLabel->SetLocation({ 5, 1 });
	this->m_ManagerViewerNameLabel->SetTabIndex(0);
	this->m_ManagerViewerNameLabel->SetText("Placeholder");
	this->m_ManagerViewerNameLabel->SetFont(boldFont);
	this->m_ManagerViewerBox->AddControl(this->m_ManagerViewerNameLabel);

	this->m_ManagerViewerDescText = new UIX::UIXTextBox();
	this->m_ManagerViewerDescText->SetSize({ 410, 52 });
	this->m_ManagerViewerDescText->SetLocation({ 8, 40 });
	this->m_ManagerViewerDescText->SetTabIndex(0);
	this->m_ManagerViewerDescText->SetText("Lorem ipsum dolor sit amet, consectetur adipiscing elit");
	this->m_ManagerViewerDescText->SetWordWrap(true);
	this->m_ManagerViewerDescText->SetMultiline(true);
	this->m_ManagerViewerDescText->SetReadOnly(true);
	this->m_ManagerViewerDescText->SetTextAlign(Forms::HorizontalAlignment::Left);
	this->m_ManagerViewerDescText->SetBorderStyle(Forms::BorderStyle::None);
	this->m_ManagerViewerDescText->SetFont(italicsFont);
	this->m_ManagerViewerBox->AddControl(this->m_ManagerViewerDescText);

	this->m_ManagerViewerAuthorLabel = new UIX::UIXLabel();
	this->m_ManagerViewerAuthorLabel->SetSize({ 350, 18 });
	this->m_ManagerViewerAuthorLabel->SetLocation({ 8, 112 });
	this->m_ManagerViewerAuthorLabel->SetTabIndex(0);
	this->m_ManagerViewerAuthorLabel->SetText("Authors: ");
	this->m_ManagerViewerAuthorLabel->SetFont(defaultFont);
	this->m_ManagerViewerBox->AddControl(this->m_ManagerViewerAuthorLabel);

	this->m_ManagerViewerVersionLabel = new UIX::UIXLabel();
	this->m_ManagerViewerVersionLabel->SetSize({ 350, 18 });
	this->m_ManagerViewerVersionLabel->SetLocation({ 8, 132 });
	this->m_ManagerViewerVersionLabel->SetTabIndex(0);
	this->m_ManagerViewerVersionLabel->SetText("Version: ");
	this->m_ManagerViewerVersionLabel->SetFont(defaultFont);
	this->m_ManagerViewerBox->AddControl(this->m_ManagerViewerVersionLabel);

	this->m_ManagerViewerAppidLabel = new UIX::UIXLabel();
	this->m_ManagerViewerAppidLabel->SetSize({ 350, 18 });
	this->m_ManagerViewerAppidLabel->SetLocation({ 8, 152 });
	this->m_ManagerViewerAppidLabel->SetTabIndex(0);
	this->m_ManagerViewerAppidLabel->SetText("AppID: ");
	this->m_ManagerViewerAppidLabel->SetFont(defaultFont);
	this->m_ManagerViewerBox->AddControl(this->m_ManagerViewerAppidLabel);

	this->m_ManagerEnabledBox = new UIX::UIXGroupBox();
	this->m_ManagerEnabledBox->SetSize({ 60, 60 });
	this->m_ManagerEnabledBox->SetLocation({ 369, 114 });
	this->m_ManagerEnabledBox->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Right);
	this->m_ManagerEnabledBox->BringToFront();
	this->m_ManagerViewerBox->AddControl(this->m_ManagerEnabledBox);

	this->m_ManagerEnabledToggle = new UIX::UIXCheckBox();
	this->m_ManagerEnabledToggle->SetSize({ 34, 34 });
	this->m_ManagerEnabledToggle->SetLocation({ 13, 5 });
	this->m_ManagerEnabledToggle->SetTabIndex(0);
	this->m_ManagerEnabledToggle->SetText("");
	this->m_ManagerEnabledToggle->SetFlatStyle(Forms::FlatStyle::Flat);
	this->m_ManagerEnabledToggle->Click += &ModManagerEnabledToggle;
	this->m_ManagerEnabledBox->AddControl(this->m_ManagerEnabledToggle);

	this->m_ManagerEnabledLabel = new UIX::UIXLabel();
	this->m_ManagerEnabledLabel->SetSize({ 50, 18 });
	this->m_ManagerEnabledLabel->SetLocation({ 6, 40 });
	this->m_ManagerEnabledLabel->SetTabIndex(0);
	this->m_ManagerEnabledLabel->SetText("Enabled");
	this->m_ManagerEnabledLabel->SetBackColor(Drawing::Color(92, 236, 89));
	this->m_ManagerEnabledLabel->SetFont(defaultFont);
	this->m_ManagerEnabledBox->AddControl(this->m_ManagerEnabledLabel);

	this->m_ManagerViewerCoverText = new UIX::UIXTextBox();
	this->m_ManagerViewerCoverText->SetSize({ 429, 194 });
	this->m_ManagerViewerCoverText->SetLocation({ 351, 0 });
	this->m_ManagerViewerCoverText->SetTabIndex(0);
	this->m_ManagerViewerCoverText->SetText("Select mod...");
	this->m_ManagerViewerCoverText->SetReadOnly(true);
	this->m_ManagerViewerCoverText->SetTextAlign(Forms::HorizontalAlignment::Center);
	this->m_ManagerGroupExt->AddControl(this->m_ManagerViewerCoverText);

	this->ResumeLayout(false);
	this->PerformLayout();

	// END DESIGNER CODE
}

//-----------------------------------------------------------------------------
// Purpose: post-init surface setup
//-----------------------------------------------------------------------------
void CUIBaseSurface::Setup()
{
	this->ParseMaps();
	this->ParsePlaylists();
	this->ReadModJson();
	this->InstallGameCheck();
	std::thread thread(LatestSDKCompare, this->FindForm());
	thread.detach();

	// Launcher.cfg setup
	ini = new CSimpleIniA;
	ini->SetUnicode();

	fs::create_directories("platform\\cfg");

	SI_Error rc = ini->LoadFile("platform\\cfg\\launcher.cfg");
	if (rc < 0) {
		// TODO: Error handling
	}

	fs::remove_all("launcher-old.exe"); // This is related to the Update SDK feature. It just makes sure the old launcher is gone.

	this->m_ModeCombo->Items.Add("Host");
	this->m_ModeCombo->Items.Add("Server");
	this->m_ModeCombo->Items.Add("Client");

#ifdef DEDI_LAUNCHER
	this->m_ModeCombo->SetSelectedIndex(1);
#endif // DEDI_LAUNCHER

	this->m_VisibilityCombo->Items.Add("Public");
	this->m_VisibilityCombo->Items.Add("Hidden");
	this->m_VisibilityCombo->Items.Add("Offline");
}

//-----------------------------------------------------------------------------
// Purpose: Gets the requested config object. Each config object must be added to
// v_LauncherValues so there's a default return value
//-----------------------------------------------------------------------------
std::string CUIBaseSurface::GetConfigCFG(const std::string& first) {
	const char* pv;
	std::string defaultVal;
	for (auto& i : v_LauncherValues) {
		if (i.first == first) {
			defaultVal = i.second;
			break;
		}
	}
	return ini->GetValue("Launcher", first.c_str(), defaultVal.c_str());
}

//-----------------------------------------------------------------------------
// Purpose: Sets the requested config object
//-----------------------------------------------------------------------------
void CUIBaseSurface::SetConfigCFG(const std::string& first, const std::string& second) {
	std::string secondT = "\"" + second + "\"";
	ini->SetValue("Launcher", first.c_str(), secondT.c_str());
	ini->SaveFile("platform\\cfg\\launcher.cfg");
}
	
//-----------------------------------------------------------------------------
// Purpose: Checks the releases page for updates, and converts it into a json object
//-----------------------------------------------------------------------------
void CUIBaseSurface::LatestSDKCompare(Forms::Control* pSender) {
	CUIBaseSurface* pSurface = reinterpret_cast<CUIBaseSurface*>(pSender->FindForm());
	std::string clientVersion = pSurface->GetConfigCFG("version");
	if (clientVersion != "") {
		clientVersion.erase(clientVersion.begin(), clientVersion.begin() + 1);
		clientVersion.erase(clientVersion.end() - 1, clientVersion.end());
	}
	pSurface->newVersion = "";
	std::ostringstream ss;

	std::list<std::string> headers{ "User-Agent: R5Realoaded-Installer" };

	cURLpp::Easy easy;
	easy.setOpt(cURLpp::Options::Url("https://api.github.com/repos/Mauler125/r5sdk/releases/latest"));
	easy.setOpt(cURLpp::Options::WriteStream(&ss));
	easy.setOpt(cURLpp::Options::HttpHeader(headers));
	easy.setOpt(cURLpp::Options::NoSignal(true));

	try {
		easy.perform();
	}
	catch (cURLpp::RuntimeError& e) {
		pSurface->logText("Error while checking for updates: " + std::string(e.what()));
	}

	try {
		json jason = json::parse(ss.str());
		pSurface->newVersion = jason["tag_name"];
		pSurface->sdkDownloadLocation = jason["assets"][0]["browser_download_url"];
		//pSurface->logText("Download Location: " + pSurface->sdkDownloadLocation);
	}
	catch (json::parse_error& e) {
		pSurface->logText("Error parsing updates: " + std::string(e.what()));
	}

	if (clientVersion != pSurface->newVersion)
		pSurface->m_UpdateSDK->SetEnabled(true);
}

//-----------------------------------------------------------------------------
// Purpose: Determines what it should do
//-----------------------------------------------------------------------------
void CUIBaseSurface::UpdateSDKChecker(Forms::Control* pSender) {
	CUIBaseSurface* pSurface = reinterpret_cast<CUIBaseSurface*>(pSender->FindForm());

	if (pSurface->sdkDownState == idle) {
		std::thread thread(DownloadSDK, pSurface->FindForm());
		thread.detach();
		pSurface->sdkDownState = sdkDownloadState::downloading;
		pSurface->m_UpdateSDK->SetEnabled(false);
	}
	else if (pSurface->sdkDownState == downloading) {
		pSurface->sdkDownState = sdkDownloadState::download_cancelled;
		pSurface->m_UpdateSDK->SetText("Canceled");
		pSurface->m_UpdateSDK->SetEnabled(false);
	}
}

size_t CUIBaseSurface::ProgressCallback(CUIBaseSurface* pSurface, double dltotal, double dlnow, double ultotal, double ulnow) {
	float progress = ceilf((dlnow / dltotal * 100) * 100) / 100;

	std::ostringstream out;
	out.precision(2);
	out << std::fixed << progress << "%";

	pSurface->m_UpdateSDK->SetText(out.str().c_str());

	if (pSurface->sdkDownState == sdkDownloadState::download_cancelled)
		return 1;

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Downloads the SDK
//-----------------------------------------------------------------------------
void CUIBaseSurface::DownloadSDK(Forms::Control* pSender) {
	CUIBaseSurface* pSurface = reinterpret_cast<CUIBaseSurface*>(pSender->FindForm());

	std::ofstream file("depots.zip", std::ios_base::binary);

	std::list<std::string> headers{ "User-Agent: R5Realoaded-Installer", "Content-Type: application/zip" };

	cURLpp::Easy easy;
	easy.setOpt(cURLpp::Options::Url(pSurface->sdkDownloadLocation));
	easy.setOpt(cURLpp::Options::WriteStream(&file));
	easy.setOpt(cURLpp::Options::HttpHeader(headers));
	easy.setOpt(cURLpp::Options::NoSignal(true));
	easy.setOpt(cURLpp::Options::NoProgress(false));
	easy.setOpt(cURLpp::Options::FollowLocation(true));

	curl_easy_setopt(easy.getHandle(), CURLOPT_PROGRESSDATA, pSurface);
	curl_easy_setopt(easy.getHandle(), CURLOPT_PROGRESSFUNCTION, ProgressCallback);

	try {
		pSurface->m_UpdateSDK->SetEnabled(true);

		easy.perform();

		file.close();

		pSurface->m_UpdateSDK->SetText("Moving Files");

		if (pSurface->sdkDownState != sdkDownloadState::download_cancelled)
			fs::rename("launcher.exe", "launcher-old.exe");

		if (fs::exists("platform\\scripts")) {
			std::string newName = "scripts-" + std::to_string(time(0));
			fs::rename("platform\\scripts", "platform\\" + newName);
			pSurface->logText("Old Scripts have been renamed to: " + newName);
		}

		// Maybe change the name to be dynamic, so if the depots zip isn't named depots, it's fine.
		libzippp::ZipArchive archive("depots.zip");
		if (archive.open(libzippp::ZipArchive::OpenMode::ReadOnly)) {
			pSurface->logText("Unzipping. " + std::to_string(archive.getEntriesCount()) + "  files inside.");
			for (auto& p : archive.getEntries()) {
				if (pSurface->sdkDownState == sdkDownloadState::download_cancelled)
					break;
				try {
					std::string name = "./" + p.getName();
					if (!fs::exists(fs::path(name).remove_filename()))
						fs::create_directories(fs::path(name).remove_filename());
					std::ofstream file(name, std::ios_base::binary);
					p.readContent(file);
					//pSurface->logText(p.getName());
				}
				catch (fs::filesystem_error& e) {
					pSurface->logText("Error: " + (std::string)e.what());
				}
			}
		}
		else
			pSurface->logText("Failed to extract zip");
	}
	catch (cURLpp::RuntimeError& e) {
		pSurface->sdkDownState = sdkDownloadState::download_failed;
		pSurface->m_UpdateSDK->SetText("Error");
	}

	pSurface->m_UpdateSDK->SetText("Finished");
	pSurface->m_UpdateSDK->SetEnabled(false);

	fs::remove_all("depots.zip");

	if (pSurface->sdkDownState != sdkDownloadState::download_cancelled) {
		pSurface->sdkDownState = sdkDownloadState::download_complete;
		pSurface->SetConfigCFG("version", pSurface->newVersion);
		
		// Idk why this isn't working
		//STARTUPINFO si;
		//PROCESS_INFORMATION pi;

		//if (CreateProcess(LPCWSTR("launcher.exe"), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
		//	pSurface->logText("Come again later nerds!");
		//}
		//else 
		//	pSurface->logText("Failed to launch launcher.exe" + GetLastError());

		//CloseHandle(pi.hProcess);
		//CloseHandle(pi.hThread);

		// So this is my substitue
		system("start launcher.exe");
		
		pSurface->Close();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Checks whether the user needs to install the game
//-----------------------------------------------------------------------------
void CUIBaseSurface::InstallGameCheck() {
	std::vector<fs::path> gameFiles;

	for (auto& p : gameFilesLocation) {
		if (fs::exists(p))
			gameFiles.push_back(p);
	}

	//for (auto& i : gameFiles)
	//	logText(i.string());

	//logText(std::to_string(gameFiles.size()) + " Vs. " + std::to_string(gameFilesLocation.size()));

	if (gameFiles != gameFilesLocation) {
		std::sort(gameFiles.begin(), gameFiles.end());
		std::sort(gameFilesLocation.begin(), gameFilesLocation.end());
		std::vector<fs::path> diff;
		std::set_difference(gameFilesLocation.begin(), gameFilesLocation.end(), gameFiles.begin(), gameFiles.end(), std::back_inserter(GameFileDiff));
		for (auto& i : GameFileDiff) {}
		//logText(spdlog::level::level_enum::critical, "Missing: " + i.string());
		bInstallGame = true;
	}

	if (bInstallGame)
		this->m_LaunchSDK->SetText("Install Game");
}

//-----------------------------------------------------------------------------
// Purpose: removes redundant files from the game install
// Input  : *pSender - 
//-----------------------------------------------------------------------------
void CUIBaseSurface::CleanSDK(Forms::Control* pSender)
{
	CUIBaseSurface* pSurface = reinterpret_cast<CUIBaseSurface*>(pSender->FindForm());
	pSurface->m_LogList.push_back(LogList_t(spdlog::level::info, "Running cleaner for SDK installation\n"));
	pSurface->m_ConsoleListView->SetVirtualListSize(static_cast<int32_t>(pSurface->m_LogList.size()));

	if (fs::exists("platform\\clean_sdk.bat")) {
		std::system("platform\\clean_sdk.bat");
	}
}

void CUIBaseSurface::InstallGame(Forms::Control* pSender)
{
	CUIBaseSurface* pSurface = reinterpret_cast<CUIBaseSurface*>(pSender->FindForm());

	try {
		lt::session ses;
		lt::add_torrent_params p = lt::parse_magnet_uri(pSurface->GetConfigCFG("magnet"));
		p.save_path = ".";

		lt::torrent_handle tH = ses.add_torrent(p);

		// This makes sure no files download in the first place. There aren't 2000 files in this, but it works
		for (int i = 0; i < 2000; i++)
			tH.file_priority(i, lt::download_priority_t(lt::dont_download));

		bool adjustValues = false;

		for (;;) {
			std::vector<lt::alert*> alerts;
			ses.pop_alerts(&alerts);

			for (lt::alert const* a : alerts) {
				if (lt::alert_cast<lt::torrent_error_alert>(a)) {
					// TODO error
				}
			}

			if (tH.status().has_metadata && !adjustValues) {
				for (int i = 0; i < (int)pSurface->GameFileDiff.size(); i++) {
					for (int j = 0; j < tH.torrent_file().get()->files().num_files(); j++) {
						std::string filePath = tH.torrent_file().get()->files().file_path(j);
						filePath.replace(filePath.begin(), filePath.begin() + 49, "");
						if (filePath == pSurface->GameFileDiff[i].string()) {
							tH.file_priority(j, lt::download_priority_t(lt::default_priority));
						}
					}
				}
				adjustValues = true;
			}

			if (tH.status().is_finished) {
				ses.remove_torrent(tH);
				goto finished;
			}

			float progress = ceilf((tH.status().progress * 100) * 100) / 100;

			std::ostringstream out;
			out.precision(2);
			out << std::fixed << progress << "%";

			pSurface->logText(out.str());

			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}
	finished:
		MoveGameFiles(pSurface->FindForm());
	}
	catch (std::exception& e) {
		pSurface->logText(e.what());
	}
}

//-----------------------------------------------------------------------------
// Purpose: Moves recently installed files to there correct place
// Input  : *pSender - 
//-----------------------------------------------------------------------------
void CUIBaseSurface::MoveGameFiles(Forms::Control* pSender) {
	CUIBaseSurface* pSurface = reinterpret_cast<CUIBaseSurface*>(pSender->FindForm());

	for (int i = 0; i < (int)pSurface->GameFileDiff.size(); i++) {
		std::string startingAddition("R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM\\");
		startingAddition.append(pSurface->GameFileDiff[i].string());
		fs::rename(startingAddition, pSurface->GameFileDiff[i].string());
		pSurface->logText("Moved file: " + pSurface->GameFileDiff[i].filename().string());
	}

	fs::remove_all("R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM");

	for (auto& it : fs::directory_iterator(".")) {
		if (it.is_directory())
			continue;
		if (!it.path().has_extension())
			continue;
		if (it.path().extension() == ".parts")
			fs::remove(it.path());
	}
}

//-----------------------------------------------------------------------------
// Purpose: launches the game with the SDK
// Input  : *pSender - 
//-----------------------------------------------------------------------------
void CUIBaseSurface::LaunchGame(Forms::Control* pSender)
{
	CUIBaseSurface* pSurface = reinterpret_cast<CUIBaseSurface*>(pSender->FindForm());

	if (pSurface->bInstallGame) {
		std::thread threadR(InstallGame, pSender);
		threadR.detach();
	}
	else {

		fs::path cfgPath = fs::current_path() /= "platform\\cfg\\startup_launcher.cfg";

		ifstream cfgFile(cfgPath);
		string svParameter = "-launcher\n";

		if (cfgFile.good() && cfgFile)
		{
			stringstream ss;
			ss << cfgFile.rdbuf();
			svParameter.append(ss.str() + '\n');
		}
		else
			pSurface->m_LogList.push_back(LogList_t(spdlog::level::warn, "Unable to load 'startup_launcher.cfg'\n"));

		eLaunchMode launchMode = g_pLauncher->GetMainSurface()->BuildParameter(svParameter);

		if (g_pLauncher->Setup(launchMode, svParameter))
			g_pLauncher->Launch();
	}
}

//-----------------------------------------------------------------------------
// Purpose: parses all available maps from the main vpk directory
//-----------------------------------------------------------------------------
void CUIBaseSurface::ParseMaps()
{
	std::regex rgArchiveRegex{ R"([^_]*_(.*)(.bsp.pak000_dir).*)" };
	std::smatch smRegexMatches;
	if (fs::exists("vpk")) {
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
					if (!this->m_MapCombo->Items.Contains("mp_lobby"))
					{
						this->m_MapCombo->Items.Add("mp_lobby");
					}
					continue;
				}
				else if (!this->m_MapCombo->Items.Contains(smRegexMatches[1].str().c_str()))
				{
					this->m_MapCombo->Items.Add(smRegexMatches[1].str().c_str());
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: reads mod's folder
//-----------------------------------------------------------------------------
void CUIBaseSurface::ReadModJson() {
	// Removes compiled_mods folder when closed
	//atexit(compiledModsRemove);
	if (fs::exists("mods\\compiled_mods"))
		fs::remove_all("mods/\\compiled_mods");
	bool ran = false;
	logText(spdlog::level::level_enum::info, "Reading mod.json's.");
	if (fs::exists("mods")) {
		for (const auto& mEntry : fs::directory_iterator("mods")) {
			if (mEntry.path() == "mods\\compiled_mods")
				continue;
			std::string path = mEntry.path().string();
			std::string modJson = path + "\\mod.json";

			if (fs::exists(modJson)) {
				std::ifstream stream(modJson);
				if (stream.good()) {
					std::stringstream contents;
					contents << stream.rdbuf();

					//logText(spdlog::level::level_enum::info, contents.str());

					ModObject object = manager.addMod(contents.str(), path, this);

					//modObject object(contents.str(), modJson);

					//logText(spdlog::level::level_enum::info, objectToString(object)
				}
				else {
					logText(spdlog::level::level_enum::err, "Unable to read " + modJson);
				}
				stream.close();
			}
			else {
				logText(spdlog::level::level_enum::err, "Unable to find mod.json for mod: " + path);
			}
		}
		bool wereInvalid = false;
		logText(spdlog::level::level_enum::warn, "The following mods are invalid:");
		for (auto& invalid : manager.mods) {
			if (invalid.invalid) {
				logText(spdlog::level::level_enum::warn, invalid.name);
				wereInvalid = true;
			}
		}
		if (!wereInvalid)
			logText(spdlog::level::level_enum::warn, "Jk. It's fine");
	}

	//manager.moveEnabled();

	//this->LoadMods();

	/*ModList.push_back(modManager_t(m_modStatusLevel::invalidM, manager.modsList()[0]));
	ModsListView->SetVirtualListSize(static_cast<int32_t>(ModList.size()));
	ModsListView->Refresh();*/

	for (auto& mod : manager.modsList()) {
		if (mod.invalid)
			ModList.push_back(ModManager_t(invalidM, mod));
		else if (mod.toggled)
			ModList.push_back(ModManager_t(enabledM, mod));
		else
			ModList.push_back(ModManager_t(disabledM, mod));
		ModsListView->SetVirtualListSize(static_cast<int32_t>(ModList.size()));
		ModsListView->Refresh();
	}

	this->AdjustValues();

	//logText(manager.scriptRson);
}

//-----------------------------------------------------------------------------
// Refresh Mod Controls Text
//-----------------------------------------------------------------------------
void CUIBaseSurface::AdjustValues() {
	validMods = manager.modsListNum(ModType::valid);
	enabledMods = manager.modsListNum(ModType::enabled);

	this->m_ManagerControlsEnabledText->SetText(std::string("Enabled Mods: " + std::to_string(enabledMods)).c_str());
	this->m_ManagerControlsValidText->SetText(std::string("Valid Mods: " + std::to_string(validMods)).c_str());
}

//-----------------------------------------------------------------------------
// Purpose: log's to console. for debugging purpose's (at least mainly)
//-----------------------------------------------------------------------------
void CUIBaseSurface::logText(spdlog::level::level_enum color, std::string text) {
	std::string newText = text + "\n";
	m_LogList.push_back(LogList_t(color, newText.c_str()));
	m_ConsoleListView->SetVirtualListSize(static_cast<int32_t>(m_LogList.size()));
	//m_ConsoleListView->Refresh();
}

//-----------------------------------------------------------------------------
// Purpose: overwrite without type
//-----------------------------------------------------------------------------
void CUIBaseSurface::logText(std::string text) {
	std::string newText = text + "\n";
	m_LogList.push_back(LogList_t(spdlog::level::level_enum::info, newText.c_str()));
	m_ConsoleListView->SetVirtualListSize(static_cast<int32_t>(m_LogList.size()));
	m_ConsoleListView->Refresh();
}

//-----------------------------------------------------------------------------
// Purpose: parses all playlists from user selected playlist file
//-----------------------------------------------------------------------------
void CUIBaseSurface::ParsePlaylists()
{
	const string svBaseDir = "platform\\";
	fs::path fsPlaylistPath(svBaseDir + this->m_PlaylistFileTextBox->Text().ToCString());

	if (fs::exists(fsPlaylistPath))
	{
		bool bOk{ };
		std::ifstream iFile(fsPlaylistPath);
		vdf::object vRoot = vdf::read(iFile, &bOk);

		if (bOk)
		{
			const auto& vcPlaylists = vRoot.childs.at("Playlists");
			for (auto [id, it] = std::tuple{ 1, vcPlaylists->childs.begin() }; it != vcPlaylists->childs.end(); id++, it++)
			{
				if (!this->m_PlaylistCombo->Items.Contains(it->first.c_str()))
				{
					this->m_PlaylistCombo->Items.Add(it->first.c_str());
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: clears the form and reloads the playlist
// Input  : *pSender - 
//-----------------------------------------------------------------------------
void CUIBaseSurface::ReloadPlaylists(Forms::Control* pSender)
{
	CUIBaseSurface* pSurface = reinterpret_cast<CUIBaseSurface*>(pSender->FindForm());

	pSurface->m_PlaylistCombo->Items.Clear();
	pSurface->m_PlaylistCombo->OnSizeChanged();
	pSurface->ParsePlaylists();
}

//-----------------------------------------------------------------------------
// Purpose: copies selected virtual items to clipboard
// Input  : &pEventArgs - 
// Input  : *pSender - 
//-----------------------------------------------------------------------------
void CUIBaseSurface::VirtualItemToClipboard(const std::unique_ptr<MouseEventArgs>& pEventArgs, Forms::Control* pSender)
{
	if (pEventArgs->Button != Forms::MouseButtons::Right)
		return;

	CUIBaseSurface* pSurface = reinterpret_cast<CUIBaseSurface*>(pSender->FindForm());
	List<uint32_t> lSelected = pSurface->m_ConsoleListView->SelectedIndices();

	if (!lSelected.Count())
		return;

	string svClipBoard;
	for (uint32_t i = 0; i < lSelected.Count(); i++)
		svClipBoard.append(pSurface->m_LogList[lSelected[i]].m_svText);

	clip::set_text(svClipBoard);
}

//-----------------------------------------------------------------------------
// Purpose: changes currently showed mod
// Input  : &pEventArgs - 
//			*pSender - 
//-----------------------------------------------------------------------------
void CUIBaseSurface::ModManagerClick(const std::unique_ptr<MouseEventArgs>& pEventArgs, Forms::Control* pSender) {
	if (pEventArgs->Button != Forms::MouseButtons::Left)
		return;

	CUIBaseSurface* pSurface = reinterpret_cast<CUIBaseSurface*>(pSender->FindForm());

	List<uint32_t> lSelected = pSurface->ModsListView->SelectedIndices();

	if (!lSelected.Count())
		return;

	ModObject object;
	for (uint32_t i = 0; i < lSelected.Count(); i++)
		object = pSurface->ModList[lSelected[i]].m_object;

	pSurface->m_ManagerViewerCoverText->Hide();
	pSurface->m_ManagerViewerNameLabel->SetText(object.name.c_str());
	pSurface->m_ManagerViewerDescText->SetText(object.description.c_str());
	std::string authors = "Authors: ";
	for (int i = 0; i < (size_t)object.authors.size(); i++) {
		authors.append(object.authors[i]);
		if (i < object.authors.size() - 1)
			authors += ", ";
	}
	pSurface->m_ManagerViewerAuthorLabel->SetText(authors.c_str());
	pSurface->m_ManagerViewerAppidLabel->SetText(std::string("AppID: " + object.appid).c_str());
	std::string version = "Version: " + object.version;
	pSurface->m_ManagerViewerVersionLabel->SetText(version.c_str());
	pSurface->m_ManagerEnabledToggle->SetChecked(object.toggled);
	if (object.toggled)
		pSurface->m_ManagerEnabledLabel->SetText("Enabled");
	else
		pSurface->m_ManagerEnabledLabel->SetText("Disabled");
}

//-----------------------------------------------------------------------------
// Purpose: handles toggling mod state
// Input  : &pEventArgs - 
//			*pSender - 
//-----------------------------------------------------------------------------
void CUIBaseSurface::ModManagerEnabledToggle(Forms::Control* pSender) {
	CUIBaseSurface* pSurface = reinterpret_cast<CUIBaseSurface*>(pSender->FindForm());

	List<uint32_t> lSelected = pSurface->ModsListView->SelectedIndices();

	if (!lSelected.Count())
		return;

	ModObject object;
	for (uint32_t i = 0; i < lSelected.Count(); i++) {
		object = pSurface->ModList[lSelected[i]].m_object;
		//pSurface->ModList[lSelected[i]];
	}

	if (pSurface->m_ManagerEnabledToggle->Checked())
		object.toggled = true;
	else if (!pSurface->m_ManagerEnabledToggle->Checked())
		object.toggled = false;

	for (uint32_t i = 0; i < lSelected.Count(); i++) {
		if (object.toggled) {
			pSurface->m_ManagerEnabledLabel->SetText("Enabled");
			pSurface->ModList[lSelected[i]].m_object = object;
			pSurface->ModList[lSelected[i]].m_nLevel = enabledM;
		}
		else {
			pSurface->m_ManagerEnabledLabel->SetText("Disabled");
			pSurface->ModList[lSelected[i]].m_object = object;
			pSurface->ModList[lSelected[i]].m_nLevel = disabledM;
		}
	}

	pSurface->ModsListView->Refresh();

	std::vector<ModObject> vObjects = manager.mods;
	// Switched to an iterator because index wasn't needed
	for (auto& vObject : vObjects) {
		if (object.appid == vObject.appid)
			vObject = object;
	}

	/*for (int i = 0; i < vObjects.size(); i++) {
		if (object.appid == vObjects[i].appid)
			vObjects[i] = object;
	}*/
	manager.mods = vObjects;
	for (uint32_t i = 0; i < lSelected.Count(); i++)
		pSurface->ModList[lSelected[i]].m_object = object;

	object.updateJson();

	pSurface->AdjustValues();
}

//-----------------------------------------------------------------------------
// Purpose: gets and handles the virtual item
// Input  : &pEventArgs - 
//			*pSender - 
//-----------------------------------------------------------------------------
void CUIBaseSurface::GetVirtItemMod(const std::unique_ptr<Forms::RetrieveVirtualItemEventArgs>& pEventArgs, Forms::Control* pSender)
{
	CUIBaseSurface* pSurface = reinterpret_cast<CUIBaseSurface*>(pSender->FindForm());
	if (static_cast<int>(pSurface->ModList.size()) <= 0)
		return;

	pEventArgs->Style.ForeColor = Drawing::Color::White;
	pEventArgs->Style.BackColor = pSender->BackColor();
	pSurface->ModsListView->SetVirtualListSize(static_cast<int32_t>(pSurface->ModList.size()));

	static const Drawing::Color cColor[] =
	{
		Drawing::Color(92, 236, 89),   // Enabled
		Drawing::Color(236, 203, 0),   // Disabled
		Drawing::Color(236, 28, 0),    // Invalid
	};
	static const String svLevel[] =
	{
		"Enabled",
		"Disabled",
		"Invalid",
	};

	switch (pEventArgs->SubItemIndex)
	{
	case 0:
		pEventArgs->Style.ForeColor = cColor[pSurface->ModList[pEventArgs->ItemIndex].m_nLevel];
		pEventArgs->Text = svLevel[pSurface->ModList[pEventArgs->ItemIndex].m_nLevel];
		break;
	case 1:
		std::string text = "";
		text = pSurface->ModList[pEventArgs->ItemIndex].m_object.name + " By ";
		for (int i = 0; i < (size_t)pSurface->ModList[pEventArgs->ItemIndex].m_object.authors.size(); i++) {
			text += pSurface->ModList[pEventArgs->ItemIndex].m_object.authors[i];
			if (i < (size_t)pSurface->ModList[pEventArgs->ItemIndex].m_object.authors.size() - 1)
				text += ", ";
		}
		pEventArgs->Text = text;
		break;
	}
}

void CUIBaseSurface::GetVirtualItem(const std::unique_ptr<Forms::RetrieveVirtualItemEventArgs>& pEventArgs, Forms::Control* pSender)
{
	CUIBaseSurface* pSurface = reinterpret_cast<CUIBaseSurface*>(pSender->FindForm());
	if (static_cast<int>(pSurface->m_LogList.size()) <= 0)
		return;

	pEventArgs->Style.ForeColor = Drawing::Color::White;
	pEventArgs->Style.BackColor = pSender->BackColor();
	pSurface->m_ConsoleListView->SetVirtualListSize(static_cast<int32_t>(pSurface->m_LogList.size()));

	static const Drawing::Color cColor[] =
	{
		Drawing::Color(255, 255, 255), // Trace
		Drawing::Color(0, 120, 215),   // Debug
		Drawing::Color(92, 236, 89),   // Info
		Drawing::Color(236, 203, 0),   // Warn
		Drawing::Color(236, 28, 0),    // Error
		Drawing::Color(236, 28, 0),    // Critical
		Drawing::Color(255, 255, 255), // General
	};
	static const String svLevel[] =
	{
		"trace",
		"debug",
		"info",
		"warning",
		"error",
		"critical",
		"general",
	};

	switch (pEventArgs->SubItemIndex)
	{
	case 0:
		pEventArgs->Style.ForeColor = cColor[pSurface->m_LogList[pEventArgs->ItemIndex].m_nLevel];
		pEventArgs->Text = svLevel[pSurface->m_LogList[pEventArgs->ItemIndex].m_nLevel];
		break;
	case 1:
		pEventArgs->Text = pSurface->m_LogList[pEventArgs->ItemIndex].m_svText;
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: forward input command to the game
// Input  : *pSender - 
//-----------------------------------------------------------------------------
void CUIBaseSurface::ForwardCommandToGame(Forms::Control* pSender)
{
	CUIBaseSurface* pSurface = reinterpret_cast<CUIBaseSurface*>(pSender->FindForm());

	const HWND hWindow = FindWindowA("Respawn001", NULL);
	if (hWindow)
	{
		String kzCommand = pSurface->m_ConsoleCommandTextBox->Text();
		const char* szCommand = kzCommand.ToCString();
		COPYDATASTRUCT cData = { 0, strnlen_s(szCommand, 259) + 1, (void*)szCommand };

		bool bProcessingMessage = SendMessageA(hWindow, WM_COPYDATA, NULL, (LPARAM)&cData); // WM_COPYDATA will only return 0 or 1, that's why we use a boolean.
		if (bProcessingMessage)
		{
			pSurface->m_ConsoleCommandTextBox->SetText("");
			pSurface->m_LogList.push_back(LogList_t((spdlog::level::level_enum)2, kzCommand));
			pSurface->m_ConsoleListView->SetVirtualListSize(static_cast<int32_t>(pSurface->m_LogList.size()));
			pSurface->m_ConsoleListView->Refresh();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: clears the form and reloads the playlist
// Input  : &svParameters - 
// Output : eLaunchMode [HOST - SERVER - CLIENT - NONE]
//-----------------------------------------------------------------------------
eLaunchMode CUIBaseSurface::BuildParameter(string& svParameters)
{
	eLaunchMode results = eLaunchMode::LM_NONE;

	switch (static_cast<eMode>(this->m_ModeCombo->SelectedIndex()))
	{
	case eMode::HOST:
	{
		// GAME ###############################################################
		if (!String::IsNullOrEmpty(this->m_MapCombo->Text()))
		{
			svParameters.append("+map \"" + this->m_MapCombo->Text() + "\"\n");
		}
		if (!String::IsNullOrEmpty(this->m_PlaylistCombo->Text()))
		{
			svParameters.append("+launchplaylist \"" + this->m_PlaylistCombo->Text() + "\"\n");
		}
		if (this->m_DevelopmentToggle->Checked())
		{
			svParameters.append("-devsdk\n");
			results = eLaunchMode::LM_HOST_DEBUG;
		}
		else
			results = eLaunchMode::LM_HOST;

		if (this->m_CheatsToggle->Checked())
		{
			svParameters.append("+sv_cheats \"1\"\n");
			svParameters.append("-showdevmenu\n");
		}



		if (this->m_ConsoleToggle->Checked())
			svParameters.append("-wconsole\n");

		if (this->m_ColorConsoleToggle->Checked())
			svParameters.append("-ansiclr\n");

		if (!String::IsNullOrEmpty(this->m_PlaylistFileTextBox->Text()))
			svParameters.append("-playlistfile \"" + this->m_PlaylistFileTextBox->Text() + "\"\n");

		// ENGINE ###############################################################
		if (StringIsDigit(this->m_ReservedCoresTextBox->Text().ToCString()))
			svParameters.append("-numreservedcores \"" + this->m_ReservedCoresTextBox->Text() + "\"\n");
		//else error;

		if (StringIsDigit(this->m_WorkerThreadsTextBox->Text().ToCString()))
			svParameters.append("-numworkerthreads \"" + this->m_WorkerThreadsTextBox->Text() + "\"\n");
		//else error;

		if (this->m_SingleCoreDediToggle->Checked())
			svParameters.append("+sv_single_core_dedi \"1\"\n");

		if (this->m_NoAsyncJobsToggle->Checked())
		{
			svParameters.append("-noasync\n");
			svParameters.append("+async_serialize \"0\"\n");
			svParameters.append("+buildcubemaps_async \"0\"\n");
			svParameters.append("+sv_asyncAIInit \"0\"\n");
			svParameters.append("+sv_asyncSendSnapshot \"0\"\n");
			svParameters.append("+sv_scriptCompileAsync \"0\"\n");
			svParameters.append("+cl_scriptCompileAsync \"0\"\n");
			svParameters.append("+cl_async_bone_setup \"0\"\n");
			svParameters.append("+cl_updatedirty_async \"0\"\n");
			svParameters.append("+mat_syncGPU \"1\"\n");
			svParameters.append("+mat_sync_rt \"1\"\n");
			svParameters.append("+mat_sync_rt_flushes_gpu \"1\"\n");
			svParameters.append("+net_async_sendto \"0\"\n");
			svParameters.append("+physics_async_sv \"0\"\n");
			svParameters.append("+physics_async_cl \"0\"\n");
		}

		if (this->m_NetEncryptionToggle->Checked())
			svParameters.append("+net_encryptionEnable \"1\"\n");

		if (this->m_NetRandomKeyToggle->Checked())
			svParameters.append("+net_useRandomKey \"1\"\n");

		if (this->m_NoQueuedPacketThread->Checked())
			svParameters.append("+net_queued_packet_thread \"0\"\n");

		if (this->m_NoTimeOutToggle->Checked())
			svParameters.append("-notimeout\n");

		if (this->m_WindowedToggle->Checked())
			svParameters.append("-windowed\n");

		if (this->m_NoBorderToggle->Checked())
			svParameters.append("-noborder\n");

		if (StringIsDigit(this->m_FpsTextBox->Text().ToCString()))
			svParameters.append("+fps_max \"" + this->m_FpsTextBox->Text() + "\"\n");

		if (!String::IsNullOrEmpty(this->m_WidthTextBox->Text()))
			svParameters.append("-w \"" + this->m_WidthTextBox->Text() + "\"\n");

		if (!String::IsNullOrEmpty(this->m_HeightTextBox->Text()))
			svParameters.append("-h \"" + this->m_HeightTextBox->Text() + "\"\n");

		// MAIN ###############################################################
		if (!String::IsNullOrEmpty(this->m_HostNameTextBox->Text()))
		{
			svParameters.append("+hostname \"" + this->m_HostNameTextBox->Text() + "\"\n");

			switch (static_cast<eVisibility>(this->m_VisibilityCombo->SelectedIndex()))
			{
			case eVisibility::PUBLIC:
			{
				svParameters.append("+sv_pylonVisibility \"2\"\n");
				break;
			}
			case eVisibility::HIDDEN:
			{
				svParameters.append("+sv_pylonVisibility \"1\"\n");
				break;
			}
			default:
			{
				svParameters.append("+sv_pylonVisibility \"0\"\n");
				break;
			}
			}
		}
		if (!String::IsNullOrEmpty(this->m_LaunchArgsTextBox->Text()))
			svParameters.append(this->m_LaunchArgsTextBox->Text());

		if (this->m_ManagerUseModsToggle->Checked()) {
			svParameters.append("-modded ");
			manager.moveEnabled();
		}

		svParameters.append("-fnf ");

		return results;
	}
	case eMode::SERVER:
	{
		// GAME ###############################################################
		if (!String::IsNullOrEmpty(this->m_MapCombo->Text()))
		{
			svParameters.append("+map \"" + this->m_MapCombo->Text() + "\"\n");
		}
		if (!String::IsNullOrEmpty(this->m_PlaylistCombo->Text()))
		{
			svParameters.append("+launchplaylist \"" + this->m_PlaylistCombo->Text() + "\"\n");
		}
		if (this->m_DevelopmentToggle->Checked())
		{
			svParameters.append("-devsdk\n");
			results = eLaunchMode::LM_SERVER_DEBUG;
		}
		else
			results = eLaunchMode::LM_SERVER;

		if (this->m_CheatsToggle->Checked())
			svParameters.append("+sv_cheats \"1\"\n");

		if (this->m_ConsoleToggle->Checked())
			svParameters.append("-wconsole\n");

		if (this->m_ColorConsoleToggle->Checked())
			svParameters.append("-ansiclr\n");

		if (!String::IsNullOrEmpty(this->m_PlaylistFileTextBox->Text()))
			svParameters.append("-playlistfile \"" + this->m_PlaylistFileTextBox->Text() + "\"\n");

		// ENGINE ###############################################################
		if (StringIsDigit(this->m_ReservedCoresTextBox->Text().ToCString()))
			svParameters.append("-numreservedcores \"" + this->m_ReservedCoresTextBox->Text() + "\"\n");
		//else error;

		if (StringIsDigit(this->m_WorkerThreadsTextBox->Text().ToCString()))
			svParameters.append("-numworkerthreads \"" + this->m_WorkerThreadsTextBox->Text() + "\"\n");
		//else error;

		if (this->m_SingleCoreDediToggle->Checked())
			svParameters.append("+sv_single_core_dedi \"1\"\n");

		if (this->m_NoAsyncJobsToggle->Checked())
		{
			svParameters.append("-noasync\n");
			svParameters.append("+async_serialize \"0\"\n");
			svParameters.append("+sv_asyncAIInit \"0\"\n");
			svParameters.append("+sv_asyncSendSnapshot \"0\"\n");
			svParameters.append("+sv_scriptCompileAsync \"0\"\n");
			svParameters.append("+physics_async_sv \"0\"\n");
		}

		if (this->m_NetEncryptionToggle->Checked())
			svParameters.append("+net_encryptionEnable \"1\"\n");

		if (this->m_NetRandomKeyToggle->Checked())
			svParameters.append("+net_useRandomKey \"1\"\n");

		if (this->m_NoQueuedPacketThread->Checked())
			svParameters.append("+net_queued_packet_thread \"0\"\n");

		if (this->m_NoTimeOutToggle->Checked())
			svParameters.append("-notimeout\n");

		// MAIN ###############################################################
		if (!String::IsNullOrEmpty(this->m_HostNameTextBox->Text()))
		{
			svParameters.append("+hostname \"" + this->m_HostNameTextBox->Text() + "\"\n");

			switch (static_cast<eVisibility>(this->m_VisibilityCombo->SelectedIndex()))
			{
			case eVisibility::PUBLIC:
			{
				svParameters.append("+sv_pylonVisibility \"2\"\n");
				break;
			}
			case eVisibility::HIDDEN:
			{
				svParameters.append("+sv_pylonVisibility \"1\"\n");
				break;
			}
			default:
			{
				svParameters.append("+sv_pylonVisibility \"0\"\n");
				break;
			}
			}
		}
		if (!String::IsNullOrEmpty(this->m_LaunchArgsTextBox->Text()))
			svParameters.append(this->m_LaunchArgsTextBox->Text());

		return results;
	}
	case eMode::CLIENT:
	{
		// GAME ###############################################################
		if (this->m_DevelopmentToggle->Checked())
		{
			svParameters.append("-devsdk\n");
			results = eLaunchMode::LM_CLIENT_DEBUG;
		}
		else
			results = eLaunchMode::LM_CLIENT;

		if (this->m_CheatsToggle->Checked())
		{
			svParameters.append("+sv_cheats \"1\"\n");
			svParameters.append("-showdevmenu\n");
		}

		if (this->m_ConsoleToggle->Checked())
			svParameters.append("-wconsole\n");

		if (this->m_ColorConsoleToggle->Checked())
			svParameters.append("-ansiclr\n");

		if (!String::IsNullOrEmpty(this->m_PlaylistFileTextBox->Text()))
			svParameters.append("-playlistfile \"" + this->m_PlaylistFileTextBox->Text() + "\"\n");

		// ENGINE ###############################################################
		if (StringIsDigit(this->m_ReservedCoresTextBox->Text().ToCString()))
			svParameters.append("-numreservedcores \"" + this->m_ReservedCoresTextBox->Text() + "\"\n");
		//else error;

		if (StringIsDigit(this->m_WorkerThreadsTextBox->Text().ToCString()))
			svParameters.append("-numworkerthreads \"" + this->m_WorkerThreadsTextBox->Text() + "\"\n");
		//else error;

		if (this->m_SingleCoreDediToggle->Checked())
			svParameters.append("+sv_single_core_dedi \"1\"\n");

		if (this->m_NoAsyncJobsToggle->Checked())
		{
			svParameters.append("-noasync\n");
			svParameters.append("+async_serialize \"0\"\n");
			svParameters.append("+buildcubemaps_async \"0\"\n");
			svParameters.append("+cl_scriptCompileAsync \"0\"\n");
			svParameters.append("+cl_async_bone_setup \"0\"\n");
			svParameters.append("+cl_updatedirty_async \"0\"\n");
			svParameters.append("+mat_syncGPU \"1\"\n");
			svParameters.append("+mat_sync_rt \"1\"\n");
			svParameters.append("+mat_sync_rt_flushes_gpu \"1\"\n");
			svParameters.append("+net_async_sendto \"0\"\n");
			svParameters.append("+physics_async_cl \"0\"\n");
		}

		if (this->m_NetEncryptionToggle->Checked())
			svParameters.append("+net_encryptionEnable \"1\"\n");

		if (this->m_NetRandomKeyToggle->Checked())
			svParameters.append("+net_useRandomKey \"1\"\n");

		if (this->m_NoQueuedPacketThread->Checked())
			svParameters.append("+net_queued_packet_thread \"0\"\n");

		if (this->m_NoTimeOutToggle->Checked())
			svParameters.append("-notimeout\n");

		if (this->m_WindowedToggle->Checked())
			svParameters.append("-windowed\n");

		if (this->m_NoBorderToggle->Checked())
			svParameters.append("-noborder\n");

		if (StringIsDigit(this->m_FpsTextBox->Text().ToCString()))
			svParameters.append("+fps_max \"" + this->m_FpsTextBox->Text() + "\"\n");

		if (!String::IsNullOrEmpty(this->m_WidthTextBox->Text()))
			svParameters.append("-w \"" + this->m_WidthTextBox->Text() + "\"\n");

		if (!String::IsNullOrEmpty(this->m_HeightTextBox->Text()))
			svParameters.append("-h \"" + this->m_HeightTextBox->Text() + "\"\n");

		// MAIN ###############################################################
		if (!String::IsNullOrEmpty(this->m_LaunchArgsTextBox->Text()))
			svParameters.append(this->m_LaunchArgsTextBox->Text());

		return results;
	}
	default:
		return results;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CUIBaseSurface::CUIBaseSurface() : Forms::Form()
{
	this->Init();
	this->Setup();
}
CUIBaseSurface* g_pMainUI;