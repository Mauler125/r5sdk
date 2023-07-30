#include "pch.h"
#include "download_surface.h"

CProgressPanel::CProgressPanel()
	: Forms::Form(), m_bCanClose(false), m_bCanceled(false), m_bAutoClose(false)
{
	this->InitializeComponent();
}

void CProgressPanel::UpdateProgress(uint32_t Progress, bool Finished)
{
	this->m_ProgressBar->SetValue(Progress);

	if (Finished)
	{
		this->m_FinishButton->SetEnabled(true);
		this->m_CancelButton->SetEnabled(false);
		this->m_bCanClose = true;
		this->SetText("Download Complete");

		if (this->m_bCanceled || this->m_bAutoClose)
			this->Close();
	}
}

bool CProgressPanel::IsCanceled()
{
	return this->m_bCanceled;
}

void CProgressPanel::SetCanCancel(bool bEnable)
{
	m_CancelButton->SetEnabled(bEnable);
}

void CProgressPanel::SetAutoClose(bool Value)
{
	this->m_bAutoClose = Value;
}

void CProgressPanel::SetExportLabel(const char* pNewLabel)
{
	m_DownloadLabel->SetText(pNewLabel);
}

void CProgressPanel::InitializeComponent()
{
	this->SuspendLayout();
	this->SetAutoScaleDimensions({ 6, 13 });
	this->SetAutoScaleMode(Forms::AutoScaleMode::Font);
	this->SetText("Downloading Files...");
	this->SetClientSize({ 409, 119 });
	this->SetFormBorderStyle(Forms::FormBorderStyle::FixedSingle);
	this->SetStartPosition(Forms::FormStartPosition::CenterParent);
	this->SetMinimizeBox(false);
	this->SetMaximizeBox(false);
	this->SetControlBox(false);
	this->SetShowInTaskbar(false);
	this->SetBackColor(Drawing::Color(47, 54, 61));

	this->m_DownloadLabel = new UIX::UIXLabel();
	this->m_DownloadLabel->SetSize({ 385, 17 });
	this->m_DownloadLabel->SetLocation({ 12, 18 });
	this->m_DownloadLabel->SetTabIndex(3);
	this->m_DownloadLabel->SetText("Progress:");
	this->m_DownloadLabel->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left | Forms::AnchorStyles::Right);
	this->m_DownloadLabel->SetTextAlign(Drawing::ContentAlignment::TopLeft);
	this->AddControl(this->m_DownloadLabel);

	this->m_CancelButton = new UIX::UIXButton();
	this->m_CancelButton->SetSize({ 87, 31 });
	this->m_CancelButton->SetLocation({ 310, 76 });
	this->m_CancelButton->SetTabIndex(2);
	this->m_CancelButton->SetText("Cancel");
	this->m_CancelButton->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Right);
	this->m_CancelButton->Click += &OnCancelClick;
	this->AddControl(this->m_CancelButton);

	this->m_FinishButton = new UIX::UIXButton();
	this->m_FinishButton->SetSize({ 87, 31 });
	this->m_FinishButton->SetLocation({ 217, 76 });
	this->m_FinishButton->SetTabIndex(1);
	this->m_FinishButton->SetText("Ok");
	this->m_FinishButton->SetEnabled(false);
	this->m_FinishButton->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Right);
	this->m_FinishButton->Click += &OnFinishClick;
	this->AddControl(this->m_FinishButton);

	this->m_ProgressBar = new UIX::UIXProgressBar();
	this->m_ProgressBar->SetSize({ 385, 29 });
	this->m_ProgressBar->SetLocation({ 12, 38 });
	this->m_ProgressBar->SetTabIndex(0);
	this->m_ProgressBar->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left | Forms::AnchorStyles::Right);
	this->AddControl(this->m_ProgressBar);

	this->ResumeLayout(false);
	this->PerformLayout();
	// END DESIGNER CODE
}

void CProgressPanel::OnFinishClick(Forms::Control* Sender)
{
	((Forms::Form*)Sender->FindForm())->Close();
}

void CProgressPanel::OnCancelClick(Forms::Control* Sender)
{
	((CProgressPanel*)Sender->FindForm())->CancelProgress();
}

void CProgressPanel::CancelProgress()
{
	this->m_CancelButton->SetEnabled(false);
	this->m_CancelButton->SetText("Canceling...");
	this->m_bCanceled = true;
}
