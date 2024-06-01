//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $NoKeywords: $
//===========================================================================//

#ifndef TIER1_CMD_H
#define TIER1_CMD_H

#include "tier1/characterset.h"

//-----------------------------------------------------------------------------
// Purpose: Command buffer context
//-----------------------------------------------------------------------------
enum class ECommandTarget_t : int
{
	CBUF_FIRST_PLAYER = 0,
	CBUF_LAST_PLAYER = MAX_SPLITSCREEN_CLIENTS - 1,
	CBUF_SERVER = CBUF_LAST_PLAYER + 1,

	CBUF_COUNT,
};

//-----------------------------------------------------------------------------
// Sources of console commands
//-----------------------------------------------------------------------------
enum class cmd_source_t : int
{
	kCommandSrcCode,
	kCommandSrcClientCmd,
	kCommandSrcUserInput,
	kCommandSrcNetClient,
	kCommandSrcNetServer,
	kCommandSrcDemoFile,
	kCommandSrcInvalid = -1
};

//-----------------------------------------------------------------------------
// Purpose: Command tokenizer
//-----------------------------------------------------------------------------
class CCommand
{
private:
	enum
	{
		COMMAND_MAX_ARGC   = 64,
		COMMAND_MAX_LENGTH = 512,
	};

public:
	CCommand();
	CCommand(int nArgC, const char** ppArgV, cmd_source_t source);
	bool Tokenize(const char* pCommand, cmd_source_t source = cmd_source_t::kCommandSrcCode, characterset_t* pBreakSet = nullptr);

	int ArgC(void) const;
	const char** ArgV(void) const;
	const char* ArgS(void) const;
	const char* GetCommandString(void) const;
	const char* Arg(int nIndex) const;
	const char* operator[](int nIndex) const;

	bool HasOnlyDigits(int nIndex) const;
	void Reset();

	static int MaxCommandLength(void);
	static characterset_t* DefaultBreakSet();

private:
	cmd_source_t m_nQueuedVal;
	int          m_nArgc;
	ssize_t      m_nArgv0Size;
	char         m_pArgSBuffer[COMMAND_MAX_LENGTH];
	char         m_pArgvBuffer[COMMAND_MAX_LENGTH];
	const char*  m_ppArgv[COMMAND_MAX_ARGC];
};

//-----------------------------------------------------------------------------
// Purpose: returns max command length
//-----------------------------------------------------------------------------
inline int CCommand::MaxCommandLength(void)
{
	return COMMAND_MAX_LENGTH - 1;
}

//-----------------------------------------------------------------------------
// Purpose: returns argument count
//-----------------------------------------------------------------------------
inline int CCommand::ArgC(void) const
{
	return m_nArgc;
}

//-----------------------------------------------------------------------------
// Purpose: returns argument vector
//-----------------------------------------------------------------------------
inline const char** CCommand::ArgV(void) const
{
	return m_nArgc ? (const char**)m_ppArgv : NULL;
}

//-----------------------------------------------------------------------------
// Purpose: returns all args that occur after the 0th arg, in string form
//-----------------------------------------------------------------------------
inline const char* CCommand::ArgS(void) const
{
	return m_nArgv0Size ? &m_pArgSBuffer[m_nArgv0Size] : "";
}

//-----------------------------------------------------------------------------
// Purpose: returns the entire command in string form, including the 0th arg
//-----------------------------------------------------------------------------
inline const char* CCommand::GetCommandString(void) const
{
	return m_nArgc ? m_pArgSBuffer : "";
}

//-----------------------------------------------------------------------------
// Purpose: returns argument from index as string
// Input  : nIndex - 
//-----------------------------------------------------------------------------
inline const char* CCommand::Arg(int nIndex) const
{
	// FIXME: Many command handlers appear to not be particularly careful
	// about checking for valid argc range. For now, we're going to
	// do the extra check and return an empty string if it's out of range
	if (nIndex < 0 || nIndex >= m_nArgc)
	{
		Assert(0);
		return "";
	}
	return m_ppArgv[nIndex];
}

//-----------------------------------------------------------------------------
// Purpose: gets at arguments
// Input  : nInput - 
//-----------------------------------------------------------------------------
inline const char* CCommand::operator[](int nIndex) const
{
	return Arg(nIndex);
}

#endif // TIER1_CMD_H
