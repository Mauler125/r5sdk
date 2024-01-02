#ifndef VSQUIRREL_H
#define VSQUIRREL_H
#include "vscript/languages/squirrel_re/include/squirrel.h"
#include "vscript/languages/squirrel_re/include/sqstate.h"
#include "vscript/languages/squirrel_re/include/sqvm.h"
#include "vscript/ivscript.h"
#include "vpc/rson.h"

#define MAX_PRECOMPILED_SCRIPTS 1024

#pragma pack(push, 4)
class CSquirrelVM
{
public:
	static bool Init(CSquirrelVM* s, SQCONTEXT context, float curtime);
	static bool DestroySignalEntryListHead(CSquirrelVM* s, HSQUIRRELVM v, SQFloat f);

	void CompileModScripts();
	void SetAsCompiler(RSON::Node_t* rson);

	SQRESULT RegisterFunction(const SQChar* scriptname, const SQChar* nativename, const SQChar* helpstring, const SQChar* returntype, const SQChar* parameters, void* functor);
	SQRESULT RegisterConstant(const SQChar* name, SQInteger value);

	FORCEINLINE HSQUIRRELVM GetVM() const { return m_sqVM; }
	FORCEINLINE SQCONTEXT GetContext() const { return m_iContext; }

private:
	SQChar pad0[0x8];
	HSQUIRRELVM m_sqVM;
	SQChar pad1[0x8];
	SQInteger m_nFlags;
	SQChar pad2[4];
	SQChar pad3[16];
	SQChar pad4[4];
	SQInteger m_nTick;
	SQCONTEXT m_iContext; // 0x38
	void* m_pCompareFunc;
};
#pragma pack(pop)

extern void(*ServerScriptRegister_Callback)(CSquirrelVM* s);
extern void(*ClientScriptRegister_Callback)(CSquirrelVM* s);
extern void(*UiScriptRegister_Callback)(CSquirrelVM* s);

extern void(*CoreServerScriptRegister_Callback)(CSquirrelVM* s);
extern void(*AdminPanelScriptRegister_Callback)(CSquirrelVM* s);

extern void(*ScriptConstantRegister_Callback)(CSquirrelVM* s);

inline bool(*CSquirrelVM__Init)(CSquirrelVM* s, SQCONTEXT context, SQFloat curtime);
inline bool(*CSquirrelVM__DestroySignalEntryListHead)(CSquirrelVM* s, HSQUIRRELVM v, SQFloat f);
inline SQRESULT(*CSquirrelVM__RegisterFunction)(CSquirrelVM* s, ScriptFunctionBinding_t* binding, SQInteger a1);
inline SQRESULT(*CSquirrelVM__RegisterConstant)(CSquirrelVM* s, const SQChar* name, SQInteger value);

#ifndef DEDICATED
inline bool(*CSquirrelVM__PrecompileClientScripts)(CSquirrelVM* vm, SQCONTEXT context, char** scriptArray, int scriptCount);
#endif

#ifndef CLIENT_DLL
inline bool(*CSquirrelVM__PrecompileServerScripts)(CSquirrelVM* vm, SQCONTEXT context, char** scriptArray, int scriptCount);
#endif

#ifndef CLIENT_DLL
inline CSquirrelVM* g_pServerScript;
#endif // !CLIENT_DLL

#ifndef DEDICATED
inline CSquirrelVM* g_pClientScript;
inline CSquirrelVM* g_pUIScript;
#endif // !DEDICATED


///////////////////////////////////////////////////////////////////////////////
class VSquirrel : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CSquirrelVM::Init", CSquirrelVM__Init);
		LogFunAdr("CSquirrelVM::DestroySignalEntryListHead", CSquirrelVM__DestroySignalEntryListHead);

		LogFunAdr("CSquirrelVM::RegisterConstant", CSquirrelVM__RegisterConstant);
		LogFunAdr("CSquirrelVM::RegisterFunction", CSquirrelVM__RegisterFunction);
#ifndef CLIENT_DLL
		LogFunAdr("CSquirrelVM::PrecompileServerScripts", CSquirrelVM__PrecompileServerScripts);
#endif // !CLIENT_DLL
#ifndef DEDICATED
		LogFunAdr("CSquirrelVM::PrecompileClientScripts", CSquirrelVM__PrecompileClientScripts);
#endif // !DEDICATED
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 0F 28 74 24 ?? 48 89 1D ?? ?? ?? ??").FollowNearCallSelf().GetPtr(CSquirrelVM__Init);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 56 57 41 56 48 83 EC 50 44 8B 42").GetPtr(CSquirrelVM__DestroySignalEntryListHead);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC 30 4C 8B").GetPtr(CSquirrelVM__RegisterConstant);
		g_GameDll.FindPatternSIMD("48 83 EC 38 45 0F B6 C8").GetPtr(CSquirrelVM__RegisterFunction);

#ifndef CLIENT_DLL
		// sv scripts.rson compiling
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 0F B6 F0 48 85 DB").FollowNearCallSelf().GetPtr(CSquirrelVM__PrecompileServerScripts);
#endif

#ifndef DEDICATED
		// cl/ui scripts.rson compiling
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 44 0F B6 F0 48 85 DB").FollowNearCallSelf().GetPtr(CSquirrelVM__PrecompileClientScripts);
#endif
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};

#endif // VSQUIRREL_H
