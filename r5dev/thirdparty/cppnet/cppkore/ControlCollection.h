#pragma once

#include <memory>
#include <cstdint>
#include "ListBase.h"

namespace Forms
{
	// We can't create a circle dependency here, so we define an empty class for use later...
	class Control;

	// Contains a collection of child controls for a ContainerControl
	class ControlCollection
	{
	public:
		ControlCollection() = default;
		~ControlCollection();

		// Adds the specified control to the collection.
		void Add(Control* Ctrl);
		// Removes the specified control from the collection.
		void Remove(Control* Ctrl);
		// Removes the specified control by index from the collection.
		void RemoveAt(uint32_t Index);

		// Gets the index of the control in the collection.
		int32_t IndexOf(Control* Ctrl);
		// Checks if this collection contains the control.
		bool Contains(Control* Ctrl);
		
		// Sets the index of the specified child control.
		void SetChildIndex(Control* Ctrl, int32_t Index);

		// Returns the count of controls
		uint32_t Count();

		// Array index operator
		Control* operator[](size_t Index);

		// Iterator definitions, for for(& :) loop
		Control* begin() const noexcept;
		Control* end() const noexcept;

	private:
		
		// An internal pointer list of controls
		List<Control*> _Controls;

		// Internal routine to move a control
		void MoveElement(Control* Ctrl, int32_t CurrentIndex, int32_t NewIndex);
		// Internal routine to copy controls
		void CopyElement(int32_t SourceIndex, int32_t DestinationIndex, uint32_t Length);
	};
}