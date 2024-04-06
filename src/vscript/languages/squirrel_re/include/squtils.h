/*	see copyright notice in squirrel.h */
#ifndef _SQUTILS_H_
#define _SQUTILS_H_

template<typename T> class sqvector
{
private:
	SQInteger _globalnum;

public:
	T& top() const { return _vals[_size - 1]; }
	inline SQUnsignedInteger size() const { return _size; }
	bool empty() const { return (_size <= 0); }

	SQUnsignedInteger capacity() { return _allocated; }
	inline T& back() const { return _vals[_size - 1]; }
	inline T& operator[](SQUnsignedInteger pos) const { return _vals[pos]; }
	T* _vals;

private:
	SQUnsignedInteger _size;
	SQUnsignedInteger _allocated;
};

#endif //_SQUTILS_H_
