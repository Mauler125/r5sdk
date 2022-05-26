#include "stdafx.h"
#include "KeyEventArgs.h"

namespace Forms
{
	KeyEventArgs::KeyEventArgs(Keys KeyData)
		: _KeyData(KeyData), _SuppressKeyPress(false), _Handled(false)
	{
	}

	bool KeyEventArgs::Alt()
	{
		return ((int)_KeyData & (int)Keys::Alt) == (int)Keys::Alt;
	}

	bool KeyEventArgs::Control()
	{
		return ((int)_KeyData & (int)Keys::Control) == (int)Keys::Control;
	}

	bool KeyEventArgs::Shift()
	{
		return ((int)_KeyData & (int)Keys::Shift) == (int)Keys::Shift;
	}

	bool KeyEventArgs::Handled()
	{
		return this->_Handled;
	}

	void KeyEventArgs::SetHandled(bool Value)
	{
		this->_Handled = Value;
	}

	bool KeyEventArgs::SuppressKeyPress()
	{
		return _SuppressKeyPress;
	}

	void KeyEventArgs::SetSurpressKeyPress(bool Value)
	{
		_SuppressKeyPress = Value;
		_Handled = Value;
	}

	Keys KeyEventArgs::KeyCode()
	{
		return (Keys)((int)_KeyData & (int)Keys::KeyCode);
	}

	uint32_t KeyEventArgs::KeyValue()
	{
		return (uint32_t)((int)_KeyData & (int)Keys::KeyCode);
	}

	Keys KeyEventArgs::KeyData()
	{
		return _KeyData;
	}

	Keys KeyEventArgs::Modifiers()
	{
		return (Keys)((int)_KeyData & (int)Keys::Modifiers);
	}
}
