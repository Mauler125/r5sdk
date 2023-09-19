#pragma once

#include "ComboBox.h"

using namespace Forms;

namespace UIX
{
	// Represents a UIX themed combo box control.
	class UIXComboBox : public ComboBox
	{
	public:
		UIXComboBox();

		// Override the paint event to provide our custom combo box control.
		virtual void OnPaint(const std::unique_ptr<PaintEventArgs>& EventArgs);
		virtual void OnMouseEnter();
		virtual void OnMouseLeave();

	private:
		// Internal cached flags
		bool _MouseOver;
	};
}