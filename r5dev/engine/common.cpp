//=====================================================================================//
//
// Purpose: 
//
//=====================================================================================//

#include <core/stdafx.h>
#include <tier1/strtools.h>
#include <engine/common.h>

/*
==============================
COM_FormatSeconds

==============================
*/
const char* COM_FormatSeconds(int seconds)
{
	static char string[64];

	int hours = 0;
	int minutes = seconds / 60;

	if (minutes > 0)
	{
		seconds -= (minutes * 60);
		hours = minutes / 60;

		if (hours > 0)
		{
			minutes -= (hours * 60);
		}
	}

	if (hours > 0)
	{
		Q_snprintf(string, sizeof(string), "%2i:%02i:%02i", hours, minutes, seconds);
	}
	else
	{
		Q_snprintf(string, sizeof(string), "%02i:%02i", minutes, seconds);
	}

	return string;
}