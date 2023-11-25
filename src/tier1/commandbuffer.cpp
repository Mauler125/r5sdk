//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: command buffer class implementation
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//===========================================================================//

#include "tier1/CommandBuffer.h"
#include "tier1/utlbuffer.h"
#include "tier1/strtools.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CCommandBuffer::CCommandBuffer() : m_Commands( 32, 32 )
{
	m_hNextCommand = m_Commands.InvalidIndex();
	m_nWaitDelayTicks = 1;
	m_nCurrentTick = 0;
	m_nLastTickToProcess = -1;
	m_nArgSBufferSize = 0;
	m_bIsProcessingCommands = false;
	m_nMaxArgSBufferLength = ARGS_BUFFER_LENGTH;
}

CCommandBuffer::~CCommandBuffer()
{
}


//-----------------------------------------------------------------------------
// Purpose: indicates how long to delay when encountering a 'wait' command
// Input  : nTickDelay - 
//-----------------------------------------------------------------------------
void CCommandBuffer::SetWaitDelayTime( const int nTickDelay )
{
	Assert( nTickDelay >= 0 );
	m_nWaitDelayTicks = nTickDelay;
}

	
//-----------------------------------------------------------------------------
// Purpose: specifies a max limit of the args buffer. For unit testing. Size == 0 means use default
// Input  : nSize - 
//-----------------------------------------------------------------------------
void CCommandBuffer::LimitArgumentBufferSize( ssize_t nSize )
{
	if ( nSize > ARGS_BUFFER_LENGTH )
	{
		nSize = ARGS_BUFFER_LENGTH;
	}

	m_nMaxArgSBufferLength = ( nSize == 0 ) ? ARGS_BUFFER_LENGTH : nSize;
}


//-----------------------------------------------------------------------------
// Purpose: parses argv0 out of the buffer
// Input  : &buf    - 
//          *pArgV0 - 
//          nMaxLen - 
//          **pArgS - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CCommandBuffer::ParseArgV0( CUtlBuffer &buf, char *const pArgV0,
	const ssize_t nMaxLen, const char **const pArgS ) const
{
	pArgV0[ 0 ] = 0;
	*pArgS = NULL;

	if ( !buf.IsValid() )
		return false;

	const ssize_t	nSize = buf.ParseToken( CCommand::DefaultBreakSet(), pArgV0, nMaxLen );
	if ( ( nSize <= 0 ) || ( nMaxLen == nSize ) )
		return false;

	const ssize_t nArgSLen = buf.TellMaxPut() - buf.TellGet();
	*pArgS = ( nArgSLen > 0 ) ? (const char*)buf.PeekGet() : NULL;
	return true;
}


//-----------------------------------------------------------------------------
// Purpose : insert a command into the command queue
// Inpur   : hCommand - 
//-----------------------------------------------------------------------------
void CCommandBuffer::InsertCommandAtAppropriateTime( const intptr_t hCommand )
{
	intptr_t i;
	Command_t &command = m_Commands[ hCommand ];
	for ( i = m_Commands.Head(); i != m_Commands.InvalidIndex(); i = m_Commands.Next( i ) )
	{
		if ( m_Commands[ i ].m_nTick > command.m_nTick )
			break;
	}
	m_Commands.LinkBefore( i, hCommand );
}


//-----------------------------------------------------------------------------
// Purpose: insert a command into the command queue at the appropriate time
// Input  : hCommand - 
//-----------------------------------------------------------------------------
void CCommandBuffer::InsertImmediateCommand( const intptr_t hCommand )
{
	m_Commands.LinkBefore( m_hNextCommand, hCommand );
}


//-----------------------------------------------------------------------------
// Purpose: insert a command into the command queue
// Input  : *pArgS       - 
//          nCommandSize - 
//          nTick        - 
//          cmdSource    - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CCommandBuffer::InsertCommand( const char *const pArgS, ssize_t nCommandSize,
	const int nTick, const cmd_source_t cmdSource )
{
	if ( nCommandSize >= CCommand::MaxCommandLength() )
	{
		Warning(eDLL_T::COMMON, "WARNING: Command too long... ignoring!\n%s\n", pArgS );
		return false;
	}

	// Add one for null termination.
	if ( m_nArgSBufferSize + nCommandSize+1 > m_nMaxArgSBufferLength )
	{
		Compact();
		if ( m_nArgSBufferSize + nCommandSize+1 > m_nMaxArgSBufferLength )
			return false;
	}
	
	memcpy( &m_pArgSBuffer[m_nArgSBufferSize], pArgS, nCommandSize );
	m_pArgSBuffer[ m_nArgSBufferSize + nCommandSize ] = 0;
	++nCommandSize;

	const intptr_t hCommand = m_Commands.Alloc();
	Command_t& command = m_Commands[ hCommand ];
	command.m_nTick = nTick;
	command.m_nFirstArgS = m_nArgSBufferSize;
	command.m_Source = cmdSource;
	command.m_nBufferSize = nCommandSize;

	m_nArgSBufferSize += nCommandSize;

	if ( !m_bIsProcessingCommands || ( nTick > m_nCurrentTick ) )
	{
		InsertCommandAtAppropriateTime( hCommand );
	}
	else
	{
		InsertImmediateCommand( hCommand );
	}
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: returns the length of the next command
// Input  : *pText              - 
//          nMaxLen             - 
//          *pCommandLength     - 
//          *pNextCommandOffset - 
//-----------------------------------------------------------------------------
void CCommandBuffer::GetNextCommandLength( const char *const pText, const ssize_t nMaxLen,
	ssize_t *const pCommandLength, ssize_t *const pNextCommandOffset ) const
{
	ssize_t nCommandLength = 0;
	ssize_t nNextCommandOffset;
	bool bIsQuoted = false;
	bool bIsCommented = false;

	for ( nNextCommandOffset=0; nNextCommandOffset < nMaxLen; ++nNextCommandOffset, nCommandLength += bIsCommented ? 0 : 1 )
	{
		const char c = pText[ nNextCommandOffset ];
		if ( !bIsCommented )
		{
			if ( c == '"' )
			{
				bIsQuoted = !bIsQuoted;
				continue;
			}

			// Don't break if inside a C++ style comment.
			if ( !bIsQuoted && c == '/' )
			{
				bIsCommented = ( nNextCommandOffset < nMaxLen-1 ) && pText[ nNextCommandOffset+1 ] == '/';
				if ( bIsCommented )
				{
					++nNextCommandOffset;
					continue;
				}
			}

			// Don't break if inside a quoted string.
			if ( !bIsQuoted && c == ';' )
				break;
		}

		// FIXME: This is legacy behavior; should we not break if a \n is inside a quoted string?
		if ( c == '\n' )
			break;
	}

	*pCommandLength = nCommandLength;
	*pNextCommandOffset = nNextCommandOffset;
}


//-----------------------------------------------------------------------------
// Purpose: add text to command buffer, return false if it couldn't owing to overflow
// Input  : *pText     - 
//          nTickDelay - 
//          cmdSource  - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CCommandBuffer::AddText( const char *const pText, const int nTickDelay, const cmd_source_t cmdSource )
{
	Assert( nTickDelay >= 0 );

	ssize_t	nLen = Q_strlen( pText );
	int nTick = m_nCurrentTick + nTickDelay;

	// Parse the text into distinct commands.
	const char *pCurrentCommand = pText;
	ssize_t nOffsetToNextCommand;

	for( ; nLen > 0; nLen -= nOffsetToNextCommand+1, pCurrentCommand += nOffsetToNextCommand+1 )
	{
		// Find a \n or ; line break.
		ssize_t nCommandLength;
		GetNextCommandLength( pCurrentCommand, nLen, &nCommandLength, &nOffsetToNextCommand );

		if ( nCommandLength <= 0 )
			continue;

		const char *pArgS;
		char *const pArgV0 = (char*)_alloca( nCommandLength+1 );
		CUtlBuffer bufParse( pCurrentCommand, nCommandLength, CUtlBuffer::TEXT_BUFFER | CUtlBuffer::READ_ONLY );
		ParseArgV0( bufParse, pArgV0, nCommandLength+1, &pArgS );
		if ( pArgV0[ 0 ] == 0 )
			continue;

		// Deal with the special 'wait' command.
		if ( !Q_stricmp( pArgV0, "wait" ) && IsWaitEnabled() )
		{
			const int nDelay = pArgS ? atoi( pArgS ) : m_nWaitDelayTicks;
			nTick += nDelay;
			continue;
		}

		if ( !InsertCommand( pCurrentCommand, nCommandLength, nTick, cmdSource ) )
			return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: checks if we are we in the middle of processing commands
//-----------------------------------------------------------------------------
bool CCommandBuffer::IsProcessingCommands() const
{
	return m_bIsProcessingCommands;
}


//-----------------------------------------------------------------------------
// Purpose: delays all queued commands to execute at a later time
// Input  : nDelay - 
//-----------------------------------------------------------------------------
void CCommandBuffer::DelayAllQueuedCommands( const int nDelay )
{
	if ( nDelay <= 0 )
		return;

	for ( intptr_t i = m_Commands.Head(); i != m_Commands.InvalidIndex(); i = m_Commands.Next( i ) )
	{
		m_Commands[ i ].m_nTick += nDelay;
	}
}

	
//-----------------------------------------------------------------------------
// Purpose: begin iterating over all commands up to flCurrentTime
// Input  : nDeltaTicks - 
//-----------------------------------------------------------------------------
void CCommandBuffer::BeginProcessingCommands( const int nDeltaTicks )
{
	if ( nDeltaTicks == 0 )
		return;

	Assert( !m_bIsProcessingCommands );
	m_bIsProcessingCommands = true;
	m_nLastTickToProcess = m_nCurrentTick + nDeltaTicks-1;

	// Necessary to insert commands while commands are being processed.
	m_hNextCommand = m_Commands.Head();
}


//-----------------------------------------------------------------------------
// Purpose: returns the next command
//-----------------------------------------------------------------------------
bool CCommandBuffer::DequeueNextCommand()
{
	m_CurrentCommand.Reset();

	Assert( m_bIsProcessingCommands );
	if ( m_Commands.Count() == 0 )
		return false;

	const intptr_t nHead = m_Commands.Head();
	const Command_t &command = m_Commands[ nHead ];
	if ( command.m_nTick > m_nLastTickToProcess )
		return false;

	m_nCurrentTick = command.m_nTick;

	// Copy the current command into a temp buffer
	// NOTE: This is here to avoid the pointers returned by DequeueNextCommand
	// to become invalid by calling AddText. Is there a way we can avoid the memcpy?
	if ( command.m_nBufferSize > 0 )
	{
		m_CurrentCommand.Tokenize( &m_pArgSBuffer[ command.m_nFirstArgS ] );
	}

	m_Commands.Remove( nHead );

	// Necessary to insert commands while commands are being processed.
	m_hNextCommand = m_Commands.Head();

//	Msg("Dequeue : ");
//	for ( int i = 0; i < nArgc; ++i )
//	{
//		Msg("%s ", m_pCurrentArgv[i] ); 
//	}
//	Msg("\n");
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: returns the next command
// Input  : **&ppArgv - 
//-----------------------------------------------------------------------------
int CCommandBuffer::DequeueNextCommand( const char **& ppArgv )
{
	DequeueNextCommand();
	ppArgv = ArgV();
	return ArgC();
}


//-----------------------------------------------------------------------------
// Purpose: compacts the command buffer
//-----------------------------------------------------------------------------
void CCommandBuffer::Compact()
{
	// Compress argvbuffer + argv
	// NOTE: I'm using this choice instead of calling malloc + free
	// per command to allocate arguments because I expect to post a
	// bunch of commands but not have many delayed commands; avoiding
	// the allocation cost seems more important that the memcpy cost
	// here since I expect to not have much to copy.
	m_nArgSBufferSize = 0;

	char pTempBuffer[ ARGS_BUFFER_LENGTH ];
	for ( intptr_t i = m_Commands.Head(); i != m_Commands.InvalidIndex(); i = m_Commands.Next( i ) )
	{
		Command_t &command = m_Commands[ i ];

		memcpy( &pTempBuffer[ m_nArgSBufferSize ], &m_pArgSBuffer[ command.m_nFirstArgS ], command.m_nBufferSize );
		command.m_nFirstArgS = m_nArgSBufferSize;
		m_nArgSBufferSize += command.m_nBufferSize;
	}

	// NOTE: We could also store 2 buffers in the command buffer and switch
	// between the two to avoid the 2nd memcpy; but again I'm guessing the memory
	// tradeoff isn't worth it
	memcpy( m_pArgSBuffer, pTempBuffer, m_nArgSBufferSize );
}


//-----------------------------------------------------------------------------
// Purpose: finish iterating over all commands
//-----------------------------------------------------------------------------
void CCommandBuffer::EndProcessingCommands()
{
	Assert( m_bIsProcessingCommands );
	m_bIsProcessingCommands = false;
	m_nCurrentTick = m_nLastTickToProcess+1;
	m_hNextCommand = m_Commands.InvalidIndex();

	// Extract commands that are before the end time.
	// NOTE: This is a bug for this to.
	intptr_t i = m_Commands.Head();
	if ( i == m_Commands.InvalidIndex() )
	{
		m_nArgSBufferSize = 0;
		return;
	}

	while ( i != m_Commands.InvalidIndex() )
	{
		if ( m_Commands[ i ].m_nTick >= m_nCurrentTick )
			break;

		//AssertMsgOnce( false, "CCommandBuffer::EndProcessingCommands() called before all appropriate commands were dequeued.\n" );
		intptr_t nNext = i;
		Msg( eDLL_T::COMMON, "Warning: Skipping command \"%s\"\n", &m_pArgSBuffer[ m_Commands[ i ].m_nFirstArgS ] );
		m_Commands.Remove( i );
		i = nNext;
	}

	Compact();
}


//-----------------------------------------------------------------------------
// Purpose: returns a handle to the next command to process
//-----------------------------------------------------------------------------
CommandHandle_t CCommandBuffer::GetNextCommandHandle() const
{
	Assert( m_bIsProcessingCommands );
	return m_Commands.Head();
}
