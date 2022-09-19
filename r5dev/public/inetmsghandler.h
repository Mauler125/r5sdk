//===== Copyright (c) 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//------------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//
#if !defined( INETMSGHANDLER_H )
#define INETMSGHANDLER_H

typedef struct netpacket_s netpacket_t;

abstract_class IConnectionlessPacketHandler
{
public:
	virtual ~IConnectionlessPacketHandler(void) = 0;
	virtual bool ProcessConnectionlessPacket(netpacket_t* packet) = 0;
};

abstract_class INetMessageHandler
{
public:
	virtual ~INetMessageHandler(void) = 0;
	virtual void ProcessStringCmd(void* msg) = 0;
	//virtual void ProcessScriptMessage(void* msg) = 0; // NET_ScriptMessage
	virtual void ProcessSetConVar(void* msg) = 0;
	virtual void ProcessSignonState(void* msg) = 0;
	virtual void ProcessMTXUserMsg(void* msg) = 0;
	//virtual void ProcessAutoPlayerMsg(void* msg) = 0;
};

abstract_class IServerMessageHandler : public INetMessageHandler // !TODO: PROCESS_SVC_MESSAGE macro.
{
public:
	virtual ~IServerMessageHandler(void) = 0;
	virtual void ProcessStringCmd(void* msg) = 0;
	//virtual void ProcessScriptMessage(void* msg) = 0;
	virtual void ProcessSetConVar(void* msg) = 0;
	virtual void ProcessSignonState(void* msg) = 0;
	virtual void ProcessMTXUserMsg(void* msg) = 0;
	//virtual void ProcessAutoPlayerMsg(void* msg) = 0;

	virtual bool ProcessPrint(void* msg) = 0;
	virtual bool ProcessServerInfo(void* msg) = 0;
	virtual bool ProcessSendTable(void* msg) = 0;
	virtual bool ProcessClassInfo(void* msg) = 0;
	virtual bool ProcessSetPause(void* msg) = 0;
	virtual bool ProcessPlaylists(void* msg) = 0;
	virtual bool ProcessCreateStringTable(void* msg) = 0;
	virtual bool ProcessUpdateStringTable(void* msg) = 0;
	virtual bool ProcessVoiceData(void* msg) = 0;
	virtual bool ProcessDurangoVoiceData(void* msg) = 0;
	virtual bool ProcessSounds(void* msg) = 0;
	virtual bool ProcessFixAngle(void* msg) = 0;
	virtual bool ProcessCrosshairAngle(void* msg) = 0;
	virtual bool ProcessGrantClientSidePickup(void* msg) = 0;
	virtual bool ProcessUserMessage(void* msg) = 0;
	virtual bool ProcessSnapshot(void* msg) = 0;
	virtual bool ProcessTempEntities(void* msg) = 0;
	virtual bool ProcessMenu(void* msg) = 0;
	virtual bool ProcessCmdKeyValues(void* msg) = 0;
	virtual bool ProcessServerTick(void* msg) = 0;
	virtual bool ProcessUseCachedPersistenceDefFile(void* msg) = 0;
	virtual bool ProcessPersistenceDefFile(void* msg) = 0;
	virtual bool ProcessPersistenceBaseline(void* msg) = 0;
	virtual bool ProcessPersistenceUpdateVar(void* msg) = 0;
	virtual bool ProcessPersistenceNotifySaved(void* msg) = 0;
	virtual bool ProcessDLCNotifyOwnership(void* msg) = 0;
	virtual bool ProcessMatchmakingETAs(void* msg) = 0;
	virtual bool ProcessMatchmakingStatus(void* msg) = 0;
	virtual bool ProcessMTX_ReadUserInfo(void* msg) = 0;
	virtual bool ProcessPlaylistChange(void* msg) = 0;
	virtual bool ProcessSetTeam(void* msg) = 0;
	virtual bool ProcessPlaylistOverrides(void* msg) = 0;
	virtual bool ProcessAntiCheat(void* msg) = 0;
	virtual bool ProcessAntiCheatChallenge(void* msg) = 0;
	virtual bool ProcessDatatableChecksum(void* msg) = 0;
	//virtual bool ProcessDeathRecap(void* msg) = 0;
};

#endif // INETMSGHANDLER_H