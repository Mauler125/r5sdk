/*
	see copyright notice in squirrel.h
*/
#include "sqstate.h"

void RefTable::AddRef(SQObject& obj)
{
	SQHash mainpos;
	RefNode* prev;
	RefNode* ref = Get(obj, mainpos, &prev, true);
	ref->refs++;
}

SQBool RefTable::Release(SQObject& obj)
{
	return v_RefTable__Release(this, &obj);
}

RefTable::RefNode* RefTable::Get(SQObject& obj, SQHash& mainpos, RefNode** prev, bool add)
{
	return (RefTable::RefNode*)v_RefTable__Get(this, &obj, &mainpos, prev, add);
}
