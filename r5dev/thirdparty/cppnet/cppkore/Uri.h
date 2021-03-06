#pragma once

#include <cstdint>
#include <memory>
#include "StringBase.h"
#include "InternetPortType.h"

namespace Net
{
	// Represents a parsed Url with all components required for a WebRequest
	class Uri
	{
	public:
		explicit Uri(const char* Url);
		Uri(const String& Url);

		// Returns the internet port of the Uri
		InternetPortType InternetPort;
		// Returns the host name of the Uri
		String Host;
		// Returns the full path and query of the Uri
		String Path;

		// Returns the fully built url of the Uri components
		String GetUrl();

	private:

		// Internal routine to parse a Uri
		void ParseUri(const String& Url);
	};
}