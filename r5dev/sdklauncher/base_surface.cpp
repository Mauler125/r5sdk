//=============================================================================//
//
// Purpose: Launcher user interface implementation.
//
//=============================================================================//
#include "base_surface.h"
#include "sdklauncher.h"
#include "advanced_surface.h"
#include "download_surface.h"
#include "sdklauncher_utils.h"
#include "tier1/xorstr.h"
#include "tier1/utlmap.h"
#include "tier2/curlutils.h"
#include "zip/src/ZipFile.h"

#define WINDOW_SIZE_X 400
#define WINDOW_SIZE_Y 224

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

	m_bIsInstalled = fs::exists("r5apex.exe");

	this->m_ManageButton = new UIX::UIXButton();
	this->m_ManageButton->SetSize({ 168, 70 });
	this->m_ManageButton->SetLocation({ 10, 10 });
	this->m_ManageButton->SetTabIndex(9);
	this->m_ManageButton->SetText(m_bIsInstalled ? XorStr("Launch Apex") : XorStr("Install Apex"));
	this->m_ManageButton->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left);
	this->m_ManageButton->Click += m_bIsInstalled ? &OnLaunchClick : &OnInstallClick;
	m_BaseGroup->AddControl(this->m_ManageButton);

	this->m_RepairButton = new UIX::UIXButton();
	this->m_RepairButton->SetSize({ 168, 70 });
	this->m_RepairButton->SetLocation({ 10, 90 });
	this->m_RepairButton->SetTabIndex(9);
	this->m_RepairButton->SetEnabled(/*m_bIsInstalled*/ false);
	this->m_RepairButton->SetText(XorStr("Repair Apex"));
	this->m_RepairButton->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left);
	// TODO: should hash every file against a downloaded manifest instead and
	// start repairing what mismatches.
	this->m_RepairButton->Click += &OnInstallClick;
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
	this->m_AdvancedButton->SetEnabled(m_bIsInstalled);
	this->m_AdvancedButton->SetText(XorStr("Advanced Options"));
	this->m_AdvancedButton->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left);
	this->m_AdvancedButton->Click += &OnAdvancedClick;
	m_BaseGroup->AddControl(this->m_AdvancedButton);

	m_ExperimentalBuildsCheckbox = new UIX::UIXCheckBox();
	this->m_ExperimentalBuildsCheckbox->SetSize({ 250, 18 });
	this->m_ExperimentalBuildsCheckbox->SetLocation({ 10, 170 });
	this->m_ExperimentalBuildsCheckbox->SetTabIndex(0);
	this->m_ExperimentalBuildsCheckbox->SetText(XorStr("Check for playtest/event updates"));
	this->m_ExperimentalBuildsCheckbox->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);

	// TODO: remove this when its made public!!!
	m_ExperimentalBuildsCheckbox->SetChecked(true);

	m_BaseGroup->AddControl(this->m_ExperimentalBuildsCheckbox);

	// TODO: Use a toggle item instead; remove this field.
	m_bPartialInstall = false;
	m_bUpdateViewToggled = false;

	Threading::Thread([this] {
		this->Frame();
	}).Start();
}

void CBaseSurface::ToggleUpdateView(bool bValue)
{
	// Game must be installed before this can be called!!
	Assert(m_bIsInstalled);

	this->m_ManageButton->SetText(bValue ? XorStr("Update Apex") : XorStr("Install Apex"));

	if (bValue)
	{
		this->m_ManageButton->Click -= &OnLaunchClick;
		this->m_ManageButton->Click += &OnUpdateClick;
	}
	else
	{
		this->m_ManageButton->Click -= &OnUpdateClick;
		this->m_ManageButton->Click += &OnLaunchClick;
	}

	m_bUpdateViewToggled = bValue;
}


void CBaseSurface::Frame()
{
	for (;;)
	{
		printf("%s: runframe; interval=%f\n", __FUNCTION__, g_flUpdateCheckRate);

		if (!m_bUpdateViewToggled && m_bIsInstalled && SDKLauncher_CheckForUpdate(m_ExperimentalBuildsCheckbox->Checked()))
		{
			ToggleUpdateView(true);
			printf("%s: found update; interval=%f\n", __FUNCTION__, g_flUpdateCheckRate);
		}

		std::this_thread::sleep_for(IntervalToDuration(g_flUpdateCheckRate));
	}
}

void CBaseSurface::OnUpdateClick(Forms::Control* Sender)
{
	//CBaseSurface* pSurf = (CBaseSurface*)Sender;

	vector<HWND> vecHandles;
	EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&vecHandles));

	if (!vecHandles.empty())
	{
		Forms::MessageBox::Show("Close all game instances before updating the game!\n",
			"Warning", Forms::MessageBoxButtons::OK, Forms::MessageBoxIcon::Warning);

		return;
	}


	auto downloadSurface = std::make_unique<CProgressPanel>();
	CProgressPanel* pProgress = downloadSurface.get();

	pProgress->SetAutoClose(true);

	Threading::Thread([pProgress] {

		if (!SDKLauncher_CreateDepotDirectories())
		{
			Forms::MessageBox::Show(Format("Failed to create depot directories: Error code = %08x\n", GetLastError()).c_str(),
				"Error", Forms::MessageBoxButtons::OK, Forms::MessageBoxIcon::Error);

			return;
		}

		CUtlVector<CUtlString> fileList;
		if (SDKLauncher_BeginInstall(true, false, fileList, pProgress))
		{
		}

		// Close on finish.
		pProgress->Close();
		}).Start();

	pProgress->ShowDialog();

	// Restart the launcher process from here through updater.exe!
	SDKLauncher_Restart();
}

void CBaseSurface::OnInstallClick(Forms::Control* Sender)
{
	vector<HWND> vecHandles;
	EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&vecHandles));

	if (!vecHandles.empty())
	{
		Forms::MessageBox::Show("Close all game instances before installing the game!\n",
			"Warning", Forms::MessageBoxButtons::OK, Forms::MessageBoxIcon::Warning);

		return;
	}


	//CBaseSurface* pSurf = (CBaseSurface*)Sender;
	//const bool bPartial = pSurf->m_bPartialInstall;

	//const int minRequiredSpace = bPartial ? MIN_REQUIRED_DISK_SPACE : MIN_REQUIRED_DISK_SPACE_OPT;
	//int currentDiskSpace;

	//if (!SDKLauncher_CheckDiskSpace(minRequiredSpace, &currentDiskSpace))
	//{
	//	Forms::MessageBox::Show(Format("There is not enough space available on the disk to install R5Reloaded;"
	//		" you need at least %iGB, you currently have %iGB\n", minRequiredSpace, currentDiskSpace).c_str(),
	//		"Error", Forms::MessageBoxButtons::OK, Forms::MessageBoxIcon::Error);

	//	return;
	//}

	auto downloadSurface = std::make_unique<CProgressPanel>();
	CProgressPanel* pProgress = downloadSurface.get();

	pProgress->SetAutoClose(true);

	Threading::Thread([pProgress] {
		
		if (!SDKLauncher_CreateDepotDirectories())
		{
			Forms::MessageBox::Show(Format("Failed to create depot directories: Error code = %08x\n", GetLastError()).c_str(),
				"Error", Forms::MessageBoxButtons::OK, Forms::MessageBoxIcon::Error);

			return;
		}

		CUtlVector<CUtlString> fileList;
		if (SDKLauncher_BeginInstall(true, false, fileList, pProgress))
		{
		}

		// Close on finish.
		pProgress->Close();
	}).Start();

	pProgress->ShowDialog();

	// Restart the launcher process from here through updater.exe!
	SDKLauncher_Restart();
}


void CBaseSurface::OnLaunchClick(Forms::Control* Sender)
{
	// !TODO: parameter building and settings loading should be its own class!!
	if (g_pLauncher->CreateLaunchContext(eLaunchMode::LM_CLIENT, 0, "", "startup_launcher.cfg"))
		g_pLauncher->LaunchProcess();
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
