/*
Copyright (c) 2003-2009 Alberto Demichelis

This software is provided 'as-is', without any
express or implied warranty. In no event will the
authors be held liable for any damages arising from
the use of this software.

Permission is granted to anyone to use this software
for any purpose, including commercial applications,
and to alter it and redistribute it freely, subject
to the following restrictions:

		1. The origin of this software must not be
		misrepresented; you must not claim that
		you wrote the original software. If you
		use this software in a product, an
		acknowledgment in the product
		documentation would be appreciated but is
		not required.

		2. Altered source versions must be plainly
		marked as such, and must not be
		misrepresented as being the original
		software.

		3. This notice may not be removed or
		altered from any source distribution.

*/
#ifndef _SQUIRREL_H_
#define _SQUIRREL_H_

#define SQ_OK (1)
#define SQ_FAIL (0)
#define SQ_ERROR (-1)
#define SQ_FAILED(res) (res<0)
#define SQ_SUCCEEDED(res) (res>=0)

#define SQ_SUSPEND_FLAG -666
#define SQ_TAILCALL_FLAG -777
#define DONT_FALL_BACK 666
//#define EXISTS_FALL_BACK -1

#define GET_FLAG_RAW                0x00000001
#define GET_FLAG_DO_NOT_RAISE_ERROR 0x00000002

#define _SC(a) a

#define SQTrue  (1)
#define SQFalse (0)

typedef long SQInteger;
typedef unsigned long SQUnsignedInteger;
typedef short SQShort;
typedef unsigned short SQUnsignedShort;

typedef uint64 SQHash; /*should be the same size of a pointer*/

typedef float SQFloat;

typedef void* SQFunctor;

typedef SQUnsignedInteger SQBool;
typedef SQInteger SQRESULT;

typedef int ScriptDataType_t;

typedef struct SQVM* HSQUIRRELVM;
//typedef SQObject HSQOBJECT;

struct SQBufState;

typedef char SQChar;
struct SQString;

#define SQOBJECT_REF_COUNTED 0x08000000
#define SQOBJECT_NUMERIC		0x04000000
#define SQOBJECT_DELEGABLE		0x02000000
#define SQOBJECT_CANBEFALSE		0x01000000

#define SQ_MATCHTYPEMASKSTRING (-99999)

#define _RT_MASK 0x00FFFFFF
#define _RAW_TYPE(type) (type&_RT_MASK)

#define _RT_NULL			0x00000001
#define _RT_INTEGER			0x00000002
#define _RT_FLOAT			0x00000004
#define _RT_BOOL			0x00000008
#define _RT_STRING			0x00000010
#define _RT_TABLE			0x00000020
#define _RT_ARRAY			0x00000040
#define _RT_USERDATA		0x00000080
#define _RT_CLOSURE			0x00000100
#define _RT_NATIVECLOSURE	0x00000200
#define _RT_ASSET			0x00000400
#define _RT_USERPOINTER		0x00000800
#define _RT_THREAD			0x00001000
#define _RT_FUNCPROTO		0x00002000
#define _RT_CLASS			0x00004000
#define _RT_INSTANCE		0x00008000
#define _RT_WEAKREF			0x00010000
#define _RT_VECTOR			0x00040000
#define _RT_UNIMPLEMENTED	0x00080000
#define _RT_STRUCTDEF		0x00100000
#define _RT_STRUCTINSTANCE	0x00200000
#define _RT_ENTITY			0x00400000

typedef enum tagSQObjectType
{
	OT_NULL =			(_RT_NULL|SQOBJECT_CANBEFALSE),
	OT_INTEGER =		(_RT_INTEGER|SQOBJECT_NUMERIC|SQOBJECT_CANBEFALSE),
	OT_FLOAT =			(_RT_FLOAT|SQOBJECT_NUMERIC|SQOBJECT_CANBEFALSE),
	OT_BOOL =			(_RT_BOOL|SQOBJECT_CANBEFALSE),
	OT_STRING =			(_RT_STRING|SQOBJECT_REF_COUNTED),
	OT_TABLE =			(_RT_TABLE|SQOBJECT_REF_COUNTED|SQOBJECT_DELEGABLE),
	OT_ARRAY =			(_RT_ARRAY|SQOBJECT_REF_COUNTED),
	OT_VAR =			NULL,
	OT_USERDATA =		(_RT_USERDATA|SQOBJECT_REF_COUNTED|SQOBJECT_DELEGABLE),
	OT_CLOSURE =		(_RT_CLOSURE|SQOBJECT_REF_COUNTED),
	OT_NATIVECLOSURE =	(_RT_NATIVECLOSURE|SQOBJECT_REF_COUNTED),
	OT_ASSET =			(_RT_ASSET|SQOBJECT_REF_COUNTED),
	OT_USERPOINTER =	_RT_USERPOINTER,
	OT_THREAD =			(_RT_THREAD|SQOBJECT_REF_COUNTED) ,
	OT_FUNCPROTO =		(_RT_FUNCPROTO|SQOBJECT_REF_COUNTED), //internal usage only
	OT_CLASS =			(_RT_CLASS|SQOBJECT_REF_COUNTED),
	OT_WEAKREF =		(_RT_WEAKREF|SQOBJECT_REF_COUNTED),
	OT_VECTOR =			_RT_VECTOR,
	OT_UNIMPLEMENTED =	(_RT_UNIMPLEMENTED|SQOBJECT_REF_COUNTED),
	OT_STRUCTDEF =		(_RT_STRUCTDEF|SQOBJECT_REF_COUNTED),
	OT_STRUCTINSTANCE =	(_RT_STRUCTINSTANCE|SQOBJECT_REF_COUNTED),
	OT_INSTANCE =		(_RT_INSTANCE|SQOBJECT_REF_COUNTED|SQOBJECT_DELEGABLE),
	OT_ENTITY =			(_RT_ENTITY|SQOBJECT_REF_COUNTED|SQOBJECT_DELEGABLE),
} SQObjectType;

// does the type keep track of references?
#define ISREFCOUNTED(t) (t & SQOBJECT_REF_COUNTED)

typedef union tagSQObjectValue
{
	struct SQTable* pTable;
	struct SQArray* pArray;
	struct SQClosure* pClosure;
	struct SQGenerator* pGenerator;
	struct SQNativeClosure* pNativeClosure;
	struct SQString* pString;
	struct SQUserData* pUserData;
	int nInteger;
	float fFloat;
	void* pUserPointer;
	struct SQFunctionProto* pFunctionProto;
	struct SQRefCounted* pRefCounted;
	struct SQDelegable* pDelegable;
	struct SQVM* pThread;
	struct SQClass* pClass;
	struct SQInstance* pInstance;
	struct SQWeakRef* pWeakRef;
	unsigned int raw;
} SQObjectValue;

typedef struct tagSQObject
{
	SQObjectType _type;
	SQInteger _pad;
	SQObjectValue _unVal;
} SQObject;

///////////////////////////////////////////////////////////////////////////////
SQRESULT sq_pushroottable(HSQUIRRELVM v);
SQRESULT sq_getinteger(HSQUIRRELVM v, SQInteger idx, SQInteger* i);
SQRESULT sq_getfloat(HSQUIRRELVM v, SQInteger idx, SQFloat* f);
SQRESULT sq_getbool(HSQUIRRELVM v, SQInteger idx, SQBool* b);
SQRESULT sq_getthread(HSQUIRRELVM v, SQInteger idx, HSQUIRRELVM* thread);
SQRESULT sq_getstring(HSQUIRRELVM v, SQInteger idx, const SQChar** c);
SQRESULT sq_get(HSQUIRRELVM v, SQInteger idx);
SQInteger sq_gettop(HSQUIRRELVM v);
SQRESULT sq_getstackobj(HSQUIRRELVM v, SQInteger idx, SQObject* po);
void sq_pop(HSQUIRRELVM v, SQInteger nelemstopop);
SQRESULT sq_pushroottable(HSQUIRRELVM v);
void sq_pushbool(HSQUIRRELVM v, SQBool b);
void sq_pushstring(HSQUIRRELVM v, const SQChar* string, SQInteger len);
void sq_pushinteger(HSQUIRRELVM v, SQInteger val);
void sq_pushfloat(HSQUIRRELVM v, SQFloat n);
void sq_newarray(HSQUIRRELVM v, SQInteger size);
void sq_newtable(HSQUIRRELVM v);
SQRESULT sq_newslot(HSQUIRRELVM v, SQInteger idx);
SQRESULT sq_arrayappend(HSQUIRRELVM v, SQInteger idx);
SQRESULT sq_pushstructure(HSQUIRRELVM v, const SQChar* name, const SQChar* member, const SQChar* codeclass1, const SQChar* codeclass2);
SQRESULT sq_compilebuffer(HSQUIRRELVM v, SQBufState* bufferState, const SQChar* buffer, SQInteger context, SQBool raiseerror);
SQRESULT sq_call(HSQUIRRELVM v, SQInteger params, SQBool retval, SQBool raiseerror);

SQRESULT sq_startconsttable(HSQUIRRELVM v);
SQRESULT sq_endconsttable(HSQUIRRELVM v);

void sq_addref(HSQUIRRELVM v, SQObject* po);
SQBool sq_release(HSQUIRRELVM v, SQObject* po);

/*UTILITY MACRO*/
#define sq_isnumeric(o) ((o)._type&SQOBJECT_NUMERIC)
#define sq_istable(o) ((o)._type==OT_TABLE)
#define sq_isarray(o) ((o)._type==OT_ARRAY)
#define sq_isfunction(o) ((o)._type==OT_FUNCPROTO)
#define sq_isclosure(o) ((o)._type==OT_CLOSURE)
#define sq_isgenerator(o) ((o)._type==OT_GENERATOR)
#define sq_isnativeclosure(o) ((o)._type==OT_NATIVECLOSURE)
#define sq_isstring(o) ((o)._type==OT_STRING)
#define sq_isinteger(o) ((o)._type==OT_INTEGER)
#define sq_isfloat(o) ((o)._type==OT_FLOAT)
#define sq_isuserpointer(o) ((o)._type==OT_USERPOINTER)
#define sq_isuserdata(o) ((o)._type==OT_USERDATA)
#define sq_isthread(o) ((o)._type==OT_THREAD)
#define sq_isnull(o) ((o)._type==OT_NULL)
#define sq_isclass(o) ((o)._type==OT_CLASS)
#define sq_isinstance(o) ((o)._type==OT_INSTANCE)
#define sq_isbool(o) ((o)._type==OT_BOOL)
#define sq_isweakref(o) ((o)._type==OT_WEAKREF)
#define sq_type(o) ((o)._type)

/* ==== SQUIRREL ======================================================================================================================================================== */
inline SQRESULT(*v_sq_pushroottable)(HSQUIRRELVM v);
inline void(*v_sq_pushbool)(HSQUIRRELVM v, SQBool b);
inline void(*v_sq_pushstring)(HSQUIRRELVM v, const SQChar* string, SQInteger len);
inline void(*v_sq_pushinteger)(HSQUIRRELVM v, SQInteger val);
inline void(*v_sq_pushfloat)(HSQUIRRELVM v, SQFloat val);
inline void(*v_sq_newarray)(HSQUIRRELVM v, SQInteger size);
inline void(*v_sq_newtable)(HSQUIRRELVM v);
inline SQRESULT(*v_sq_newslot)(HSQUIRRELVM v, SQInteger idx);
inline SQRESULT(*v_sq_arrayappend)(HSQUIRRELVM v, SQInteger idx);
inline SQRESULT(*v_sq_pushstructure)(HSQUIRRELVM v, const SQChar* name, const SQChar* member, const SQChar* codeclass1, const SQChar* codeclass2);
inline SQRESULT(*v_sq_compilebuffer)(HSQUIRRELVM v, SQBufState* bufferstate, const SQChar* buffer, SQInteger level, SQBool raiseerror);
inline SQRESULT(*v_sq_call)(HSQUIRRELVM v, SQInteger params, SQBool retval, SQBool raiseerror);
inline SQRESULT(*v_sq_get)(HSQUIRRELVM v, SQInteger idx);

inline SQRESULT (*v_sq_startconsttable)(HSQUIRRELVM v);
inline SQRESULT (*v_sq_endconsttable)(HSQUIRRELVM v);

inline SQString* (*v_StringTable__Add)(void* a1, const SQChar* str, SQInteger len);

// returns: RefTable::RefNode*
// thisp = RefTable*
// prev = RefTable::RefNode*
inline void* (*v_RefTable__Get)(void* thisp, SQObject* obj, SQHash* mainpos, void* prev, bool add);
inline SQBool(*v_RefTable__Release)(void* thisp, SQObject* obj);

///////////////////////////////////////////////////////////////////////////////
class VSquirrelAPI : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("sq_pushroottable", v_sq_pushroottable);
		LogFunAdr("sq_pushbool", v_sq_pushbool);
		LogFunAdr("sq_pushstring", v_sq_pushstring);
		LogFunAdr("sq_pushinteger", v_sq_pushinteger);
		LogFunAdr("sq_pushfloat", v_sq_pushfloat);
		LogFunAdr("sq_newarray", v_sq_newarray);
		LogFunAdr("sq_arrayappend", v_sq_arrayappend);
		LogFunAdr("sq_newtable", v_sq_newtable);
		LogFunAdr("sq_newslot", v_sq_newslot);
		LogFunAdr("sq_pushstructure", v_sq_pushstructure);
		LogFunAdr("sq_compilebuffer", v_sq_compilebuffer);
		LogFunAdr("sq_call", v_sq_call);
		LogFunAdr("sq_get", v_sq_get);

		LogFunAdr("sq_startconsttable", v_sq_startconsttable);
		LogFunAdr("sq_endconsttable", v_sq_endconsttable);

		LogFunAdr("StringTable::Add", v_StringTable__Add);

		LogFunAdr("RefTable::Get", v_RefTable__Get);
		LogFunAdr("RefTable::Release", v_RefTable__Release);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 83 EC 28 8B 51 ?? 44 8B C2").GetPtr(v_sq_pushroottable);
		g_GameDll.FindPatternSIMD("48 83 EC 38 33 C0 48 C7 44 24 20 08 ?? ?? 01 48").GetPtr(v_sq_pushbool);
		g_GameDll.FindPatternSIMD("40 56 48 83 EC 30 48 8B F1 48 85 D2 0F 84 8F ??").GetPtr(v_sq_pushstring);
		g_GameDll.FindPatternSIMD("48 83 EC 38 33 C0 48 C7 44 24 20 02 ?? ?? 05 48").GetPtr(v_sq_pushinteger);
		g_GameDll.FindPatternSIMD("48 83 EC 38 8B 51 78 33 C0").GetPtr(v_sq_pushfloat);
		g_GameDll.FindPatternSIMD("48 89 5C 24 08 57 48 83 EC 30 48 8B D9 48 C7 44 24 20 40").GetPtr(v_sq_newarray);
		g_GameDll.FindPatternSIMD("48 89 5C 24 08 57 48 83 EC 30 48 8B D9 48 C7 44 24 20 20").GetPtr(v_sq_newtable);
		g_GameDll.FindPatternSIMD("40 53 48 83 EC 30 44 8B 49 ?? 48 8B D9 41 8B C1").GetPtr(v_sq_newslot);
		g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 8B 41 ?? 48 8B D9 2B 41 ?? 83 F8 02 7D").GetPtr(v_sq_arrayappend);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 55 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC 60 48 8B 59 60").GetPtr(v_sq_pushstructure);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 41 56 41 57 48 83 EC 50 41 8B E9 49 8B F8").GetPtr(v_sq_compilebuffer);
		g_GameDll.FindPatternSIMD("4C 8B DC 49 89 5B 08 49 89 6B 10 49 89 73 18 57 48 83 EC 50 8B F2").GetPtr(v_sq_call);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 40 48 8B F9 8B 49 78").GetPtr(v_sq_get);

		g_GameDll.FindPatternSIMD("8B 51 78 4C 8B 49 60 44 8B C2 49 C1 E0 04 4C 03 81 ?? ?? ?? ?? 8D 42 01 89 41 78 41 F7 81 ?? ?? ?? ?? ?? ?? ?? ?? 74 0A 49 8B 81 ?? ?? ?? ?? FF 40 08 41 F7 00 ?? ?? ?? ?? 41 0F 10 81 ?? ?? ?? ?? 74 15").GetPtr(v_sq_startconsttable);
		g_GameDll.FindPatternSIMD("8B 41 78 45 33 C0 FF C8 8B D0 89 41 78 48 C1 E2 04 48 03 91 ?? ?? ?? ?? 8B 02 48 C7 02 ?? ?? ?? ?? 25 ?? ?? ?? ?? 74 15").GetPtr(v_sq_endconsttable);

		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 41 8D 4D FF").FollowNearCallSelf().GetPtr(v_StringTable__Add);

		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? FF 40 10 FF C6").FollowNearCallSelf().GetPtr(v_RefTable__Get);
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 40 84 7D BC").FollowNearCallSelf().GetPtr(v_RefTable__Release);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
#endif // _SQUIRREL_H_
