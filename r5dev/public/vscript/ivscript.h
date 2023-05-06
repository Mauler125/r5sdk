#ifndef IVSCRIPT_H
#define IVSCRIPT_H
#include "vscript/languages/squirrel_re/include/squirrel.h"
#include "tier1/utlvector.h"
#include "datamap.h"

//---------------------------------------------------------

DECLARE_POINTER_HANDLE(HSCRIPT);
#define INVALID_HSCRIPT ((HSCRIPT)-1)

typedef int ScriptDataType_t;
typedef void* ScriptFunctionBindingStorageType_t;

enum ScriptLanguage_t
{
	SL_NONE,
	SL_GAMEMONKEY,
	SL_SQUIRREL,
	SL_LUA,
	SL_PYTHON,

	SL_DEFAULT = SL_SQUIRREL
};

//---------------------------------------------------------

enum ExtendedFieldType
{
	FIELD_TYPEUNKNOWN = FIELD_TYPECOUNT,
	FIELD_TYPEUNKNOWN1, // Unknown field from R2
	FIELD_TYPEUNKNOWN2, // Unknown field from R5
	FIELD_CSTRING,
	FIELD_HSCRIPT,
	FIELD_VARIANT,
	FIELD_TYPEUNKNOWN3,
	FIELD_TYPEUNKNOWN4,
	FIELD_TYPEUNKNOWN5,
	FIELD_TYPEUNKNOWN6,
	FIELD_ASSET,
	FIELD_OSTRING
};

//---------------------------------------------------------

struct ScriptFuncDescriptor_t
{
	void Init(const SQChar* scriptName, const SQChar* nativeName,
		const SQChar* description, const SQChar* returnType,
		const SQChar* parameters)
	{
		m_ScriptName = scriptName;
		m_Function = nativeName;
		m_Description = description;
		m_ReturnType = returnType;
		m_Parameters = parameters;
	}

	const SQChar* m_ScriptName;
	const SQChar* m_Function;
	const SQChar* m_Description;
	const SQChar* m_ReturnType;
	const SQChar* m_Parameters;
};

//---------------------------------------------------------

struct ScriptFunctionBinding_t
{
	ScriptFuncDescriptor_t m_Descriptor;
	bool m_bCheckParams;
	bool unk29; // Last (new) parameter to 'sq_setparamscheck'

	SQInteger m_nDevLevel; // TODO: confirm, this is a guess.
	const SQChar* m_pszCodeHook;
	short unk38; // Second last (new) parameter to 'sq_setparamscheck'

	ScriptDataType_t m_ReturnType;
	CUtlVector<ScriptDataType_t> m_Parameters;
	ScriptFunctionBindingStorageType_t m_pFunction;

	void Init(
		const SQChar* scriptName, const SQChar* nativeName,
		const SQChar* helpString, const SQChar* returnString,
		const SQChar* parameters, const ScriptDataType_t returnType,
		const ScriptFunctionBindingStorageType_t function)
	{
		m_Descriptor.Init(scriptName, nativeName, helpString, returnString, parameters);
		m_bCheckParams = false;
		unk29 = false;

		m_nDevLevel = 0;
		m_pszCodeHook = nullptr;
		unk38 = 0;

		m_ReturnType = returnType;
		m_pFunction = function;
	}
};
static_assert(sizeof(ScriptFunctionBinding_t) == 0x68);

#endif // IVSCRIPT_H
