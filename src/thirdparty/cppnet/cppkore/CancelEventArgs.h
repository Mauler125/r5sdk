#pragma once

#include <cstdint>

namespace Forms
{
	// Provides data for the cancel event.
	class CancelEventArgs
	{
	public:
		CancelEventArgs();
		CancelEventArgs(bool Cancel);
		~CancelEventArgs() = default;

		// Gets or sets a value indicating whether the operation should be cancelled.
		bool Cancel;
	};
}