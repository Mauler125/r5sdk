#pragma once
#include "inputsystem/iinputsystem.h"
#include "mathlib/bitvec.h"
#include "tier1/utlstringmap.h"
#include <Xinput.h>

//-----------------------------------------------------------------------------
// Implementation of the input system
//-----------------------------------------------------------------------------
class CInputSystem : public CTier1AppSystem< IInputSystem >
{
public:
	// !!!interface implemented in engine!!!

private:
	enum
	{
		INPUT_STATE_QUEUED = 0,
		INPUT_STATE_CURRENT,

		INPUT_STATE_COUNT,
	};

	struct xdevice_t
	{
		struct xvibration_t
		{
			float leftMainMotor;
			float rightMainMotor;
			float leftTriggerMotor;
			float rightTriggerMotor;
		};

		int userId;
		char active;
		XINPUT_STATE states[INPUT_STATE_COUNT];
		int newState;
		_BYTE gap6[20];
		bool pendingRumbleUpdate;
		_BYTE gap41[3];
		xvibration_t vibration;
		bool bUnk0;
		char field_55;
		_BYTE gap56[42];
		int field_80;
		_BYTE gap84[316];
	};

	struct appKey_t
	{
		int repeats;
		int	sample;
	};

	struct InputState_t
	{
		// Analog states
		CBitVec<BUTTON_CODE_LAST> m_ButtonState;
		int m_pAnalogValue[JOYSTICK_MAX_BUTTON_COUNT];
	};


	HWND m_ChainedWndProc;
	HWND m_hAttachedHWnd;
	bool m_bEnabled;
	bool m_bPumpEnabled;
	bool m_bIsPolling;
	bool m_bIMEComposing;
	bool m_bMouseCursorVisible;
	bool m_bJoystickCursorVisible;
	bool m_bIsInGame; // Delay joystick polling if in-game.

	// Current button state
	InputState_t m_InputState[INPUT_STATE_COUNT];

	// Current button state mutex
	CRITICAL_SECTION m_InputStateMutex;
	int unknown0;
	short unknown1;
	bool unknown2;

	// Analog event mutex
	CRITICAL_SECTION m_AnalogEventMutex;
	int unknown3;
	short unknown4;
	bool unknown5;

	// Analog events
	InputEvent_t m_AnalogEvents[JOYSTICK_AXIS_BUTTON_COUNT];
	int m_AnalogEventTypes[JOYSTICK_AXIS_BUTTON_COUNT];

	// Button events
	InputEvent_t m_Events[128];
	InputEvent_t m_CurrentEvent;

	DWORD m_StartupTimeTick;
	int m_nLastPollTick;
	int m_nLastSampleTick;
	int m_nLastAnalogPollTick;
	int m_nLastAnalogSampleTick;

	// Mouse wheel hack
	UINT m_uiMouseWheel;

	// Xbox controller info
	int m_nJoystickCount;
	appKey_t m_appXKeys[XUSER_MAX_COUNT][XK_MAX_KEYS+2];
	char pad_unk[16];
	xdevice_t m_XDevices[XUSER_MAX_COUNT];

	// Used to determine whether to generate UI events
	int m_nUIEventClientCount;

	// Raw mouse input
	bool m_bRawInputSupported;
	CRITICAL_SECTION m_MouseAccumMutex;
	int m_mouseRawAccumX;
	int m_mouseRawAccumY;

	_BYTE gap1785[8];

	// Current mouse capture window
	PlatWindow_t m_hCurrentCaptureWnd;

	// For the 'SleepUntilInput' feature
	HANDLE m_hEvent;

	InputCursorHandle_t m_pDefaultCursors[INPUT_CURSOR_COUNT];
	CUtlStringMap<InputCursorHandle_t> m_UserCursors;

	CSysModule* m_pXInputDLL;
	CSysModule* m_pRawInputDLL;

	// NVNT falcon module
	CSysModule* m_pNovintDLL; // Unused in R5?

	bool m_bIgnoreLocalJoystick;
	InputCursorHandle_t m_hCursor;
};

///////////////////////////////////////////////////////////////////////////////
extern CInputSystem* g_pInputSystem;

///////////////////////////////////////////////////////////////////////////////
class VInputSystem : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogVarAdr("g_pInputSystem", reinterpret_cast<uintptr_t>(g_pInputSystem));
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
		g_pInputSystem = g_GameDll.FindPatternSIMD("48 83 EC 28 48 8B 0D ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 48 89 05 ?? ?? ?? ?? 48 85 C9 74 11")
			.FindPatternSelf("48 89 05", CMemory::Direction::DOWN, 40).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CInputSystem*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////
