#pragma once
#include "advanced_surface.h"

class CBaseSurface : public Forms::Form
{
public:
	CBaseSurface();
	virtual ~CBaseSurface()
	{
	};

protected:
	static void OnAdvancedClick(Forms::Control* Sender);


	static void OnSupportClick(Forms::Control* Sender);
	static void OnJoinClick(Forms::Control* Sender);

private:

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

	// Game.
	UIX::UIXGroupBox* m_BaseGroup;

	UIX::UIXButton* m_ManageButton;
	UIX::UIXButton* m_RepairButton;

	UIX::UIXButton* m_DonateButton;
	UIX::UIXButton* m_JoinButton;
	UIX::UIXButton* m_AdvancedButton;
};
