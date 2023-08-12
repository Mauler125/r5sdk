#ifndef SQSTATE_H
#define SQSTATE_H
#include "squirrel.h"

#pragma pack(push, 1)
struct SQSharedState
{
	uint8_t gap0[17256];
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1) && !defined (GAMEDLL_S2)
	uint8_t gap1[32];
#endif
	void* _printfunc;
	uint8_t gap4390[33];
	SQChar _contextname[8];
	char gap43b9[135];
};
#pragma pack(pop)

struct SQBufState
{
	const SQChar* buf;
	const SQChar* bufTail;
	const SQChar* bufCopy;

	SQBufState(const SQChar* code)
	{
		buf = code;
		bufTail = code + strlen(code);
		bufCopy = code;
	}
};
#endif // SQSTATE_H