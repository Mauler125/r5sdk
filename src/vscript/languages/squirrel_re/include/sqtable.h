#ifndef SQTABLE_H
#define SQTABLE_H
#include "sqobject.h"
#include "sqstring.h"

#define hashptr(p)  ((SQHash)(((SQInteger)((intp)p)) >> 3))

inline SQHash HashObj(const SQObjectPtr& key) // TODO: untested
{
	switch (sq_type(key)) {
	case OT_STRING:		return _string(key)->_hash;
	case OT_FLOAT:		return (SQHash)((SQInteger)_float(key));
	case OT_BOOL: case OT_INTEGER:	return (SQHash)((SQInteger)_integer(key));
	default:			return hashptr(key._unVal.pRefCounted);
	}
}

struct SQTable : public SQDelegable
{
public:
	struct _HashNode
	{
		_HashNode() { hash = NULL; prev = NULL; next = NULL; }

		SQObjectPtr val;
		SQObjectPtr key;
		SQInteger hash;
		SQShort prev;
		SQShort next;
	};

	void _ClearNodes();
	inline _HashNode* _Get(const SQObjectPtr& key, SQHash hash) // TODO: untested
	{
		_HashNode* n = &_nodes[hash];
		do {
			if (_rawval(n->key) == _rawval(key) && sq_type(n->key) == sq_type(key)) {
				return n;
			}
		} while (n->hash != -1);
		return NULL;
	}
	bool Get(const SQObjectPtr& key, SQObjectPtr& val);

	_HashNode* _nodes;
	SQInteger _numofnodes;
	SQInteger _usednodes;
	SQInteger _prev;
	SQInteger _next;
	SQInteger _last;
};

#define SQ_FOR_EACH_TABLE(tableName, iteratorName) \
	for (int iteratorName = 0; iteratorName < tableName->_numofnodes; iteratorName++)

#endif // SQTABLE_H
