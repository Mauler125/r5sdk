#ifndef KVTOKENREADER_H
#define KVTOKENREADER_H
#include "kverrorstack.h"
#include "tier1/keyvalues.h"

// This class gets the tokens out of a CUtlBuffer for KeyValues.
// Since KeyValues likes to seek backwards and seeking won't work with a text-mode CUtlStreamBuffer 
// (which is what dmserializers uses), this class allows you to seek back one token.
class CKeyValuesTokenReader
{
public:
	CKeyValuesTokenReader(KeyValues* pKeyValues, CUtlBuffer& buf);

	const char* ReadToken(bool& wasQuoted, bool& wasConditional);
	void SeekBackOneToken();

private:
	KeyValues* m_pKeyValues;
	CUtlBuffer& m_Buffer;

	int m_nTokensRead;
	bool m_bUsePriorToken;
	bool m_bPriorTokenWasQuoted;
	bool m_bPriorTokenWasConditional;
	static char s_pTokenBuf[KEYVALUES_TOKEN_SIZE];
};

char CKeyValuesTokenReader::s_pTokenBuf[KEYVALUES_TOKEN_SIZE];

CKeyValuesTokenReader::CKeyValuesTokenReader(KeyValues* pKeyValues, CUtlBuffer& buf) :
	m_Buffer(buf)
{
	m_pKeyValues = pKeyValues;
	m_nTokensRead = 0;
	m_bUsePriorToken = false;
}

const char* CKeyValuesTokenReader::ReadToken(bool& wasQuoted, bool& wasConditional)
{
	if (m_bUsePriorToken)
	{
		m_bUsePriorToken = false;
		wasQuoted = m_bPriorTokenWasQuoted;
		wasConditional = m_bPriorTokenWasConditional;
		return s_pTokenBuf;
	}

	m_bPriorTokenWasQuoted = wasQuoted = false;
	m_bPriorTokenWasConditional = wasConditional = false;

	if (!m_Buffer.IsValid())
		return NULL;

	// eating white spaces and remarks loop
	while (true)
	{
		m_Buffer.EatWhiteSpace();
		if (!m_Buffer.IsValid())
		{
			return NULL;	// file ends after reading whitespaces
		}

		// stop if it's not a comment; a new token starts here
		if (!m_Buffer.EatCPPComment())
			break;
	}

	const char* c = (const char*)m_Buffer.PeekGet(sizeof(char), 0);
	if (!c)
	{
		return NULL;
	}

	// read quoted strings specially
	if (*c == '\"')
	{
		m_bPriorTokenWasQuoted = wasQuoted = true;
		m_Buffer.GetDelimitedString(m_pKeyValues->m_bHasEscapeSequences ? GetCStringCharConversion() : GetNoEscCharConversion(),
			s_pTokenBuf, KEYVALUES_TOKEN_SIZE);

		++m_nTokensRead;
		return s_pTokenBuf;
	}

	if (*c == '{' || *c == '}' || *c == '=')
	{
		// it's a control char, just add this one char and stop reading
		s_pTokenBuf[0] = *c;
		s_pTokenBuf[1] = 0;
		m_Buffer.GetChar();
		++m_nTokensRead;
		return s_pTokenBuf;
	}

	// read in the token until we hit a whitespace or a control character
	bool bReportedError = false;
	bool bConditionalStart = false;
	int nCount = 0;
	while (1)
	{
		c = (const char*)m_Buffer.PeekGet(sizeof(char), 0);

		// end of file
		if (!c || *c == 0)
			break;

		// break if any control character appears in non quoted tokens
		if (*c == '"' || *c == '{' || *c == '}' || *c == '=')
			break;

		if (*c == '[')
			bConditionalStart = true;

		if (*c == ']' && bConditionalStart)
		{
			m_bPriorTokenWasConditional = wasConditional = true;
			bConditionalStart = false;
		}

		// break on whitespace
		if (V_isspace(*c) && !bConditionalStart)
			break;

		if (nCount < (KEYVALUES_TOKEN_SIZE - 1))
		{
			s_pTokenBuf[nCount++] = *c;	// add char to buffer
		}
		else if (!bReportedError)
		{
			bReportedError = true;
			g_KeyValuesErrorStack.ReportError(" ReadToken overflow");
		}

		m_Buffer.GetChar();
	}
	s_pTokenBuf[nCount] = 0;
	++m_nTokensRead;

	return s_pTokenBuf;
}

void CKeyValuesTokenReader::SeekBackOneToken()
{
	if (m_bUsePriorToken)
		Plat_FatalError(eDLL_T::COMMON, "CKeyValuesTokenReader::SeekBackOneToken: It is only possible to seek back one token at a time");

	if (m_nTokensRead == 0)
		Plat_FatalError(eDLL_T::COMMON, "CkeyValuesTokenReader::SeekBackOneToken: No tokens read yet");

	m_bUsePriorToken = true;
}

#endif // KVTOKENREADER_H
