//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: command buffer class implementation
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//===========================================================================//


#ifndef COMMANDBUFFER_H
#define COMMANDBUFFER_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/utllinkedlist.h"
#include "tier1/convar.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CUtlBuffer;


//-----------------------------------------------------------------------------
// Command handle
//-----------------------------------------------------------------------------
typedef intptr_t CommandHandle_t;
enum
{
	COMMAND_BUFFER_INVALID_COMMAND_HANDLE = 0
};


//-----------------------------------------------------------------------------
// A command buffer class- a queue of argc/argv based commands associated
// with a particular time
//-----------------------------------------------------------------------------
class CCommandBuffer
{
public:
	// Constructor, destructor
	CCommandBuffer();
	~CCommandBuffer();

    // Inserts text into the command buffer
	bool AddText( const char *const pText, const int nTickDelay = 0, const cmd_source_t cmdSource = cmd_source_t::kCommandSrcInvalid );

	// Used to iterate over all commands appropriate for the current time
	void BeginProcessingCommands( const int nDeltaTicks );
	bool DequeueNextCommand( );
	int DequeueNextCommand( const char **& ppArgv );
	int ArgC() const;
	const char **ArgV() const;
	const char *ArgS() const;		// All args that occur after the 0th arg, in string form
	const char *GetCommandString() const;	// The entire command in string form, including the 0th arg
	const CCommand& GetCommand() const;
	void EndProcessingCommands();

	// Are we in the middle of processing commands?
	bool IsProcessingCommands() const;

	// Delays all queued commands to execute at a later time
	void DelayAllQueuedCommands( const int nTickDelay );

	// Indicates how long to delay when encountering a 'wait' command
	void SetWaitDelayTime( const int nTickDelay );

	// Returns a handle to the next command to process
	// (useful when inserting commands into the buffer during processing
	// of commands to force immediate execution of those commands,
	// most relevantly, to implement a feature where you stream a file
	// worth of commands into the buffer, where the file size is too large
	// to entirely contain in the buffer).
	CommandHandle_t GetNextCommandHandle() const;

	// Specifies a max limit of the args buffer. For unit testing. Size == 0 means use default
	void LimitArgumentBufferSize( ssize_t nSize );

	void SetWaitEnabled( const bool bEnable )		{ m_bWaitEnabled = bEnable; }
	bool IsWaitEnabled( void ) const				{ return m_bWaitEnabled; }

	ssize_t GetArgumentBufferSize() const			{ return m_nArgSBufferSize; }
	ssize_t GetMaxArgumentBufferSize() const		{ return m_nMaxArgSBufferLength; }

private:
	enum
	{
		ARGS_BUFFER_LENGTH = 8192,
	};

	struct Command_t
	{
		int m_nTick;
		ssize_t m_nFirstArgS;
		ssize_t m_nBufferSize;
		cmd_source_t m_Source;
	};

	// Insert a command into the command queue at the appropriate time
	void InsertCommandAtAppropriateTime( const intptr_t hCommand );
						   
	// Insert a command into the command queue
	// Only happens if it's inserted while processing other commands
	void InsertImmediateCommand( const intptr_t hCommand );

	// Insert a command into the command queue
	bool InsertCommand( const char *const pArgS, ssize_t nCommandSize, const int nTick, const cmd_source_t cmdSource );

	// Returns the length of the next command, as well as the offset to the next command
	void GetNextCommandLength( const char *const pText, const ssize_t nMaxLen, ssize_t *const pCommandLength, ssize_t *const pNextCommandOffset ) const;

	// Compacts the command buffer
	void Compact();

	// Parses argv0 out of the buffer
	bool ParseArgV0( CUtlBuffer &buf, char *const pArgv0, const ssize_t nMaxLen, const char **const pArgs ) const;

	char	m_pArgSBuffer[ ARGS_BUFFER_LENGTH ];
	ssize_t	m_nArgSBufferSize;
	CUtlFixedLinkedList< Command_t >	m_Commands;
	int		m_nCurrentTick;
	int		m_nLastTickToProcess;
	int		m_nWaitDelayTicks;
	intptr_t	m_hNextCommand;
	ssize_t	m_nMaxArgSBufferLength;
	bool	m_bIsProcessingCommands;
	bool	m_bWaitEnabled;

	// NOTE: This is here to avoid the pointers returned by DequeueNextCommand
	// to become invalid by calling AddText. Is there a way we can avoid the memcpy?
	CCommand m_CurrentCommand;
};


//-----------------------------------------------------------------------------
// Returns the next command
//-----------------------------------------------------------------------------
inline int CCommandBuffer::ArgC() const
{
	return m_CurrentCommand.ArgC();
}

inline const char **CCommandBuffer::ArgV() const
{
	return m_CurrentCommand.ArgV();
}

inline const char *CCommandBuffer::ArgS() const
{
	return m_CurrentCommand.ArgS();
}

inline const char *CCommandBuffer::GetCommandString() const
{
	return m_CurrentCommand.GetCommandString();
}

inline const CCommand& CCommandBuffer::GetCommand() const
{
	return m_CurrentCommand;
}

#endif // COMMANDBUFFER_H
