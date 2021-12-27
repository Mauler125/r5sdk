#include "core/stdafx.h"
#include "tier0/cvar.h"
#include "tier0/IConVar.h"
#include "engine/sys_dll2.h"

//-------------------------------------------------------------------------
// ENGINE                                                                 |
ConVar* cm_debug_cmdquery                  = new ConVar();
ConVar* cm_return_false_cmdquery_all       = new ConVar();
ConVar* cm_return_false_cmdquery_cheats    = new ConVar();
//-------------------------------------------------------------------------
// SERVER                                                                 |
ConVar* sv_showconnecting                  = new ConVar();
//-------------------------------------------------------------------------
// CLIENT                                                                 |
ConVar* cl_drawconsoleoverlay              = new ConVar();
ConVar* cl_consoleoverlay_lines            = new ConVar();
ConVar* cl_consoleoverlay_offset_x         = new ConVar();
ConVar* cl_consoleoverlay_offset_y         = new ConVar();
//-------------------------------------------------------------------------
// FILESYSTEM                                                             |
ConVar* fs_warning_level_native            = new ConVar();
ConVar* fs_packedstore_entryblock_stats    = new ConVar();
//-------------------------------------------------------------------------
// MATERIALSYSTEM                                                         |
ConVar* mat_showdxoutput                 = new ConVar();
//-------------------------------------------------------------------------
// SQUIRREL                                                               |
ConVar* sq_showrsonloading                 = new ConVar();
ConVar* sq_showscriptloading               = new ConVar();
ConVar* sq_showvmoutput                    = new ConVar();
ConVar* sq_showvmwarning                   = new ConVar();
//-------------------------------------------------------------------------
// NETCHANNEL                                                             |
ConVar* net_userandomkey                   = new ConVar();
ConVar* r5net_matchmaking_hostname         = new ConVar();
ConVar* r5net_show_debug                   = new ConVar();

///////////////////////////////////////////////////////////////////////////////
CCVar* g_pCvar = reinterpret_cast<CCVar*>(p_CEngineAPI_Connect.FindPatternSelf("48 8D 0D", ADDRESS::Direction::DOWN, 40).ResolveRelativeAddressSelf(0x3, 0x7).GetPtr());
