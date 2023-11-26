#pragma once
#include "public/iconvar.h"
#include "vstdlib/autocompletefilelist.h"

int Host_SSMap_f_CompletionFunc(char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);
int Host_Map_f_CompletionFunc(char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);
int Host_Background_f_CompletionFunc(char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);
int Host_Changelevel_f_CompletionFunc(char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);

int Game_Give_f_CompletionFunc(char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);

int RTech_PakLoad_f_CompletionFunc(char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);
int RTech_PakUnload_f_CompletionFunc(char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);
int RTech_PakDecompress_f_CompletionFunc(char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);

inline CMemory p_CBaseAutoCompleteFileList_AutoCompletionFunc;
inline int(*v_CBaseAutoCompleteFileList_AutoCompletionFunc)
(CBaseAutoCompleteFileList* thisp, const char* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);

///////////////////////////////////////////////////////////////////////////////
class VCompletion : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CBaseAutoCompleteFileList::AutoCompletionFunc", p_CBaseAutoCompleteFileList_AutoCompletionFunc.GetPtr());
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CBaseAutoCompleteFileList_AutoCompletionFunc = g_GameDll.FindPatternSIMD("40 55 53 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ?? 48 81 EC ?? ?? ?? ?? 48 8B 39");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CBaseAutoCompleteFileList_AutoCompletionFunc = g_GameDll.FindPatternSIMD("48 8B C4 4C 89 40 18 55 41 54");
#endif
		v_CBaseAutoCompleteFileList_AutoCompletionFunc = p_CBaseAutoCompleteFileList_AutoCompletionFunc.RCast<int(*)(
			CBaseAutoCompleteFileList*, const char*, char[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])>();
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////
