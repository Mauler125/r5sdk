#pragma once

#include <cstdint>

namespace Forms
{
	// Specifies key codes and modifiers.
	enum class Keys
	{
		// The bit mask to extract a key code from a key value.
		KeyCode = 0x0000FFFF,
		// The bit mask to extract modifiers from a key value.
		Modifiers = (int)0xFFFF0000,
		// No key pressed.
		None = 0x00,
		// The left mouse button.
		LButton = 0x01,
		// The right mouse button.
		RButton = 0x02,
		// The CANCEL key.
		Cancel = 0x03,
		// The middle mouse button (three-button mouse).
		MButton = 0x04,
		// The first x mouse button (five-button mouse).
		XButton1 = 0x05,
		// The second x mouse button (five-button mouse).
		XButton2 = 0x06,
		// The BACKSPACE key.
		Back = 0x08,
		// The TAB key.
		Tab = 0x09,
		// The CLEAR key.
		LineFeed = 0x0A,
		// The CLEAR key.
		Clear = 0x0C,
		// The RETURN key.
		Return = 0x0D,
		// The ENTER key.
		Enter = Return,
		// The SHIFT key.
		ShiftKey = 0x10,
		// The CTRL key.
		ControlKey = 0x11,
		// The ALT key.
		Menu = 0x12,
		// The PAUSE key.
		Pause = 0x13,
		// The CAPS LOCK key.
		Capital = 0x14,
		// The CAPS LOCK key.
		CapsLock = 0x14,
		// The IME Kana mode key.
		KanaMode = 0x15,
		// The IME Hanguel mode key.
		HanguelMode = 0x15,
		// The IME Hangul mode key.
		HangulMode = 0x15,
		// The IME Junja mode key.
		JunjaMode = 0x17,
		// The IME Final mode key.
		FinalMode = 0x18,
		// The IME Hanja mode key.
		HanjaMode = 0x19,
		// The IME Kanji mode key.
		KanjiMode = 0x19,
		// The ESC key.
		Escape = 0x1B,
		// The IME Convert key.
		IMEConvert = 0x1C,
		// The IME NonConvert key.
		IMENonconvert = 0x1D,
		// The IME Accept key.
		IMEAccept = 0x1E,
		// The IME Accept key.
		IMEAceept = IMEAccept,
		// The IME Mode change request.
		IMEModeChange = 0x1F,
		// The SPACEBAR key.
		Space = 0x20,
		// The PAGE UP key.
		Prior = 0x21,
		// The PAGE UP key.
		PageUp = Prior,
		// The PAGE DOWN key.
		Next = 0x22,
		// The PAGE DOWN key.
		PageDown = Next,
		// The END key.
		End = 0x23,
		// The HOME key.
		Home = 0x24,
		// The LEFT ARROW key.
		Left = 0x25,
		// The UP ARROW key.
		Up = 0x26,
		// The RIGHT ARROW key.
		Right = 0x27,
		// The DOWN ARROW key.
		Down = 0x28,
		// The SELECT key.
		Select = 0x29,
		// The PRINT key.
		Print = 0x2A,
		// The EXECUTE key.
		Execute = 0x2B,
		// The PRINT SCREEN key.
		Snapshot = 0x2C,
		// The PRINT SCREEN key.
		PrintScreen = Snapshot,
		// The INS key.
		Insert = 0x2D,
		// The DEL key.
		Delete = 0x2E,
		// The HELP key.
		Help = 0x2F,
		// The 0 key.
		D0 = 0x30,
		// The 1 key.
		D1 = 0x31,
		// The 2 key.
		D2 = 0x32,
		// The 3 key.
		D3 = 0x33,
		// The 4 key.
		D4 = 0x34,
		// The 5 key.
		D5 = 0x35,
		// The 6 key.
		D6 = 0x36,
		// The 7 key.
		D7 = 0x37,
		// The 8 key.
		D8 = 0x38,
		// The 9 key.
		D9 = 0x39,
		// The A key.
		A = 0x41,
		// The B key.
		B = 0x42,
		// The C key.
		C = 0x43,
		// The D key.
		D = 0x44,
		// The E key.
		E = 0x45,
		// The F key.
		F = 0x46,
		// The G key.
		G = 0x47,
		// The H key.
		H = 0x48,
		// The I key.
		I = 0x49,
		// The J key.
		J = 0x4A,
		// The K key.
		K = 0x4B,
		// The L key.
		L = 0x4C,
		// The M key.
		M = 0x4D,
		// The N key.
		N = 0x4E,
		// The O key.
		O = 0x4F,
		// The P key.
		P = 0x50,
		// The Q key.
		Q = 0x51,
		// The R key.
		R = 0x52,
		// The S key.
		S = 0x53,
		// The T key.
		T = 0x54,
		// The U key.
		U = 0x55,
		// The V key.
		V = 0x56,
		// The W key.
		W = 0x57,
		// The X key.
		X = 0x58,
		// The Y key.
		Y = 0x59,
		// The Z key.
		Z = 0x5A,
		// The left Windows logo key (Microsoft Natural Keyboard).
		LWin = 0x5B,
		// The right Windows logo key (Microsoft Natural Keyboard).
		RWin = 0x5C,
		// The Application key (Microsoft Natural Keyboard).
		Apps = 0x5D,
		// The Computer Sleep key.
		Sleep = 0x5F,
		// The 0 key on the numeric keypad.
		NumPad0 = 0x60,
		// The 1 key on the numeric keypad.
		NumPad1 = 0x61,
		// The 2 key on the numeric keypad.
		NumPad2 = 0x62,
		// The 3 key on the numeric keypad.
		NumPad3 = 0x63,
		// The 4 key on the numeric keypad.
		NumPad4 = 0x64,
		// The 5 key on the numeric keypad.
		NumPad5 = 0x65,
		// The 6 key on the numeric keypad.
		NumPad6 = 0x66,
		// The 7 key on the numeric keypad.
		NumPad7 = 0x67,
		// The 8 key on the numeric keypad.
		NumPad8 = 0x68,
		// The 9 key on the numeric keypad.
		NumPad9 = 0x69,
		// The Multiply key.
		Multiply = 0x6A,
		// The Add key.
		Add = 0x6B,
		// The Separator key.
		Separator = 0x6C,
		// The Subtract key.
		Subtract = 0x6D,
		// The Decimal key.
		Decimal = 0x6E,
		// The Divide key.
		Divide = 0x6F,
		// The F1 key.
		F1 = 0x70,
		// The F2 key.
		F2 = 0x71,
		// The F3 key.
		F3 = 0x72,
		// The F4 key.
		F4 = 0x73,
		// The F5 key.
		F5 = 0x74,
		// The F6 key.
		F6 = 0x75,
		// The F7 key.
		F7 = 0x76,
		// The F8 key.
		F8 = 0x77,
		// The F9 key.
		F9 = 0x78,
		// The F10 key.
		F10 = 0x79,
		// The F11 key.
		F11 = 0x7A,
		// The F12 key.
		F12 = 0x7B,
		// The F13 key.
		F13 = 0x7C,
		// The F14 key.
		F14 = 0x7D,
		// The F15 key.
		F15 = 0x7E,
		// The F16 key.
		F16 = 0x7F,
		// The F17 key.
		F17 = 0x80,
		// The F18 key.
		F18 = 0x81,
		// The F19 key.
		F19 = 0x82,
		// The F20 key.
		F20 = 0x83,
		// The F21 key.
		F21 = 0x84,
		// The F22 key.
		F22 = 0x85,
		// The F23 key.
		F23 = 0x86,
		// The F24 key.
		F24 = 0x87,
		// The NUM LOCK key.
		NumLock = 0x90,
		// The SCROLL LOCK key.
		Scroll = 0x91,
		// The left SHIFT key.
		LShiftKey = 0xA0,
		// The right SHIFT key.
		RShiftKey = 0xA1,
		// The left CTRL key.
		LControlKey = 0xA2,
		// The right CTRL key.
		RControlKey = 0xA3,
		// The left ALT key.
		LMenu = 0xA4,
		// The right ALT key.
		RMenu = 0xA5,
		// The Browser Back key.
		BrowserBack = 0xA6,
		// The Browser Forward key.
		BrowserForward = 0xA7,
		// The Browser Refresh key.
		BrowserRefresh = 0xA8,
		// The Browser Stop key.
		BrowserStop = 0xA9,
		// The Browser Search key.
		BrowserSearch = 0xAA,
		// The Browser Favorites key.
		BrowserFavorites = 0xAB,
		// The Browser Home key.
		BrowserHome = 0xAC,
		// The Volume Mute key.
		VolumeMute = 0xAD,
		// The Volume Down key.
		VolumeDown = 0xAE,
		// The Volume Up key.
		VolumeUp = 0xAF,
		// The Media Next Track key.
		MediaNextTrack = 0xB0,
		// The Media Previous Track key.
		MediaPreviousTrack = 0xB1,
		// The Media Stop key.
		MediaStop = 0xB2,
		// The Media Play Pause key.
		MediaPlayPause = 0xB3,
		// The Launch Mail key.
		LaunchMail = 0xB4,
		// The Select Media key.
		SelectMedia = 0xB5,
		// The Launch Application1 key.
		LaunchApplication1 = 0xB6,
		// The Launch Application2 key.
		LaunchApplication2 = 0xB7,
		// The Oem Semicolon key.
		OemSemicolon = 0xBA,
		// The Oem 1 key.
		Oem1 = OemSemicolon,
		// The Oem plus key.
		Oemplus = 0xBB,
		// The Oem comma key.
		Oemcomma = 0xBC,
		// The Oem Minus key.
		OemMinus = 0xBD,
		// The Oem Period key.
		OemPeriod = 0xBE,
		// The Oem Question key.
		OemQuestion = 0xBF,
		// The Oem 2 key.
		Oem2 = OemQuestion,
		// The Oem tilde key.
		Oemtilde = 0xC0,
		// The Oem 3 key.
		Oem3 = Oemtilde,
		// The Oem Open Brackets key.
		OemOpenBrackets = 0xDB,
		// The Oem 4 key.
		Oem4 = OemOpenBrackets,
		// The Oem Pipe key.
		OemPipe = 0xDC,
		// The Oem 5 key.
		Oem5 = OemPipe,
		// The Oem Close Brackets key.
		OemCloseBrackets = 0xDD,
		// The Oem 6 key.
		Oem6 = OemCloseBrackets,
		// The Oem Quotes key.
		OemQuotes = 0xDE,
		// The Oem 7 key.
		Oem7 = OemQuotes,
		// The Oem8 key.
		Oem8 = 0xDF,
		// The Oem Backslash key.
		OemBackslash = 0xE2,
		// The Oem 102 key.
		Oem102 = OemBackslash,
		// The PROCESS KEY key.
		ProcessKey = 0xE5,
		// The Packet KEY key.
		Packet = 0xE7,
		// The ATTN key.
		Attn = 0xF6,
		// The CRSEL key.
		Crsel = 0xF7,
		// The EXSEL key.
		Exsel = 0xF8,
		// The ERASE EOF key.
		EraseEof = 0xF9,
		// The PLAY key.
		Play = 0xFA,
		// The ZOOM key.
		Zoom = 0xFB,
		// A constant reserved for future use.
		NoName = 0xFC,
		// The PA1 key.
		Pa1 = 0xFD,
		// The CLEAR key.
		OemClear = 0xFE,
		// The SHIFT modifier key.
		Shift = 0x00010000,
		// The CTRL modifier key.
		Control = 0x00020000,
		// The ALT modifier key.
		Alt = 0x00040000,
	};
}