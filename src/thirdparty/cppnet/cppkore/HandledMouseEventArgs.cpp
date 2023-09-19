#include "stdafx.h"
#include "HandledMouseEventArgs.h"

namespace Forms
{
	HandledMouseEventArgs::HandledMouseEventArgs(MouseButtons Button, uint32_t Clicks, int32_t X, int32_t Y, int32_t Delta)
		: MouseEventArgs(Button, Clicks, X, Y, Delta), Handled(false)
	{
	}
}
