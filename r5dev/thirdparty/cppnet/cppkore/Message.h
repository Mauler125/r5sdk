#pragma once

#include <cstdint>
#include <Windows.h>

namespace Forms
{
	// Implements a Windows message.
	struct Message
	{
		uintptr_t HWnd;
		uint32_t Msg;

		uintptr_t WParam;
		uintptr_t LParam;
		uintptr_t Result;

		// Construct a new Message instance from the parameters
		Message(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
			: HWnd((uintptr_t)hWnd), Msg(msg), WParam(wParam), LParam(lParam), Result(NULL)
		{
		}
	};
}