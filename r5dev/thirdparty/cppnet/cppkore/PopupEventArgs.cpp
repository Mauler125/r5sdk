#include "stdafx.h"
#include "PopupEventArgs.h"

namespace Forms
{
	PopupEventArgs::PopupEventArgs(Control* Window, Control* Ctrl, bool IsBalloon, Drawing::Size Size)
		: CancelEventArgs(), AssociatedWindow(Window), AssociatedControl(Ctrl), IsBalloon(IsBalloon), ToolTipSize(Size)
	{
	}
}
