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

typedef SQUnsignedInteger SQBool;
typedef SQInteger SQRESULT;

enum class SQCONTEXT : SQInteger
{
	SERVER = 0,
	CLIENT,
	UI,
	NONE
};

const static string SQVM_TYPE_T[4] =
{
	"SERVER",
	"CLIENT",
	"UI",
	"NONE"
};

const static string SQVM_LOG_T[4] =
{
	"Script(S):",
	"Script(C):",
	"Script(U):",
	"Script(X):"
};

const static string SQVM_ANSI_LOG_T[4] =
{
	"\033[38;2;151;149;187mScript(S):",
	"\033[38;2;151;149;163mScript(C):",
	"\033[38;2;151;123;136mScript(U):",
	"\033[38;2;151;149;163mScript(X):"
};

const static string SQVM_WARNING_ANSI_LOG_T[4] =
{
	"\033[38;2;151;149;187mScript(S):\033[38;2;255;255;000m",
	"\033[38;2;151;149;163mScript(C):\033[38;2;255;255;000m",
	"\033[38;2;151;123;136mScript(U):\033[38;2;255;255;000m",
	"\033[38;2;151;149;163mScript(X):\033[38;2;255;255;000m"
};

const static string SQVM_ERROR_ANSI_LOG_T[4] =
{
	"\033[38;2;151;149;187mScript(S):\033[38;2;255;000;000m",
	"\033[38;2;151;149;163mScript(C):\033[38;2;255;000;000m",
	"\033[38;2;151;123;136mScript(U):\033[38;2;255;000;000m",
	"\033[38;2;151;149;163mScript(X):\033[38;2;255;000;000m"
};
#endif // SQTYPE_H
