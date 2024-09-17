#include "keys.h"
#include "windows/id3dx.h"
#include "geforce/reflex.h"
#include <materialsystem/cmaterialsystem.h>

KeyInfo_t* g_pKeyInfo = nullptr;
ButtonCode_t* g_pKeyEventTicks = nullptr;
short* g_nKeyEventCount = nullptr;


bool Input_Event(const InputEvent_t& inputEvent, const int noKeyUpCheck)
{
	bool runTriggerMarker = inputEvent.m_nData == ButtonCode_t::MOUSE_LEFT;
	const KeyInfo_t& keyInfo = g_pKeyInfo[inputEvent.m_nData];

	if (noKeyUpCheck)
	{
		const int v = (inputEvent.m_nType & 0xFFFFFFFD) == 0;

		if (keyInfo.m_nKeyDownTarget == v)
			runTriggerMarker = false;
	}

	if (runTriggerMarker && (inputEvent.m_nType != IE_ButtonReleased || keyInfo.m_bTrapKeyUp))
	{
		GFX_SetLatencyMarker(D3D11Device(), TRIGGER_FLASH, MaterialSystem()->GetCurrentFrameCount());
	}

	return v_Input_Event(inputEvent, noKeyUpCheck);
}

///////////////////////////////////////////////////////////////////////////////
void VKeys::Detour(const bool bAttach) const
{
	DetourSetup(&v_Input_Event, &Input_Event, bAttach);
}
