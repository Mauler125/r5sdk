#pragma once

#include <cstdint>
#include <type_traits>

namespace Forms
{
	// Specifies control states.
	enum class ControlStates
	{
		StateCreated = 0x00000001,
		StateVisible = 0x00000002,
		StateEnabled = 0x00000004,
		StateTabstop = 0x00000008,
		StateRecreate = 0x00000010,
		StateModal = 0x00000020,
		StateAllowDrop = 0x00000040,
		StateDropTarget = 0x00000080,
		StateNoZOrder = 0x00000100,
		StateLayoutDeferred = 0x00000200,
		StateUseWaitCursor = 0x00000400,
		StateDisposed = 0x00000800,
		StateDisposing = 0x00001000,
		StateMouseEnterPending = 0x00002000,
		StateTrackingMouseEvent = 0x00004000,
		StateThreadMarshallPending = 0x00008000,
		StateSizeLockedByOS = 0x00010000,
		StateCausesValidation = 0x00020000,
		StateCreatingHandle = 0x00040000,
		StateTopLevel = 0x00080000,
		StateISACCESSIBLE = 0x00100000,
		StateOwnCtlBrush = 0x00200000,
		StateExceptionWhilePainting = 0x00400000,
		StateLayoutIsDirty = 0x00800000,
		StateCheckedHost = 0x01000000,
		StateHostedInDialog = 0x02000000,
		StateDoubleClickFired = 0x04000000,
		StateMousePressed = 0x08000000,
		StateValidationCancelled = 0x10000000,
		StateParentRecreating = 0x20000000,
		StateMirrored = 0x40000000,
	};

	//
	// Allow bitwise operations on this enumeration
	//
	constexpr ControlStates operator|(ControlStates Lhs, ControlStates Rhs)
	{
		return static_cast<ControlStates>(static_cast<std::underlying_type<ControlStates>::type>(Lhs) | static_cast<std::underlying_type<ControlStates>::type>(Rhs));
	};
}