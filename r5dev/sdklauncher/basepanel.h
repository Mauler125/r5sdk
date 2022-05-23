#pragma once

#include "thirdparty/cppnet/cppkore/Kore.h"
#include "thirdparty/cppnet/cppkore/UIXTheme.h"
#include "thirdparty/cppnet/cppkore/UIXLabel.h"
#include "thirdparty/cppnet/cppkore/UIXListView.h"
#include "thirdparty/cppnet/cppkore/UIXCheckBox.h"
#include "thirdparty/cppnet/cppkore/UIXComboBox.h"
#include "thirdparty/cppnet/cppkore/UIXTextBox.h"
#include "thirdparty/cppnet/cppkore/UIXGroupBox.h"
#include "thirdparty/cppnet/cppkore/UIXButton.h"
#include "thirdparty/cppnet/cppkore/UIXRadioButton.h"
#include "thirdparty/cppnet/cppkore/KoreTheme.h"

class CUIBasePanel : public Forms::Form
{
public:
    CUIBasePanel();
    virtual ~CUIBasePanel() = default;

private:
    void Init();

	UIX::UIXTextBox* m_HeightTextBox;
	UIX::UIXTextBox* m_WidthTextBox;
	UIX::UIXTextBox* m_WorkerThreadsTextBox;
	UIX::UIXTextBox* m_ReservedCoresTextBox;
	UIX::UIXTextBox* m_FpsTextBox;
	UIX::UIXTextBox* m_PlaylistFileTextBox;
	UIX::UIXTextBox* m_CustomDllTextBox;
	UIX::UIXTextBox* m_LaunchArgsTextBox;
	// Labels
	UIX::UIXLabel* m_WorkerThreadsLabel;
	UIX::UIXLabel* m_ReservedCoresLabel;
	UIX::UIXLabel* m_MapLabel;
	UIX::UIXLabel* m_PlaylistLabel;
	UIX::UIXLabel* m_ModeLabel;
	UIX::UIXLabel* m_FpsLabel;
	UIX::UIXLabel* m_ResolutionLabel;
	UIX::UIXLabel* m_PlaylistFileLabel;
	UIX::UIXLabel* m_CustomDllLabel;
	UIX::UIXLabel* m_LaunchArgsLabel;
	// Boxes
	UIX::UIXGroupBox* m_GameGroup;
	UIX::UIXGroupBox* m_MainGroup;
	UIX::UIXGroupBox* m_GameGroupExt;
	UIX::UIXGroupBox* m_MainGroupExt;
	UIX::UIXGroupBox* m_ConsoleGroup;
	UIX::UIXGroupBox* m_EngineBaseGroup;
	UIX::UIXGroupBox* m_EngineNetworkGroup;
	UIX::UIXGroupBox* m_EngineVideoGroup;
	// Toggles
	UIX::UIXCheckBox* m_CheatsToggle;
	UIX::UIXCheckBox* m_DevelopmentToggle;
	UIX::UIXCheckBox* m_ConsoleToggle;
	UIX::UIXCheckBox* m_WindowedToggle;
	UIX::UIXCheckBox* m_BorderlessToggle;
	UIX::UIXCheckBox* m_SingleCoreDediToggle;
	UIX::UIXCheckBox* m_NoAsyncJobsToggle;
	UIX::UIXCheckBox* m_NetEncryptionToggle;
	UIX::UIXCheckBox* m_NetRandomKeyToggle;
	UIX::UIXCheckBox* m_QueuedPacketThread;
	UIX::UIXCheckBox* m_NoTimeOut;
	UIX::UIXCheckBox* m_ColorConsoleToggle;
	// Combo
	UIX::UIXComboBox* m_MapCombo;
	UIX::UIXComboBox* m_PlaylistCombo;
	UIX::UIXComboBox* m_ModeCombo;
	// Buttons
	UIX::UIXButton* m_LaunchGame;
	UIX::UIXButton* m_CleanSDK;
	UIX::UIXButton* m_UpdateSDK;
	UIX::UIXButton* m_LaunchSDK;

	UIX::UIXListView* m_ConsoleListView;
};

extern CUIBasePanel* g_pMainUI;
