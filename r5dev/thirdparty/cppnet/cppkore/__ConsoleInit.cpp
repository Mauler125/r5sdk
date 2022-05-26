#include "stdafx.h"
#include "__ConsoleInit.h"

namespace System
{
	__ConsoleInit::__ConsoleInit()
		: In(new IO::ConsoleStream(GetStdHandle(STD_INPUT_HANDLE), IO::FileAccess::Read)), Out(new IO::ConsoleStream(GetStdHandle(STD_OUTPUT_HANDLE), IO::FileAccess::Write))
	{
	}
}
