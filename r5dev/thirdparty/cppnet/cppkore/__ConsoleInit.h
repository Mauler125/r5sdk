#pragma once

#include "StreamWriter.h"
#include "StreamReader.h"
#include "ConsoleStream.h"

namespace System
{
	class __ConsoleInit
	{
	public:
		__ConsoleInit();

		// The console streams
		IO::StreamReader In;
		IO::StreamWriter Out;
	};
}