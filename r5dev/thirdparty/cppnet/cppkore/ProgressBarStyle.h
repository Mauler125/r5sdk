#pragma once

#include <cstdint>

namespace Forms
{
	// This Enumeration represents the styles the ProgressBar can take.
	enum class ProgressBarStyle
	{
		// The progress bar displays the progress status as a segmented bar.
		Blocks,
		// The progress bar displays the progress status in a smooth scrolling bar.
		Continuous,
		// The progress bar displays the progress status in the marquee style.
		Marquee
	};
}