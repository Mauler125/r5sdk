/*	see copyright notice in squirrel.h */
#ifndef _SQCLASS_H_
#define _SQCLASS_H_

struct SQInstance;

struct SQClassMember {
	SQClassMember() {}
	SQClassMember(const SQClassMember& o) {
		val = o.val;
		attrs = o.attrs;
	}
	SQObjectPtr val;
	SQObjectPtr attrs;
};

typedef sqvector<SQClassMember> SQClassMemberVec;

#define MEMBER_TYPE_METHOD 0x01000000
#define MEMBER_TYPE_FIELD 0x02000000
#define MEMBER_MAX_COUNT 0x00FFFFFF

#define _ismethod(o) (_integer(o)&MEMBER_TYPE_METHOD)
#define _isfield(o) (_integer(o)&MEMBER_TYPE_FIELD)
#define _make_method_idx(i) ((SQInteger)(MEMBER_TYPE_METHOD|i))
#define _make_field_idx(i) ((SQInteger)(MEMBER_TYPE_FIELD|i))
#define _member_type(o) (_integer(o)&0xFF000000)
#define _member_idx(o) (_integer(o)&0x00FFFFFF)

struct SQClass : public CHAINABLE_OBJ
{
	// NOTE: when rebuilding NewSlot, implement the patch from commit:
	// https://github.com/albertodemichelis/squirrel/commit/23a0620658714b996d20da3d4dd1a0dcf9b0bd98

	bool Get(const SQObjectPtr& key, SQObjectPtr& val) { // TODO: untested
		if (_members->Get(key, val)) {
			if (_isfield(val)) {
				SQObjectPtr& o = _defaultvalues[_member_idx(val)].val;
				val = _realval(o);
			}
			else {
				val = _methods[_member_idx(val)].val;
			}
			return true;
		}
		return false;
	}

	SQTable* _members;
	SQClass* _base;
	SQClassMemberVec _defaultvalues;
	SQClassMemberVec _methods;
};

#endif // _SQCLASS_H_
