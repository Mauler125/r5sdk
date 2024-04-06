#ifndef KVERRORCONTEXT_H
#define KVERRORCONTEXT_H
#include "kverrorstack.h"

// a simple helper that creates stack entries as it goes in & out of scope
class CKeyErrorContext
{
public:
	~CKeyErrorContext()
	{
		g_KeyValuesErrorStack.Pop();
	}
	explicit CKeyErrorContext(int symName)
	{
		Init(symName);
	}
	void Reset(int symName)
	{
		g_KeyValuesErrorStack.Reset(m_stackLevel, symName);
	}
	int GetStackLevel() const
	{
		return m_stackLevel;
	}
private:
	void Init(int symName)
	{
		m_stackLevel = g_KeyValuesErrorStack.Push(symName);
	}

	int m_stackLevel;
};

#endif // KVERRORCONTEXT_H
