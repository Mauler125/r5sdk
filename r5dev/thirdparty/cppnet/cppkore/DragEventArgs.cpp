#include "stdafx.h"
#include "DragEventArgs.h"

namespace Forms
{
	DragEventArgs::DragEventArgs(IDataObject* Data, const int32_t KeyState, const int32_t X, const int32_t Y, const DragDropEffects AllowedEffect, DragDropEffects Effect)
		: Data(Data), KeyState(KeyState), X(X), Y(Y), AllowedEffect(AllowedEffect), Effect(Effect)
	{
	}
}
