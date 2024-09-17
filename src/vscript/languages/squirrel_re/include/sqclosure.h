/*	see copyright notice in squirrel.h */
#ifndef _SQCLOSURE_H_
#define _SQCLOSURE_H_

struct SQClosure : public SQCollectable
{
public:
	char gap_40[16];
	SQObjectPtr _function;
};
static_assert(offsetof(SQClosure, _function) == 0x50);

#endif //_SQCLOSURE_H_
