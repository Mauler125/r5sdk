#pragma once

#include <cstdint>
#include "DragDropEffects.h"

namespace Forms
{
	// Provides data for the DragDrop events.
	class DragEventArgs
	{
	public:
		DragEventArgs(IDataObject* Data, const int32_t KeyState, const int32_t X, const int32_t Y, const DragDropEffects AllowedEffect, DragDropEffects Effect);;
		~DragEventArgs() = default;

		// The data associated with this event.
		IDataObject* Data;
		// The current statie of the shift, ctrl, and alt keys.
		const int32_t KeyState;

		// The mouse X location.
		const int32_t X;
		// The mouse Y location.
		const int32_t Y;

		// The effect that should be applied to the mouse cursor.
		const DragDropEffects AllowedEffect;

		// Gets or sets which drag-and-drop operations are allowed by the target of the drag event.
		DragDropEffects Effect;
	};
}