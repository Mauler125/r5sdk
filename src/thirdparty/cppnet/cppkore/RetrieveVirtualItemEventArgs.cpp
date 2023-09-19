#include "stdafx.h"
#include "RetrieveVirtualItemEventArgs.h"

namespace Forms
{
	RetrieveVirtualItemEventArgs::RetrieveVirtualItemEventArgs(int32_t Index, int32_t SubIndex)
		: ItemIndex(Index), SubItemIndex(SubIndex), Text(""), Style()
	{
	}
}
