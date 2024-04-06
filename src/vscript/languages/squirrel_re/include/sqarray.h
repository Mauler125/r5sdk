/*	see copyright notice in squirrel.h */
#ifndef _SQARRAY_H_
#define _SQARRAY_H_

struct SQArray : public CHAINABLE_OBJ
{
public:
	bool Get(const SQInteger nidx, SQObjectPtr& val)
	{
		if (nidx >= 0 && nidx < (SQInteger)_values.size()) {
			SQObjectPtr& o = _values[nidx];
			val = _realval(o);
			return true;
		}
		else return false;
	}
	bool Set(const SQInteger nidx, const SQObjectPtr& val)
	{
		if (nidx >= 0 && nidx < (SQInteger)_values.size()) {
			_values[nidx] = val;
			return true;
		}
		else return false;
	}
	SQInteger Size() const { return _values.size(); }

	SQObjectPtrVec _values;
};

#endif //_SQARRAY_H_
