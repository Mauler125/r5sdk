#include "stdafx.h"
#include "CancelEventArgs.h"

namespace Forms
{
	CancelEventArgs::CancelEventArgs()
		: CancelEventArgs(false)
	{
	}

	CancelEventArgs::CancelEventArgs(bool Cancel)
		: Cancel(Cancel)
	{
	}
}
