#ifndef CMD_H
#define CMD_H
#include "tier1/commandbuffer.h"

#define MAX_EXECUTION_MARKERS 2048

typedef enum
{
	eCmdExecutionMarker_Enable_FCVAR_SERVER_CAN_EXECUTE = 'a',
	eCmdExecutionMarker_Disable_FCVAR_SERVER_CAN_EXECUTE = 'b',

	eCmdExecutionMarker_Enable_FCVAR_CLIENTCMD_CAN_EXECUTE = 'c',
	eCmdExecutionMarker_Disable_FCVAR_CLIENTCMD_CAN_EXECUTE = 'd'
} ECmdExecutionMarker;

//-----------------------------------------------------------------------------
// Purpose: Returns current player calling this function
// Output : ECommandTarget_t - 
//-----------------------------------------------------------------------------
FORCEINLINE ECommandTarget_t Cbuf_GetCurrentPlayer(void)
{
	// Always returns 'CBUF_FIRST_PLAYER' in Respawn's code.
	return ECommandTarget_t::CBUF_FIRST_PLAYER;
}

extern bool Cbuf_HasRoomForExecutionMarkers(const int cExecutionMarkers);
extern bool Cbuf_AddTextWithMarkers(const char* text, const ECmdExecutionMarker markerLeft, const ECmdExecutionMarker markerRight);

#ifndef CLIENT_DLL
extern bool Cmd_ExecuteUnrestricted(const char* const pCommandString, const char* const pValueString);
#endif // CLIENT_DLL

/* ==== COMMAND_BUFFER ================================================================================================================================================== */
inline void(*Cbuf_AddText)(ECommandTarget_t eTarget, const char* pText, cmd_source_t cmdSource);
inline void(*Cbuf_AddExecutionMarker)(ECommandTarget_t target, ECmdExecutionMarker marker);
inline void(*Cbuf_Execute)(void);
inline void(*v_Cmd_Dispatch)(ECommandTarget_t eTarget, const ConCommandBase* pCmdBase, const CCommand* pCommand, bool bCallBackupCallback);
inline bool(*v_Cmd_ForwardToServer)(const CCommand* pCommand);

extern CCommandBuffer** s_pCommandBuffer;
extern LPCRITICAL_SECTION s_pCommandBufferMutex;

extern CUtlVector<int>* g_pExecutionMarkers;


///////////////////////////////////////////////////////////////////////////////
class VCmd : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("Cbuf_AddText", Cbuf_AddText);
		LogFunAdr("Cbuf_AddExecutionMarker", Cbuf_AddExecutionMarker);
		LogFunAdr("Cbuf_Execute", Cbuf_Execute);
		LogFunAdr("Cmd_Dispatch", v_Cmd_Dispatch);
		LogFunAdr("Cmd_ForwardToServer", v_Cmd_ForwardToServer);
		LogVarAdr("s_CommandBuffer", s_pCommandBuffer);
		LogVarAdr("s_CommandBufferMutex", s_pCommandBufferMutex);
		LogVarAdr("g_ExecutionMarkers", g_pExecutionMarkers);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 63 D9 41 8B F8 48 8D 0D ?? ?? ?? ?? 48 8B F2 FF 15 ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 41 B9 ?? ?? ?? ??").GetPtr(Cbuf_AddText);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 81 EC ?? ?? ?? ?? 44 8B 05 ?? ?? ?? ??").GetPtr(Cbuf_AddExecutionMarker);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 FF 15 ?? ?? ?? ??").GetPtr(Cbuf_Execute);

		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 8B ?? 0C 49 FF C7").FollowNearCallSelf().GetPtr(v_Cmd_Dispatch);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 81 EC ?? ?? ?? ?? 44 8B 59 04").GetPtr(v_Cmd_ForwardToServer);
	}
	virtual void GetVar(void) const
	{
		s_pCommandBuffer      = CMemory(Cbuf_AddText).FindPattern("48 8D 05").ResolveRelativeAddressSelf(3, 7).RCast<CCommandBuffer**>();
		s_pCommandBufferMutex = CMemory(Cbuf_AddText).FindPattern("48 8D 0D").ResolveRelativeAddressSelf(3, 7).RCast<LPCRITICAL_SECTION>();
		g_pExecutionMarkers   = CMemory(Cbuf_AddExecutionMarker).FindPattern("48 8B 0D").ResolveRelativeAddressSelf(3, 7).RCast<CUtlVector<int>*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};

#endif // CMD_H
