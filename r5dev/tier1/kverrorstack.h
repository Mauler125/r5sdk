#ifndef KVERRORSTACK_H
#define KVERRORSTACK_H
#include "ikeyvaluessystem.h"

// a simple class to keep track of a stack of valid parsed symbols
const int MAX_ERROR_STACK = 64;
class CKeyValuesErrorStack
{
public:
	CKeyValuesErrorStack() : m_pFilename("NULL"), m_errorIndex(0), m_maxErrorIndex(0), m_bEncounteredErrors(false) {}

	void SetFilename(const char* pFilename)
	{
		m_pFilename = pFilename;
		m_maxErrorIndex = 0;
	}

	// entering a new keyvalues block, save state for errors
	// Not save symbols instead of pointers because the pointers can move!
	int Push(int symName)
	{
		if (m_errorIndex < MAX_ERROR_STACK)
		{
			m_errorStack[m_errorIndex] = symName;
		}
		m_errorIndex++;
		m_maxErrorIndex = MAX(m_maxErrorIndex, (m_errorIndex - 1));
		return m_errorIndex - 1;
	}

	// exiting block, error isn't in this block, remove.
	void Pop()
	{
		m_errorIndex--;
		Assert(m_errorIndex >= 0);
	}

	// Allows you to keep the same stack level, but change the name as you parse peers
	void Reset(int stackLevel, int symName)
	{
		Assert(stackLevel >= 0 && stackLevel < m_errorIndex);
		if (stackLevel < MAX_ERROR_STACK)
			m_errorStack[stackLevel] = symName;
	}

	// Hit an error, report it and the parsing stack for context
	void ReportError(const char* pError)
	{
		Warning(eDLL_T::COMMON, "KeyValues Error: %s in file %s\n", pError, m_pFilename);
		for (int i = 0; i < m_maxErrorIndex; i++)
		{
			if (i < MAX_ERROR_STACK && m_errorStack[i] != INVALID_KEY_SYMBOL)
			{
				if (i < m_errorIndex)
				{
					Warning(eDLL_T::COMMON, "%s, ", KeyValuesSystem()->GetStringForSymbol(m_errorStack[i]));
				}
				else
				{
					Warning(eDLL_T::COMMON, "(*%s*), ", KeyValuesSystem()->GetStringForSymbol(m_errorStack[i]));
				}
			}
		}
		Warning(eDLL_T::COMMON, "\n");
		m_bEncounteredErrors = true;
	}

	bool EncounteredAnyErrors()
	{
		return m_bEncounteredErrors;
	}

	void ClearErrorFlag()
	{
		m_bEncounteredErrors = false;
	}

private:
	int		m_errorStack[MAX_ERROR_STACK];
	const char* m_pFilename;
	int		m_errorIndex;
	int		m_maxErrorIndex;
	bool	m_bEncounteredErrors;
};

inline CKeyValuesErrorStack g_KeyValuesErrorStack;

#endif // KVERRORSTACK_H
