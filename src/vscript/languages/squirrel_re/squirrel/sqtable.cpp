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

bool SQTable::Next(SQObjectPtr& outkey, SQObjectPtr& outval) 
{
    static SQInteger _nextidx = 0;
    if (_nextidx >= _numofnodes) {
        _nextidx = 0;
        return false;
    }
    while (_nodes[_nextidx].key._type == OT_NULL) {
        if (++_nextidx >= _numofnodes) {
            _nextidx = 0;
            return false;
        }
    }
    outkey = _nodes[_nextidx].key;
    outval = _nodes[_nextidx++].val;
    return true;
}