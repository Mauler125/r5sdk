#ifndef IVSCRIPT_H
#define IVSCRIPT_H

enum ScriptLanguage_t
{
	SL_NONE,
	SL_GAMEMONKEY,
	SL_SQUIRREL,
	SL_LUA,
	SL_PYTHON,

	SL_DEFAULT = SL_SQUIRREL
};

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------

DECLARE_POINTER_HANDLE(HSCRIPT);
#define INVALID_HSCRIPT ((HSCRIPT)-1)

#endif // IVSCRIPT_H
