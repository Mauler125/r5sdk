//=============================================================================//
//
// Purpose: Completion functions for ConCommand callbacks.
//
//=============================================================================//
#include "core/stdafx.h"
#include "public/iconvar.h"
#include "engine/cmodel_bsp.h"
#include "tier1/strtools.h"
#include "completion.h"

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *partial - 
//			context - 
//			longest - 
//			maxcommands - 
//			**commands - 
// Output : int
//-----------------------------------------------------------------------------
int _Host_Map_f_CompletionFunc(char const* cmdname, char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	char* substring = (char*)partial;
	if (strstr(partial, cmdname))
	{
		substring = (char*)partial + strlen(cmdname);
	}

	const size_t mapcount = g_vAllMaps.size();
	const size_t longest = COMMAND_COMPLETION_ITEM_LENGTH;
	const size_t count = MIN(mapcount, COMMAND_COMPLETION_MAXITEMS);

	if (count > 0)
	{
		for (size_t i = 0; i < count; i++)
		{
			strncpy(commands[i], g_vAllMaps[i].c_str(), longest);

			char old[COMMAND_COMPLETION_ITEM_LENGTH];
			strncpy(old, commands[i], sizeof(old));

			snprintf(commands[i], sizeof(commands[i]), "%s%s", cmdname, old);
			commands[i][strlen(commands[i])] = '\0';
		}
	}

	return count;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *partial - 
//			context - 
//			longest - 
//			maxcommands - 
//			**commands - 
// Output : int
//-----------------------------------------------------------------------------
int Host_SSMap_f_CompletionFunc(char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	char const* cmdname = "ss_map ";
	return _Host_Map_f_CompletionFunc(cmdname, partial, commands);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *partial - 
//			context - 
//			longest - 
//			maxcommands - 
//			**commands - 
// Output : int
//-----------------------------------------------------------------------------
int Host_Map_f_CompletionFunc(char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	char const* cmdname = "map ";
	return _Host_Map_f_CompletionFunc(cmdname, partial, commands);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *partial - 
//			context - 
//			longest - 
//			maxcommands - 
//			**commands - 
// Output : int
//-----------------------------------------------------------------------------
int Host_Background_f_CompletionFunc(char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	char const* cmdname = "map_background ";
	return _Host_Map_f_CompletionFunc(cmdname, partial, commands);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *partial - 
//			context - 
//			longest - 
//			maxcommands - 
//			**commands - 
// Output : int
//-----------------------------------------------------------------------------
int Host_Changelevel_f_CompletionFunc(char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	char const* cmdname = "changelevel ";
	return _Host_Map_f_CompletionFunc(cmdname, partial, commands);
}
