#ifndef SQTYPE_H
#define SQTYPE_H

#define SQ_OK (1)
#define SQ_ERROR (-1)
#define SQ_FAILED(res) (res<0)
#define SQ_SUCCEEDED(res) (res>=0)

typedef char SQChar;
typedef float SQFloat;
typedef long SQInteger;
typedef unsigned long SQUnsignedInteger;

typedef SQUnsignedInteger SQBool;
typedef SQInteger SQRESULT;

struct SQVM
{
	char pad_0000[0x8];
	SQVM* m_pSqVTable;
	// !TODO: The rest.

	SQVM* GetVTable()
	{
		return m_pSqVTable;
	}
};
typedef SQVM* HSQUIRRELVM;

struct SQBufState
{
	const SQChar* buf;
	const SQChar* bufTail;
	const SQChar* bufCopy;

	SQBufState(const std::string& code)
	{
		buf = code.c_str();
		bufTail = code.c_str() + code.size();
		bufCopy = code.c_str();
	}
};

struct SQFuncRegistration
{
	const char* m_szScriptName; // 00
	const char* m_szNativeName; // 08
	const char* m_szHelpString; // 10
	const char* m_szRetValType; // 18
	const char* m_szArgTypes;   // 20
	std::int16_t unk28;         // 28
	std::int16_t padding1;      // 2A
	std::int32_t unk2c;         // 2C
	std::int64_t unk30;         // 30
	std::int32_t unk38;         // 38
	std::int32_t padding2;      // 3C
	std::int64_t unk40;         // 40
	std::int64_t unk48;         // 48
	std::int64_t unk50;         // 50
	std::int32_t unk58;         // 58
	std::int32_t padding3;      // 5C
	void* m_pFunction;          // 60

	SQFuncRegistration()
	{
		memset(this, 0, sizeof(SQFuncRegistration));
		this->padding2 = 6;
	}
};

enum class SQCONTEXT : int
{
	SERVER = 0,
	CLIENT,
	UI,
	NONE
};

const static std::string SQVM_TYPE_T[4] =
{
	"SERVER",
	"CLIENT",
	"UI",
	"NONE"
};

const static std::string SQVM_LOG_T[4] =
{
	"Script(S):",
	"Script(C):",
	"Script(U):",
	"Script(X):"
};

const static std::string SQVM_WARNING_LOG_T[4] =
{
	"Script(S):Warning:",
	"Script(C):Warning:",
	"Script(U):Warning:",
	"Script(X):Warning:"
};

const static std::string SQVM_ANSI_LOG_T[4] =
{
	"\033[38;2;151;149;187mScript(S):",
	"\033[38;2;151;149;163mScript(C):",
	"\033[38;2;151;123;136mScript(U):",
	"\033[38;2;151;149;163mScript(X):"
};

const static std::string SQVM_WARNING_ANSI_LOG_T[4] =
{
	"\033[38;2;151;149;187mScript(S):\033[38;2;255;255;000mWarning:",
	"\033[38;2;151;149;163mScript(C):\033[38;2;255;255;000mWarning:",
	"\033[38;2;151;123;136mScript(U):\033[38;2;255;255;000mWarning:",
	"\033[38;2;151;149;163mScript(X):\033[38;2;255;255;000mWarning:"
};
#endif // SQTYPE_H
