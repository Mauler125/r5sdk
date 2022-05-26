#include "stdafx.h"
#include "FormClosingEventArgs.h"

namespace Forms
{
	FormClosingEventArgs::FormClosingEventArgs(CloseReason Reason, bool Cancel)
		: CancelEventArgs(Cancel), Reason(Reason)
	{
	}
}
