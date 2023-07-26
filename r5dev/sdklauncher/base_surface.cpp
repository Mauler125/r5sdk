//=============================================================================//
//
// Purpose: Launcher user interface implementation.
//
//=============================================================================//
#include "base_surface.h"
#include "advanced_surface.h"
#include "tier1/xorstr.h"

CBaseSurface::CBaseSurface()
{
	const INT WindowX = 400;
	const INT WindowY = 194;

	this->SuspendLayout();
	this->SetAutoScaleDimensions({ 6, 13 });
	this->SetAutoScaleMode(Forms::AutoScaleMode::Font);
	this->SetText(XorStr("R5Reloaded"));
	this->SetClientSize({ WindowX, WindowY });
	this->SetFormBorderStyle(Forms::FormBorderStyle::FixedSingle);
	this->SetStartPosition(Forms::FormStartPosition::CenterScreen);
	this->SetMinimizeBox(true);
	this->SetMaximizeBox(false);
	this->SetBackColor(Drawing::Color(47, 54, 61));

	const INT BASE_GROUP_OFFSET = 12;

	this->m_BaseGroup = new UIX::UIXGroupBox();
	this->m_BaseGroup->SetSize({ WindowX-(BASE_GROUP_OFFSET*2), WindowY-(BASE_GROUP_OFFSET*2) });
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
	this->m_ManageButton->Click += isInstalled ? &OnAdvancedClick : &OnAdvancedClick;
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
}

void CBaseSurface::OnAdvancedClick(Forms::Control* Sender)
{
	auto pAdvancedSurface = std::make_unique<CAdvancedSurface>();
	pAdvancedSurface->ShowDialog((Forms::Form*)Sender->FindForm());
}

void CBaseSurface::OnSupportClick(Forms::Control* /*Sender*/)
{
	//auto pAdvancedSurface = std::make_unique<CAdvancedSurface>();
	//pAdvancedSurface->ShowDialog((Forms::Form*)Sender->FindForm());

	ShellExecute(0, 0, XorStr("https://www.paypal.com/donate/?hosted_button_id=S28DHC2TF6UV4"), 0, 0, SW_SHOW);
}

void CBaseSurface::OnJoinClick(Forms::Control* /*Sender*/)
{
	//auto pAdvancedSurface = std::make_unique<CAdvancedSurface>();
	//pAdvancedSurface->ShowDialog((Forms::Form*)Sender->FindForm());

	ShellExecute(0, 0, XorStr("https://discord.com/invite/jqMkUdXrBr"), 0, 0, SW_SHOW);
}
