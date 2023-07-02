#ifndef CMD_H
#define CMD_H

ECommandTarget_t Cbuf_GetCurrentPlayer(void);

/* ==== COMMAND_BUFFER ================================================================================================================================================== */
inline CMemory p_Cbuf_AddText;
inline void(*Cbuf_AddText)(ECommandTarget_t eTarget, const char* pText, cmd_source_t cmdSource);

inline CMemory p_Cbuf_Execute;
inline void(*Cbuf_Execute)(void);

inline CMemory p_Cmd_ForwardToServer;
inline bool(*v_Cmd_ForwardToServer)(const CCommand* args);

///////////////////////////////////////////////////////////////////////////////
class VCmd : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("Cbuf_AddText", p_Cbuf_AddText.GetPtr());
		LogFunAdr("Cbuf_Execute", p_Cbuf_Execute.GetPtr());
		LogFunAdr("Cmd_ForwardToServer", p_Cmd_ForwardToServer.GetPtr());
	}
	virtual void GetFun(void) const
	{
		p_Cbuf_AddText = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 63 D9 41 8B F8 48 8D 0D ?? ?? ?? ?? 48 8B F2 FF 15 ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 41 B9 ?? ?? ?? ??");
		p_Cbuf_Execute = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 FF 15 ?? ?? ?? ??");
		p_Cmd_ForwardToServer = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 81 EC ?? ?? ?? ?? 44 8B 59 04");

		Cbuf_AddText = p_Cbuf_AddText.RCast<void (*)(ECommandTarget_t, const char*, cmd_source_t)>(); /*48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 63 D9 41 8B F8 48 8D 0D ?? ?? ?? ?? 48 8B F2 FF 15 ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 41 B9 ?? ?? ?? ??*/
		Cbuf_Execute = p_Cbuf_Execute.RCast<void (*)(void)>();                                        /*48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 FF 15 ?? ?? ?? ??*/
		v_Cmd_ForwardToServer = p_Cmd_ForwardToServer.RCast<bool (*)(const CCommand*)>();             /*48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 81 EC ?? ?? ?? ?? 44 8B 59 04*/
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const;
	virtual void Detach(void) const;
};

#endif // CMD_H
