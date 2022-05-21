#pragma once

namespace IO
{
	// Provides seek reference points.
	enum class SeekOrigin
	{
		Begin = 0,
		Current = 1,
		End = 2
	};
}