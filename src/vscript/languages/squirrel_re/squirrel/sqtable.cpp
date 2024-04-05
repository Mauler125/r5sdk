#include "sqtable.h"

void SQTable::_ClearNodes()
{
	for (SQInteger i = 0; i < _numofnodes; i++) { _nodes[i].key = _null_; _nodes[i].val = _null_; }
}

// TODO: untested
bool SQTable::Get(const SQObjectPtr& key, SQObjectPtr& val)
{
	if (sq_type(key) == OT_NULL)
		return false;
	_HashNode* n = _Get(key, HashObj(key) & (_numofnodes - 1));
	if (n) {
		val = _realval(n->val);
		return true;
	}
	return false;
}
