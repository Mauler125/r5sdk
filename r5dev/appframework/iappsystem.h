//-----------------------------------------------------------------------------
// Specifies a module + interface name for initialization
//-----------------------------------------------------------------------------
struct AppSystemInfo_t
{
	const char* m_pModuleName;
	const char* m_pInterfaceName;
};

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
enum InitReturnVal_t
{
	INIT_FAILED = 0,
	INIT_OK,

	INIT_LAST_VAL,
};

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
enum AppSystemTier_t
{
	APP_SYSTEM_TIER0 = 0,
	APP_SYSTEM_TIER1,
	APP_SYSTEM_TIER2,
	APP_SYSTEM_TIER3,

	APP_SYSTEM_TIER_OTHER,
};
// NOTE: _purecall IAppSystem vtable
