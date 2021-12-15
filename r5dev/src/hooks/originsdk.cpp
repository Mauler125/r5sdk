#include "pch.h"
#include "hooks.h"

namespace Hooks
{
	OriginGetErrorDescriptionWrapperFn originalOriginGetErrorDescriptionWrapper = nullptr;
}

const char* Hooks::OriginGetErrorDescription(std::uint32_t originCode)
{
	switch (originCode)
	{
	case ORIGIN_ERROR_CORE_AUTHENTICATION_FAILED:
	{
		MessageBoxA(NULL, "Origin LSX Authentication challenge failed.\nAre you perhaps not logged into Origin?", "R5 Reloaded", MB_OK);
		break;
	}
	case ORIGIN_ERROR_SDK_NOT_INITIALIZED:
	{
		MessageBoxA(NULL, "Origin SDK was not initialized.", "R5 Reloaded", MB_OK);
		break;
	}
	case ORIGIN_ERROR_CORE_NOTLOADED:
	{
		MessageBoxA(NULL, "Origin Desktop Application is not loaded.", "R5 Reloaded", MB_OK);
		break;
	}
	case ORIGIN_ERROR_CORE_LOGIN_FAILED:
	{
		MessageBoxA(NULL, "Origin couldn't authenticate with the Origin Servers.", "R5 Reloaded", MB_OK);
		break;
	}
	case ORIGIN_ERROR_CORE_NOT_INSTALLED:
	{
		MessageBoxA(NULL, "Origin is not installed on this machine or could not be found.\nOrigin is needed to run R5 Reloaded.", "R5 Reloaded", MB_OK);
		break;
	}
	default:
		break;
	}

	return originalOriginGetErrorDescriptionWrapper(originCode);
}