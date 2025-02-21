//=============================================================================//
//
// Purpose: Squirrel API interface to engine
//
//=============================================================================//

#include "core/stdafx.h"
#include "squirrel.h"
#include "sqvm.h"
#include "sqarray.h"
#include "sqstring.h"
#include "sqtable.h"

//---------------------------------------------------------------------------------
bool sq_aux_gettypedarg(HSQUIRRELVM v, SQInteger idx, SQObjectType type, SQObjectPtr** o)
{
	*o = &stack_get(v, idx);
	if (sq_type(**o) != type) {
		SQObjectPtr oval;
		v->PrintObjVal(*o, &oval);
		v_SQVM_RaiseError(v, _SC("wrong argument type, expected '%s' got '%.50s'"), IdType2Name(type), _stringval(oval));
		return false;
	}
	return true;
}

//---------------------------------------------------------------------------------
#define _GETSAFE_OBJ(v,idx,type,o) { if(!sq_aux_gettypedarg(v,idx,type,&o)) return SQ_ERROR; }

#define sq_aux_paramscheck(v,count) \
{ \
	if(sq_gettop(v) < count){ v_SQVM_RaiseError(v, _SC("not enough params in the stack")); return SQ_ERROR; }\
}

//---------------------------------------------------------------------------------
SQRESULT sq_getinteger(HSQUIRRELVM v, SQInteger idx, SQInteger* i)
{
	SQObjectPtr& o = stack_get(v, idx);
	if (sq_isnumeric(o)) {
		*i = tointeger(o);
		return SQ_OK;
	}
	return SQ_ERROR;
}

//---------------------------------------------------------------------------------
SQRESULT sq_getfloat(HSQUIRRELVM v, SQInteger idx, SQFloat* f)
{
	SQObjectPtr& o = stack_get(v, idx);
	if (sq_isnumeric(o)) {
		*f = tofloat(o);
		return SQ_OK;
	}
	return SQ_ERROR;
}

//---------------------------------------------------------------------------------
SQRESULT sq_getbool(HSQUIRRELVM v, SQInteger idx, SQBool* b)
{
	SQObjectPtr& o = stack_get(v, idx);
	if (sq_isbool(o)) {
		*b = _integer(o);
		return SQ_OK;
	}
	return SQ_ERROR;
}

//---------------------------------------------------------------------------------
SQRESULT sq_getthread(HSQUIRRELVM v, SQInteger idx, HSQUIRRELVM* thread)
{
	SQObjectPtr* o = NULL;
	_GETSAFE_OBJ(v, idx, OT_THREAD, o);
	*thread = _thread(*o);
	return SQ_OK;
}

//---------------------------------------------------------------------------------
SQRESULT sq_getstring(HSQUIRRELVM v, SQInteger idx, const SQChar** c)
{
	SQObjectPtr* o = NULL;
	_GETSAFE_OBJ(v, idx, OT_STRING, o);
	*c = _stringval(*o);
	return SQ_OK;
}

//---------------------------------------------------------------------------------
SQRESULT sq_get(HSQUIRRELVM v, SQInteger idx)
{
	return v_sq_get(v, idx);
}

//---------------------------------------------------------------------------------
SQInteger sq_gettop(HSQUIRRELVM v)
{
	return (v->_top - v->_bottom);
}

//---------------------------------------------------------------------------------
SQRESULT sq_getstackobj(HSQUIRRELVM v, SQInteger idx, SQObject* po)
{
	*po = stack_get(v, idx);
	return SQ_OK;
}

//---------------------------------------------------------------------------------
void sq_pop(HSQUIRRELVM v, SQInteger nelemstopop)
{
	Assert(v->_top >= nelemstopop);
	v->Pop(nelemstopop);
}

//---------------------------------------------------------------------------------
SQRESULT sq_pushroottable(HSQUIRRELVM v)
{
	v->Push(v->_roottable);

	return SQ_OK;
}

//---------------------------------------------------------------------------------
void sq_pushbool(HSQUIRRELVM v, SQBool b)
{
	v->Push(b?true:false);
}

//---------------------------------------------------------------------------------
void sq_pushstring(HSQUIRRELVM v, const SQChar* s, SQInteger len)
{
	if (s)
	{
		SQString* pString = SQString::Create(v->_sharedstate, s, len);
		v->Push(pString);
	}
	else
		v->Push(_null_);
}

//---------------------------------------------------------------------------------
void sq_pushinteger(HSQUIRRELVM v, SQInteger val)
{
	v->Push(val);
}

//---------------------------------------------------------------------------------
void sq_pushfloat(HSQUIRRELVM v, SQFloat n)
{
	v->Push(n);
}

//---------------------------------------------------------------------------------
void sq_newarray(HSQUIRRELVM v, SQInteger size)
{
	v_sq_newarray(v, size);
}

//---------------------------------------------------------------------------------
void sq_newtable(HSQUIRRELVM v)
{
	v_sq_newtable(v);
}

//---------------------------------------------------------------------------------
SQRESULT sq_newslot(HSQUIRRELVM v, SQInteger idx)
{
	return v_sq_newslot(v, idx);
}

//---------------------------------------------------------------------------------
SQRESULT sq_arrayappend(HSQUIRRELVM v, SQInteger idx)
{
	return v_sq_arrayappend(v, idx);
}

//---------------------------------------------------------------------------------
SQRESULT sq_pushstructure(HSQUIRRELVM v, const SQChar* name, const SQChar* member, const SQChar* codeclass1, const SQChar* codeclass2)
{
	return v_sq_pushstructure(v, name, member, codeclass1, codeclass2);
}

//---------------------------------------------------------------------------------
SQRESULT sq_compilebuffer(HSQUIRRELVM v, SQBufState* bufferState, const SQChar* buffer, SQInteger level, SQBool raiseerror)
{
	return v_sq_compilebuffer(v, bufferState, buffer, level, raiseerror);
}

//---------------------------------------------------------------------------------
SQRESULT sq_call(HSQUIRRELVM v, SQInteger params, SQBool retval, SQBool raiseerror)
{
	return v_sq_call(v, params, retval, raiseerror);
}

//---------------------------------------------------------------------------------
SQRESULT sq_startconsttable(HSQUIRRELVM v)
{
	return v_sq_startconsttable(v);
}

//---------------------------------------------------------------------------------
SQRESULT sq_endconsttable(HSQUIRRELVM v)
{
	return v_sq_endconsttable(v);
}

//---------------------------------------------------------------------------------
void sq_addref(HSQUIRRELVM v, SQObject* po)
{
	if (!ISREFCOUNTED(sq_type(*po))) return;
	_ss(v)->_refs_table.AddRef(*po);
}

//---------------------------------------------------------------------------------
SQBool sq_release(HSQUIRRELVM v, SQObject* po)
{
	if (!ISREFCOUNTED(sq_type(*po))) return SQTrue;
	return _ss(v)->_refs_table.Release(*po);
}

//---------------------------------------------------------------------------------
void sq_pushuserpointer(HSQUIRRELVM v, void* p)
{
	SQObjectPtr obj;
	obj._type = OT_USERPOINTER;
	obj._unVal.pUserPointer = p;
	v->Push(obj);
}

//---------------------------------------------------------------------------------
void sq_pushnull(HSQUIRRELVM v) 
{
	v->Push(_null_);
}

//---------------------------------------------------------------------------------
SQObjectType sq_gettype(HSQUIRRELVM v, SQInteger idx) 
{
	return sq_type(stack_get(v, idx));
}


//---------------------------------------------------------------------------------
SQRESULT sq_next(HSQUIRRELVM v, SQInteger idx) 
{
	SQObjectPtr& o = stack_get(v, idx);
	if (o._type == OT_TABLE) 
	{
		SQTable* table = _table(o);
		SQObjectPtr key, value;
		
		if (table->Next(key, value)) 
		{
			v->Push(key);
			v->Push(value);
			return SQ_OK;
		}
		return SQ_ERROR;
	}
	else if (o._type == OT_ARRAY)
	{
		SQArray* arr = _array(o);
		SQInteger arrSize = (SQInteger)arr->Size();

		SQInteger keyIndex = (v->_top - 1);

		if (keyIndex < 0)
			return SQ_ERROR;

		SQObjectPtr& currentKeyObj = stack_get(v, keyIndex);
		SQInteger nextIndex = 0;

		if (sq_type(currentKeyObj) == OT_NULL)
			nextIndex = 0;
		else if (sq_type(currentKeyObj) == OT_INTEGER)
		{
			SQInteger i = _integer(currentKeyObj);
			nextIndex = i + 1;
		}
		else
			return SQ_ERROR;

		if (nextIndex < arrSize)
		{
			SQObjectPtr val;
			bool gotVal = arr->Get(nextIndex, val);

			if (!gotVal)
				return SQ_ERROR;

			v->Push(SQObjectPtr(nextIndex));
			v->Push(val);
			return SQ_OK;
		}
		else
			return SQ_ERROR;
	}

	return SQ_ERROR;
}

SQRESULT sq_getarraysize(HSQUIRRELVM v, SQInteger idx, SQInteger* outSize) 
{
	SQObject obj = stack_get(v, idx);
	if (sq_type(obj) != OT_ARRAY)
		return SQ_ERROR;
	SQArray* arr = _array(obj);
	*outSize = (SQInteger)arr->Size();
	return SQ_OK;
}

SQInteger sq_absindex(HSQUIRRELVM v, SQInteger idx)
{
	if (idx >= 0)
		return idx;

	SQInteger absIdx = sq_gettop(v) + idx + 1;
	return ( absIdx > 0 ) ? absIdx : 0;
}

void VSquirrelAPI::Detour(const bool bAttach) const
{
	DetourSetup(&v_sq_pushroottable, &sq_pushroottable, bAttach);
	//DetourSetup(&v_sq_pushbool, &sq_pushbool, bAttach);
	//DetourSetup(&v_sq_pushstring, &sq_pushstring, bAttach);
	//DetourSetup(&v_sq_pushinteger, &sq_pushinteger, bAttach);
	//DetourSetup(&v_sq_pushfloat, &sq_pushfloat, bAttach);
	DetourSetup(&v_sq_newarray, &sq_newarray, bAttach);
	DetourSetup(&v_sq_newtable, &sq_newtable, bAttach);
	DetourSetup(&v_sq_newslot, &sq_newslot, bAttach);
	DetourSetup(&v_sq_arrayappend, &sq_arrayappend, bAttach);
	DetourSetup(&v_sq_pushstructure, &sq_pushstructure, bAttach);
	DetourSetup(&v_sq_compilebuffer, &sq_compilebuffer, bAttach);
	DetourSetup(&v_sq_call, &sq_call, bAttach);

	DetourSetup(&v_sq_startconsttable, &sq_startconsttable, bAttach);
	DetourSetup(&v_sq_endconsttable, &sq_endconsttable, bAttach);
}
