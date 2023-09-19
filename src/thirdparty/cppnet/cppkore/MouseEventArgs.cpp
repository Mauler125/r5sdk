#include "stdafx.h"
#include "MouseEventArgs.h"

namespace Forms
{
	MouseEventArgs::MouseEventArgs(MouseButtons Button, uint32_t Clicks, int32_t X, int32_t Y, int32_t Delta)
		: Button(Button), Clicks(Clicks), X(X), Y(Y), Delta(Delta)
	{
	}
}
