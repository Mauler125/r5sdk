#include "pch.h"
#include "hooks.h"

namespace Hooks
{
	namespace
	{
		static POINT g_pLastCursorPos{ 0 };
	}

	GetCursorPosFn originalGetCursorPos = nullptr;
	SetCursorPosFn originalSetCursorPos = nullptr;
	ClipCursorFn originalClipCursor = nullptr;
	ShowCursorFn originalShowCursor = nullptr;
}

BOOL WINAPI Hooks::GetCursorPos(LPPOINT lpPoint)
{
	if (g_bBlockInput)
	{
		assert(lpPoint != nullptr);
		*lpPoint = g_pLastCursorPos;
	}

	return originalGetCursorPos(lpPoint);
}

BOOL WINAPI Hooks::SetCursorPos(int X, int Y)
{
	g_pLastCursorPos.x = X;
	g_pLastCursorPos.y = Y;

	if (g_bBlockInput)
	{
		return TRUE;
	}

	return originalSetCursorPos(X, Y);
}

BOOL WINAPI Hooks::ClipCursor(const RECT* lpRect)
{
	if (g_bBlockInput)
	{
		lpRect = nullptr;
	}

	return originalClipCursor(lpRect);
}

BOOL WINAPI Hooks::ShowCursor(BOOL bShow)
{
	if (g_bBlockInput)
	{
		bShow = TRUE;
	}

	return originalShowCursor(bShow);
}