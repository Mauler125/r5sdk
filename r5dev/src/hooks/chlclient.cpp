#include "pch.h"
#include "hooks.h"

namespace Hooks
{
	FrameStageNotifyFn originalFrameStageNotify = nullptr;
}

void __fastcall Hooks::FrameStageNotify(CHLClient* rcx, ClientFrameStage_t curStage)
{
	switch (curStage)
	{
	case FRAME_START: // FrameStageNotify gets called every frame by CEngine::Frame with the stage being FRAME_START. We can use this to check/set global variables.
	{
		if (!GameGlobals::IsInitialized)
			GameGlobals::InitGameGlobals();

		break;
	}
	default:
		break;
	}
	originalFrameStageNotify(rcx, curStage);
}