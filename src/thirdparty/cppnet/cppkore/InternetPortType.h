#pragma once

#include <cstdint>

namespace Net
{
	// Specifies the internet protocol port to use in a Uri request
	enum class InternetPortType
	{
		Default = 0,
		Http = 1,
		Https = 2
	};
}