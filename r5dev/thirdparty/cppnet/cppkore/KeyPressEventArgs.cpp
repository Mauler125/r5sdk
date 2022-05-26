#include "stdafx.h"
#include "KeyPressEventArgs.h"

namespace Forms
{
	KeyPressEventArgs::KeyPressEventArgs(char KeyChar)
		: KeyChar(KeyChar), _Handled(false)
	{
	}

	bool KeyPressEventArgs::Handled()
	{
		return _Handled;
	}

	void KeyPressEventArgs::SetHandled(bool Value)
	{
		_Handled = Value;
	}
}