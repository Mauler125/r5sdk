#include "stdafx.h"
#include "FormClosedEventArgs.h"

namespace Forms
{
	FormClosedEventArgs::FormClosedEventArgs(CloseReason Reason)
		: Reason(Reason)
	{
	}
}
