#ifndef SQTYPE_H
#define SQTYPE_H

#define SQ_OK (1)
#define SQ_ERROR (-1)
#define SQ_FAILED(res) (res<0)
#define SQ_SUCCEEDED(res) (res>=0)

#define SQ_SUSPEND_FLAG -666
#define SQ_TAILCALL_FLAG -777
#define DONT_FALL_BACK 666
//#define EXISTS_FALL_BACK -1

#define GET_FLAG_RAW                0x00000001
#define GET_FLAG_DO_NOT_RAISE_ERROR 0x00000002

typedef char SQChar;
typedef float SQFloat;
typedef long SQInteger;
typedef unsigned long SQUnsignedInteger;
typedef void* SQFunctor;

typedef SQUnsignedInteger SQBool;
typedef SQInteger SQRESULT;

typedef int ScriptDataType_t;

enum class SQCONTEXT : SQInteger
{
	SERVER = 0,
	CLIENT,
	UI,
	NONE
};

constexpr const char* s_SqContext[4] =
{
	"SERVER",
	"CLIENT",
	"UI",
	"NONE"
};

constexpr const char* s_ScriptContext[4] =
{
	"Script(S):",
	"Script(C):",
	"Script(U):",
	"Script(X):"
};
#endif // SQTYPE_H
