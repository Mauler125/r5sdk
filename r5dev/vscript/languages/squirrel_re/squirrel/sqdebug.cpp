/*
	see copyright notice in squirrel.h
*/
#include "sqvm.h"
#include "sqstring.h"

void SQVM::PrintObjVal(const SQObject* oin, SQObject* oout)
{
	v_SQVM_PrintObjVal(this, oin, oout);
}
