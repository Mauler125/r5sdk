#pragma once

/* Enumerations */
enum class D3D11DeviceVTbl : short
{
    // IUnknown
    QueryInterface                       = 0,
    AddRef                               = 1,
    Release                              = 2,

    // ID3D11Device
    CreateBuffer                         = 3,
    CreateTexture1D                      = 4,
    CreateTexture2D                      = 5,
    CreateTexture3D                      = 6,
    CreateShaderResourceView             = 7,
    CreateUnorderedAccessView            = 8,
    CreateRenderTargetView               = 9,
    CreateDepthStencilView               = 10,
    CreateInputLayout                    = 11,
    CreateVertexShader                   = 12,
    CreateGeometryShader                 = 13,
    CreateGeometryShaderWithStreamOutput = 14,
    CreatePixelShader                    = 15,
    CreateHullShader                     = 16,
    CreateDomainShader                   = 17,
    CreateComputeShader                  = 18,
    CreateClassLinkage                   = 19,
    CreateBlendState                     = 20,
    CreateDepthStencilState              = 21,
    CreateRasterizerState                = 22,
    CreateSamplerState                   = 23,
    CreateQuery                          = 24,
    CreatePredicate                      = 25,
    CreateCounter                        = 26,
    CreateDeferredContext                = 27,
    OpenSharedResource                   = 28,
    CheckFormatSupport                   = 29,
    CheckMultisampleQualityLevels        = 30,
    CheckCounterInfo                     = 31,
    CheckCounter                         = 32,
    CheckFeatureSupport                  = 33,
    GetPrivateData                       = 34,
    SetPrivateData                       = 35,
    SetPrivateDataInterface              = 36,
    GetFeatureLevel                      = 37,
    GetCreationFlags                     = 38,
    GetDeviceRemovedReason               = 39,
    GetImmediateContext                  = 40,
    SetExceptionMode                     = 41,
    GetExceptionMode                     = 42,
};

enum class DXGISwapChainVTbl : short
{
    // IUnknown
    QueryInterface                       = 0,
    AddRef                               = 1,
    Release                              = 2,

    // IDXGIObject
    SetPrivateData                       = 3,
    SetPrivateDataInterface              = 4,
    GetPrivateData                       = 5,
    GetParent                            = 6,

    // IDXGIDeviceSubObject
    GetDevice                            = 7,

    // IDXGISwapChain
    Present                              = 8,
    GetBuffer                            = 9,
    SetFullscreenState                   = 10,
    GetFullscreenState                   = 11,
    GetDesc                              = 12,
    ResizeBuffers                        = 13,
    ResizeTarget                         = 14,
    GetContainingOutput                  = 15,
    GetFrameStatistics                   = 16,
    GetLastPresentCount                  = 17,
};

#define MAX_SPLITSCREEN_CLIENT_BITS 2
#define MAX_SPLITSCREEN_CLIENTS	( 1 << MAX_SPLITSCREEN_CLIENT_BITS ) // 4

enum
{
	MAX_JOYSTICKS = MAX_SPLITSCREEN_CLIENTS,
	MOUSE_BUTTON_COUNT = 5,
};

enum JoystickAxis_t
{
	JOY_AXIS_X = 0,
	JOY_AXIS_Y,
	JOY_AXIS_Z,
	JOY_AXIS_R,
	JOY_AXIS_U,
	JOY_AXIS_V,
	MAX_JOYSTICK_AXES,
};

enum
{
	JOYSTICK_MAX_BUTTON_COUNT = 32,
	JOYSTICK_POV_BUTTON_COUNT = 4,
	JOYSTICK_AXIS_BUTTON_COUNT = MAX_JOYSTICK_AXES * 2,
};

#define JOYSTICK_BUTTON_INTERNAL( _joystick, _button ) ( JOYSTICK_FIRST_BUTTON + ((_joystick) * JOYSTICK_MAX_BUTTON_COUNT) + (_button) )
#define JOYSTICK_POV_BUTTON_INTERNAL( _joystick, _button ) ( JOYSTICK_FIRST_POV_BUTTON + ((_joystick) * JOYSTICK_POV_BUTTON_COUNT) + (_button) )
#define JOYSTICK_AXIS_BUTTON_INTERNAL( _joystick, _button ) ( JOYSTICK_FIRST_AXIS_BUTTON + ((_joystick) * JOYSTICK_AXIS_BUTTON_COUNT) + (_button) )

#define JOYSTICK_BUTTON( _joystick, _button ) ( (ButtonCode_t)JOYSTICK_BUTTON_INTERNAL( _joystick, _button ) )
#define JOYSTICK_POV_BUTTON( _joystick, _button ) ( (ButtonCode_t)JOYSTICK_POV_BUTTON_INTERNAL( _joystick, _button ) )
#define JOYSTICK_AXIS_BUTTON( _joystick, _button ) ( (ButtonCode_t)JOYSTICK_AXIS_BUTTON_INTERNAL( _joystick, _button ) )

enum ButtonCode_t
{
	BUTTON_CODE_INVALID = -1,
	BUTTON_CODE_NONE = 0,

	KEY_FIRST = 0,

	KEY_NONE = KEY_FIRST,
	KEY_0,
	KEY_1,
	KEY_2,
	KEY_3,
	KEY_4,
	KEY_5,
	KEY_6,
	KEY_7,
	KEY_8,
	KEY_9,
	KEY_A,
	KEY_B,
	KEY_C,
	KEY_D,
	KEY_E,
	KEY_F,
	KEY_G,
	KEY_H,
	KEY_I,
	KEY_J,
	KEY_K,
	KEY_L,
	KEY_M,
	KEY_N,
	KEY_O,
	KEY_P,
	KEY_Q,
	KEY_R,
	KEY_S,
	KEY_T,
	KEY_U,
	KEY_V,
	KEY_W,
	KEY_X,
	KEY_Y,
	KEY_Z,
	KEY_PAD_0,
	KEY_PAD_1,
	KEY_PAD_2,
	KEY_PAD_3,
	KEY_PAD_4,
	KEY_PAD_5,
	KEY_PAD_6,
	KEY_PAD_7,
	KEY_PAD_8,
	KEY_PAD_9,
	KEY_PAD_DIVIDE,
	KEY_PAD_MULTIPLY,
	KEY_PAD_MINUS,
	KEY_PAD_PLUS,
	KEY_PAD_ENTER,
	KEY_PAD_DECIMAL,
	KEY_LBRACKET,
	KEY_RBRACKET,
	KEY_SEMICOLON,
	KEY_APOSTROPHE,
	KEY_BACKQUOTE,
	KEY_COMMA,
	KEY_PERIOD,
	KEY_SLASH,
	KEY_BACKSLASH,
	KEY_MINUS,
	KEY_EQUAL,
	KEY_ENTER,
	KEY_SPACE,
	KEY_BACKSPACE,
	KEY_TAB,
	KEY_CAPSLOCK,
	KEY_NUMLOCK,
	KEY_ESCAPE,
	KEY_SCROLLLOCK,
	KEY_INSERT,
	KEY_DELETE,
	KEY_HOME,
	KEY_END,
	KEY_PAGEUP,
	KEY_PAGEDOWN,
	KEY_BREAK,
	KEY_LSHIFT,
	KEY_RSHIFT,
	KEY_LALT,
	KEY_RALT,
	KEY_LCONTROL,
	KEY_RCONTROL,
	KEY_LWIN,
	KEY_RWIN,
	KEY_APP,
	KEY_UP,
	KEY_LEFT,
	KEY_DOWN,
	KEY_RIGHT,
	KEY_F1,
	KEY_F2,
	KEY_F3,
	KEY_F4,
	KEY_F5,
	KEY_F6,
	KEY_F7,
	KEY_F8,
	KEY_F9,
	KEY_F10,
	KEY_F11,
	KEY_F12,
	KEY_CAPSLOCKTOGGLE,
	KEY_NUMLOCKTOGGLE,
	KEY_SCROLLLOCKTOGGLE,

	KEY_LAST = KEY_SCROLLLOCKTOGGLE,
	KEY_COUNT = KEY_LAST - KEY_FIRST + 1,

	// Mouse
	MOUSE_FIRST = KEY_LAST + 1,

	MOUSE_LEFT = MOUSE_FIRST,
	MOUSE_RIGHT,
	MOUSE_MIDDLE,
	MOUSE_4,
	MOUSE_5,
	MOUSE_WHEEL_UP,		// A fake button which is 'pressed' and 'released' when the wheel is moved up
	MOUSE_WHEEL_DOWN,	// A fake button which is 'pressed' and 'released' when the wheel is moved down

	MOUSE_LAST = MOUSE_WHEEL_DOWN,
	MOUSE_COUNT = MOUSE_LAST - MOUSE_FIRST + 1,

	// Joystick
	JOYSTICK_FIRST = MOUSE_LAST + 1,

	JOYSTICK_FIRST_BUTTON = JOYSTICK_FIRST,
	JOYSTICK_LAST_BUTTON = JOYSTICK_BUTTON_INTERNAL(MAX_JOYSTICKS - 1, JOYSTICK_MAX_BUTTON_COUNT - 1),
	JOYSTICK_FIRST_POV_BUTTON,
	JOYSTICK_LAST_POV_BUTTON = JOYSTICK_POV_BUTTON_INTERNAL(MAX_JOYSTICKS - 1, JOYSTICK_POV_BUTTON_COUNT - 1),
	JOYSTICK_FIRST_AXIS_BUTTON,
	JOYSTICK_LAST_AXIS_BUTTON = JOYSTICK_AXIS_BUTTON_INTERNAL(MAX_JOYSTICKS - 1, JOYSTICK_AXIS_BUTTON_COUNT - 1),

	JOYSTICK_LAST = JOYSTICK_LAST_AXIS_BUTTON,

	BUTTON_CODE_LAST,
	BUTTON_CODE_COUNT = BUTTON_CODE_LAST - KEY_FIRST + 1,

	// Helpers for XBox 360
	KEY_XBUTTON_UP = JOYSTICK_FIRST_POV_BUTTON,	// POV buttons
	KEY_XBUTTON_RIGHT,
	KEY_XBUTTON_DOWN,
	KEY_XBUTTON_LEFT,

	KEY_XBUTTON_A = JOYSTICK_FIRST_BUTTON,		// Buttons
	KEY_XBUTTON_B,
	KEY_XBUTTON_X,
	KEY_XBUTTON_Y,
	KEY_XBUTTON_LEFT_SHOULDER,
	KEY_XBUTTON_RIGHT_SHOULDER,
	KEY_XBUTTON_BACK,
	KEY_XBUTTON_START,
	KEY_XBUTTON_STICK1,
	KEY_XBUTTON_STICK2,
	KEY_XBUTTON_INACTIVE_START,

	KEY_XSTICK1_RIGHT = JOYSTICK_FIRST_AXIS_BUTTON,	// XAXIS POSITIVE
	KEY_XSTICK1_LEFT,							// XAXIS NEGATIVE
	KEY_XSTICK1_DOWN,							// YAXIS POSITIVE
	KEY_XSTICK1_UP,								// YAXIS NEGATIVE
	KEY_XBUTTON_LTRIGGER,						// ZAXIS POSITIVE
	KEY_XBUTTON_RTRIGGER,						// ZAXIS NEGATIVE
	KEY_XSTICK2_RIGHT,							// UAXIS POSITIVE
	KEY_XSTICK2_LEFT,							// UAXIS NEGATIVE
	KEY_XSTICK2_DOWN,							// VAXIS POSITIVE
	KEY_XSTICK2_UP,								// VAXIS NEGATIVE
};

// Buttons are not confirmed to be the same. They have been always the same throughout the source engine. Lets hope they did not change them.

enum KeyValuesTypes
{
	TYPE_NONE = 0x0,
	TYPE_STRING = 0x1,
	TYPE_INT = 0x2,
	TYPE_FLOAT = 0x3,
	TYPE_PTR = 0x4,
	TYPE_WSTRING = 0x5,
	TYPE_COLOR = 0x6,
	TYPE_UINT64 = 0x7,
	TYPE_COMPILED_INT_BYTE = 0x8,
	TYPE_COMPILED_INT_0 = 0x9,
	TYPE_COMPILED_INT_1 = 0xA,
	TYPE_NUMTYPES = 0xB,
};

enum ClientFrameStage_t
{
	FRAME_UNDEFINED = -1,			// (haven't run any frames yet)
	FRAME_START,

	// A network packet is being recieved
	FRAME_NET_UPDATE_START,
	// Data has been received and we're going to start calling PostDataUpdate
	FRAME_NET_UPDATE_POSTDATAUPDATE_START,
	// Data has been received and we've called PostDataUpdate on all data recipients
	FRAME_NET_UPDATE_POSTDATAUPDATE_END,
	// We've received all packets, we can now do interpolation, prediction, etc..
	FRAME_NET_UPDATE_END,

	// We're about to start rendering the scene
	FRAME_RENDER_START,
	// We've finished rendering the scene.
	FRAME_RENDER_END,

	FRAME_NET_FULL_FRAME_UPDATE_ON_REMOVE
};

enum class HostStates_t : int
{
	HS_NEW_GAME = 0x0,
	HS_LOAD_GAME = 0x1,
	HS_CHANGE_LEVEL_SP = 0x2,
	HS_CHANGE_LEVEL_MP = 0x3,
	HS_RUN = 0x4,
	HS_GAME_SHUTDOWN = 0x5,
	HS_SHUTDOWN = 0x6,
	HS_RESTART = 0x7,
};

enum SIGNONSTATE
{
	SIGNONSTATE_NONE = 0, // no state yet; about to connect
	SIGNONSTATE_CHALLENGE = 1, // client challenging server; all OOB packets
	SIGNONSTATE_CONNECTED = 2, // client is connected to server; netchans ready
	SIGNONSTATE_NEW = 3, // just got serverinfo and string tables
	SIGNONSTATE_PRESPAWN = 4, // received signon buffers
	SIGNONSTATE_GETTING_DATA = 5, // getting persistence data I assume?
	SIGNONSTATE_SPAWN = 6, // ready to receive entity packets
	SIGNONSTATE_FIRST_SNAP = 7, // ???
	SIGNONSTATE_FULL = 8, // we are fully connected; first non-delta packet received
	SIGNONSTATE_CHANGELEVEL = 9, // server is changing level; please wait
};

enum FileWarningLevel_t
{
	FILESYSTEM_WARNING = -1,
	FILESYSTEM_WARNING_QUIET = 0,
	FILESYSTEM_WARNING_REPORTUNCLOSED,
	FILESYSTEM_WARNING_REPORTUSAGE,
	FILESYSTEM_WARNING_REPORTALLACCESSES,
	FILESYSTEM_WARNING_REPORTALLACCESSES_READ,
	FILESYSTEM_WARNING_REPORTALLACCESSES_READWRITE,
	FILESYSTEM_WARNING_REPORTALLACCESSES_ASYNC
};

#define FCVAR_NONE				0 

// Command to ConVars and ConCommands
// ConVar Systems
#define FCVAR_UNREGISTERED		(1<<0)	// If this is set, don't add to linked list, etc.
#define FCVAR_DEVELOPMENTONLY	(1<<1)	// Hidden in released products. Flag is removed automatically if ALLOW_DEVELOPMENT_CVARS is defined.
#define FCVAR_GAMEDLL			(1<<2)	// defined by the game DLL
#define FCVAR_CLIENTDLL			(1<<3)  // defined by the client DLL
#define FCVAR_HIDDEN			(1<<4)	// Hidden. Doesn't appear in find or auto complete. Like DEVELOPMENTONLY, but can't be compiled out.

// ConVar only
#define FCVAR_PROTECTED			(1<<5)  // It's a server cvar, but we don't send the data since it's a password, etc.  Sends 1 if it's not bland/zero, 0 otherwise as value
#define FCVAR_SPONLY			(1<<6)  // This cvar cannot be changed by clients connected to a multiplayer server.
#define	FCVAR_ARCHIVE			(1<<7)	// set to cause it to be saved to vars.rc
#define	FCVAR_NOTIFY			(1<<8)	// notifies players when changed
#define	FCVAR_USERINFO			(1<<9)	// changes the client's info string

#define FCVAR_PRINTABLEONLY		(1<<10)  // This cvar's string cannot contain unprintable characters ( e.g., used for player name etc ).

#define FCVAR_GAMEDLL_FOR_REMOTE_CLIENTS		(1<<10)  // When on concommands this allows remote clients to execute this cmd on the server. 
														 // We are changing the default behavior of concommands to disallow execution by remote clients without
														 // this flag due to the number existing concommands that can lag or crash the server when clients abuse them.

#define FCVAR_UNLOGGED			(1<<11)  // If this is a FCVAR_SERVER, don't log changes to the log file / console if we are creating a log
#define FCVAR_NEVER_AS_STRING	(1<<12)  // never try to print that cvar

// It's a ConVar that's shared between the client and the server.
// At signon, the values of all such ConVars are sent from the server to the client (skipped for local
//  client, of course )
// If a change is requested it must come from the console (i.e., no remote client changes)
// If a value is changed while a server is active, it's replicated to all connected clients
#define FCVAR_REPLICATED		(1<<13)	// server setting enforced on clients, TODO rename to FCAR_SERVER at some time
#define FCVAR_CHEAT				(1<<14) // Only useable in singleplayer / debug / multiplayer & sv_cheats
#define FCVAR_SS				(1<<15) // causes varnameN where N == 2 through max splitscreen slots for mod to be autogenerated
#define FCVAR_DEMO				(1<<16) // record this cvar when starting a demo file
#define FCVAR_DONTRECORD		(1<<17) // don't record these command in demofiles
#define FCVAR_SS_ADDED			(1<<18) // This is one of the "added" FCVAR_SS variables for the splitscreen players
#define FCVAR_RELEASE			(1<<19) // Cvars tagged with this are the only cvars avaliable to customers
#define FCVAR_RELOAD_MATERIALS	(1<<20)	// If this cvar changes, it forces a material reload
#define FCVAR_RELOAD_TEXTURES	(1<<21)	// If this cvar changes, if forces a texture reload

#define FCVAR_NOT_CONNECTED		(1<<22)	// cvar cannot be changed by a client that is connected to a server
#define FCVAR_MATERIAL_SYSTEM_THREAD (1<<23)	// Indicates this cvar is read from the material system thread
#define FCVAR_ARCHIVE_GAMECONSOLE	(1<<24) // cvar written to config.cfg on the Xbox

#define FCVAR_SERVER_CAN_EXECUTE	(1<<28)// the server is allowed to execute this command on clients via ClientCommand/NET_StringCmd/CBaseClientState::ProcessStringCmd.
#define FCVAR_SERVER_CANNOT_QUERY	(1<<29)// If this is set, then the server is not allowed to query this cvar's value (via IServerPluginHelpers::StartQueryCvarValue).
#define FCVAR_CLIENTCMD_CAN_EXECUTE	(1<<30)	// IVEngineClient::ClientCmd is allowed to execute this command.

#define MAX_PLAYERS 128 // Max R5 players.

/// \ingroup error
#define ORIGIN_ERROR						0xA0000000				///< Add this to your error code to get an error. Bit 29 is set to distinguish Origin error codes from system error codes. 

/// \ingroup error
#define ORIGIN_WARNING					0x40000000				///< Add this to your warning code to get a warning. Bit 29 is set to distinguish Origin warning codes from system warnings codes.

/// \ingroup error
#define ORIGIN_ERROR_AREA_GENERAL		0x00000000				///< Add this to your error to get a general error.

/// \ingroup error
#define ORIGIN_ERROR_AREA_SDK			(1<<16)					///< Add this to your error to get an SDK error.

/// \ingroup error
#define ORIGIN_ERROR_AREA_CORE			(2<<16)					///< Add this to your error to get a core error.

/// \ingroup error
#define ORIGIN_ERROR_AREA_IGO			(3<<16)					///< Add this to your error to get an IGO error.

/// \ingroup error
#define ORIGIN_ERROR_AREA_FRIENDS		(4<<16)					///< Add this to your error to get a friends error.

/// \ingroup error
#define ORIGIN_ERROR_AREA_PRESENCE		(5<<16)					///< Add this to your error to get a presence error.

/// \ingroup error
#define ORIGIN_ERROR_AREA_COMMERCE		(6<<16)					///< Add this to your error to get a commerce error.

/// \ingroup error
#define ORIGIN_ERROR_AREA_ACHIEVEMENTS	(7<<16)					///< Add this to your error to get a achievement error.

/// \ingroup error
#define ORIGIN_ERROR_AREA_LSX			(8<<16)					///< Add this to your error to get an LSX error.

/// \ingroup error
#define ORIGIN_ERROR_AREA_PROXY			(9<<16)					///< Add this to your error to get an Origin Proxy error.


/// \ingroup error
#define ORIGIN_ERROR_LEVEL_SHIFT		24          ///< [A description is required for this define]

/// \ingroup error
#define ORIGIN_ERROR_LEVEL_MASK		0x0F000000		///< The error level mask.

/// \ingroup error
#define ORIGIN_LEVEL_0				(0<<24)		///< A severe error.

/// \ingroup error
#define ORIGIN_LEVEL_1				(1<<24)		///< A major error.

/// \ingroup error
#define ORIGIN_LEVEL_2				(2<<24)		///< A minor error.

/// \ingroup error
#define ORIGIN_LEVEL_3				(3<<24)		///< A trivial error.

/// \ingroup error
#define ORIGIN_LEVEL_4				(4<<24)		///< Every error.

/// @}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

/// $errors

#endif /* DOXYGEN_SHOULD_SKIP_THIS */



/// \name Origin Error Codes
/// These defines specify Origin-specific errors.
/// @{

/// \ingroup error
#define ORIGIN_SUCCESS			0		///< The operation succeeded.

/// \ingroup error
#define ORIGIN_PENDING			1		///< The operation is still waiting to complete.

// General error codes

/// \ingroup error
#define ORIGIN_ERROR_GENERAL	-1		///< An unspecified error has occured. 

/// \ingroup error
#define ORIGIN_ERROR_INVALID_HANDLE				(ORIGIN_ERROR + ORIGIN_LEVEL_1 + ORIGIN_ERROR_AREA_GENERAL + 0)    ///< The provided handle is invalid.

/// \ingroup error
#define ORIGIN_ERROR_OUT_OF_MEMORY				(ORIGIN_ERROR + ORIGIN_LEVEL_0 + ORIGIN_ERROR_AREA_GENERAL + 1)    ///< Failed to allocate memory.

/// \ingroup error
#define ORIGIN_ERROR_NOT_IMPLEMENTED			(ORIGIN_ERROR + ORIGIN_LEVEL_1 + ORIGIN_ERROR_AREA_GENERAL + 2)    ///< The function is not implemented.

/// \ingroup error
#define ORIGIN_ERROR_INVALID_USER				(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_GENERAL + 3)    ///< The specified user is not valid in this context, or the userId is invalid.

/// \ingroup error
#define ORIGIN_ERROR_INVALID_ARGUMENT			(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_GENERAL + 4)    ///< One or more arguments are invalid.

/// \ingroup error
#define ORIGIN_ERROR_NO_CALLBACK_SPECIFIED		(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_GENERAL + 5)	///< The asynchronous operation expected a callback, but no callback was specified.

/// \ingroup error
#define ORIGIN_ERROR_BUFFER_TOO_SMALL			(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_GENERAL + 6)	///< The provided buffer doesn't have enough space to contain the requested data.

/// \ingroup error
#define ORIGIN_ERROR_TOO_MANY_VALUES_IN_LIST	(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_GENERAL + 7)	///< We are currently only supporting one item in the list. 

/// \ingroup error
#define ORIGIN_ERROR_NOT_FOUND					(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_GENERAL + 8)	///< The requested item was not found. 

/// \ingroup error
#define ORIGIN_ERROR_INVALID_PERSONA			(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_GENERAL + 9)    ///< The specified persona is not valid in this context, or the personaId is invalid.

/// \ingroup error
#define ORIGIN_ERROR_NO_NETWORK                  (ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_GENERAL + 10)	///< No internet connection available. 

/// \ingroup error
#define ORIGIN_ERROR_NO_SERVICE                  (ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_GENERAL + 11)	///< Origin services are unavailable. 

/// \ingroup error
#define ORIGIN_ERROR_NOT_LOGGED_IN               (ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_GENERAL + 12)	///< The user isn't logged in. No valid session is available. 

/// \ingroup error
#define ORIGIN_ERROR_MANDATORY_ORIGIN_UPDATE_PENDING    (ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_GENERAL + 13)	///< There is a mandatory update pending for Origin, this will prevent origin from going online. 

/// \ingroup error
#define ORIGIN_ERROR_ACCOUNT_IN_USE              (ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_GENERAL + 14)	///< The account is currently in use by another Origin instance. 

/// \ingroup error
#define ORIGIN_ERROR_TOO_MANY_INSTANCES          (ORIGIN_ERROR + ORIGIN_LEVEL_1 + ORIGIN_ERROR_AREA_GENERAL + 15)    ///< Too many instances of the OriginSDK created.

/// \ingroup error
#define ORIGIN_ERROR_ALREADY_EXISTS              (ORIGIN_ERROR + ORIGIN_LEVEL_3 + ORIGIN_ERROR_AREA_GENERAL + 16)    ///< The item already exists in the list.

/// \ingroup error
#define ORIGIN_ERROR_INVALID_OPERATION           (ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_GENERAL + 17)    ///< The requested operation cannot be performed.

/// \ingroup error
#define ORIGIN_ERROR_AGE_RESTRICTED              (ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_GENERAL + 18)    ///< The item has age restrictions.

/// \ingroup error
#define ORIGIN_ERROR_BANNED                      (ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_GENERAL + 19)    ///< The user is banned.

/// \ingroup error
#define ORIGIN_ERROR_NOT_READY                   (ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_GENERAL + 20)	///< The item is not ready. 

/// @}

/// \name SDK Error Codes
/// These defines specify Origin SDK-specific errors.
/// @{

// Sdk error codes

/// \ingroup error
#define ORIGIN_ERROR_SDK_NOT_INITIALIZED				(ORIGIN_ERROR + ORIGIN_LEVEL_0 + ORIGIN_ERROR_AREA_SDK + 0)		///< The Origin SDK was not running. 

/// \ingroup error
#define ORIGIN_ERROR_SDK_INVALID_ALLOCATOR_DEALLOCATOR_COMBINATION (ORIGIN_ERROR + ORIGIN_LEVEL_1 + ORIGIN_ERROR_AREA_SDK + 1) ///< Make sure that you provide both an allocator and a deallocator.

/// \ingroup error
#define ORIGIN_ERROR_SDK_IS_RUNNING					(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_SDK + 2)		///< The Origin SDK is running. This operation should only be done before the SDK is initialized or after the SDK is shutdown.

/// \ingroup error
#define ORIGIN_ERROR_SDK_NOT_ALL_RESOURCES_RELEASED	(ORIGIN_ERROR + ORIGIN_LEVEL_3 + ORIGIN_ERROR_AREA_SDK + 3)		///< The game is still holding on to the resource handles. Call #OriginDestroyHandle on resources that are no longer needed.

/// \ingroup error
#define ORIGIN_ERROR_SDK_INVALID_RESOURCE			(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_SDK + 4)		///< The resource in the resource map is invalid. Memory corruption?

/// \ingroup error
#define ORIGIN_ERROR_SDK_INTERNAL_ERROR				(ORIGIN_ERROR + ORIGIN_LEVEL_1 + ORIGIN_ERROR_AREA_SDK + 5)		///< The SDK experienced an internal error.

/// \ingroup error
#define ORIGIN_ERROR_SDK_INTERNAL_BUFFER_TOO_SMALL  (ORIGIN_ERROR + ORIGIN_LEVEL_1 + ORIGIN_ERROR_AREA_SDK + 6)		///< The internal buffer that the SDK is using is not big enough to receive the response. Inform OriginSDK Support.

/// @}

/// \name SDK Warning Codes
/// These defines specify Origin SDK-specific warnings.
/// @{

/// \ingroup error
#define ORIGIN_WARNING_SDK_ALREADY_INITIALIZED	(ORIGIN_WARNING + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_SDK + 1)		///< The Origin SDK is already initialized. 

/// \ingroup error
#define ORIGIN_WARNING_SDK_STILL_RUNNING			(ORIGIN_WARNING + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_SDK + 2)		///< The Origin SDK is still running.

/// \ingroup error
#define ORIGIN_WARNING_SDK_ENUMERATOR_IN_USE		(ORIGIN_WARNING + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_SDK + 3)		///< The Enumerator associated with the handle was in use.

/// \ingroup error
#define ORIGIN_WARNING_SDK_ENUMERATOR_TERMINATED		(ORIGIN_WARNING + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_SDK + 4)		///< The Enumerator associated with the handle was not finished.

/// @}

/// \name Core Error Codes
/// These defines specify Origin Core-specific errors.
/// @{

/// \ingroup error
#define ORIGIN_ERROR_CORE_NOTLOADED				(ORIGIN_ERROR + ORIGIN_LEVEL_0 + ORIGIN_ERROR_AREA_CORE + 0)		///< The Origin desktop application is not loaded.

/// \ingroup error
#define ORIGIN_ERROR_CORE_LOGIN_FAILED			(ORIGIN_ERROR + ORIGIN_LEVEL_1 + ORIGIN_ERROR_AREA_CORE + 1)		///< Origin couldn't authenticate with the Origin Servers.

/// \ingroup error
#define ORIGIN_ERROR_CORE_AUTHENTICATION_FAILED  (ORIGIN_ERROR + ORIGIN_LEVEL_1 + ORIGIN_ERROR_AREA_CORE + 2)		///< Origin seems to be running, but the LSX Authentication Challenge failed. No communication with Core is possible.

/// \ingroup error
#define ORIGIN_ERROR_CORE_SEND_FAILED			(ORIGIN_ERROR + ORIGIN_LEVEL_1 + ORIGIN_ERROR_AREA_CORE + 4)		///< Sending data to Origin failed.

/// \ingroup error
#define ORIGIN_ERROR_CORE_RECEIVE_FAILED			(ORIGIN_ERROR + ORIGIN_LEVEL_1 + ORIGIN_ERROR_AREA_CORE + 5)		///< Receiving data from Origin failed.

/// \ingroup error
#define ORIGIN_ERROR_CORE_RESOURCE_NOT_FOUND		(ORIGIN_ERROR + ORIGIN_LEVEL_1 + ORIGIN_ERROR_AREA_CORE + 6)		///< The requested resource could not be located.

/// \ingroup error
#define ORIGIN_ERROR_CORE_INCOMPATIBLE_VERSION	(ORIGIN_ERROR + ORIGIN_LEVEL_0 + ORIGIN_ERROR_AREA_CORE + 7)		///< The Origin version is too old to work with this SDK version.

/// \ingroup error
#define ORIGIN_ERROR_CORE_NOT_INSTALLED			(ORIGIN_ERROR + ORIGIN_LEVEL_0 + ORIGIN_ERROR_AREA_CORE + 8)		///< The Origin installation couldn't be found.

/// @}

/// \name IGO Error and Warning Codes
/// These defines specify In-game Overlay errors and warnings.
/// @{

/// \ingroup error
#define ORIGIN_WARNING_IGO_NOTLOADED				(ORIGIN_WARNING + ORIGIN_LEVEL_1 + ORIGIN_ERROR_AREA_IGO + 0)	///< The IGO could not be loaded, so SDK functionality is degraded.

/// \ingroup error
#define ORIGIN_WARNING_IGO_SUPPORT_NOTLOADED		(ORIGIN_WARNING + ORIGIN_LEVEL_1 + ORIGIN_ERROR_AREA_IGO + 1)	///< IGO support is not loaded, so SDK functionality is degraded.

/// \ingroup error
#define ORIGIN_ERROR_IGO_ILLEGAL_ANCHOR_POINT	(ORIGIN_ERROR + ORIGIN_LEVEL_3 + ORIGIN_ERROR_AREA_IGO + 2)	///< The combination of anchor point bits doesn't resolve to a proper dialog anchor point.

/// \ingroup error
#define ORIGIN_ERROR_IGO_ILLEGAL_DOCK_POINT		(ORIGIN_ERROR + ORIGIN_LEVEL_3 + ORIGIN_ERROR_AREA_IGO + 3)	///< The combination of dock point bits doesn't resolve to a proper dock point.

/// \ingroup error
#define ORIGIN_ERROR_IGO_NOT_AVAILABLE			(ORIGIN_ERROR + ORIGIN_LEVEL_3 + ORIGIN_ERROR_AREA_IGO + 4)	///< The IGO is not available.

/// @}

/// \name Presence Error Codes
/// These defines specify Origin Presence errors.
/// @{

/// \ingroup error
#define ORIGIN_ERROR_NO_MULTIPLAYER_ID           (ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_PRESENCE + 0)   ///< It is not possible to set the presence to JOINABLE when no multiplayer Id is defined on the offer. 

/// @}

/// \name Friends Error Codes
/// These defines specify Origin Friends errors.
/// @{

/// \ingroup error
#define ORIGIN_ERROR_LSX_INVALID_RESPONSE		(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_LSX + 0)	///< The LSX Decoder didn't expect this response.

/// \ingroup error
#define ORIGIN_ERROR_LSX_NO_RESPONSE				(ORIGIN_ERROR + ORIGIN_LEVEL_1 + ORIGIN_ERROR_AREA_LSX + 1)	///< The LSX server didn't respond within the set timeout.

/// \ingroup error
#define ORIGIN_ERROR_LSX_INVALID_REQUEST			(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_LSX + 2)	///< The LSX Decoder didn't expect this request.

/// @}

/// \name Commerce Error Codes
/// These defines specify Origin Commerce errors.
/// @{

/// \ingroup error
#define ORIGIN_ERROR_COMMERCE_NO_SUCH_STORE		(ORIGIN_ERROR + ORIGIN_LEVEL_1 + ORIGIN_ERROR_AREA_COMMERCE + 0)	///< The store could not be found.

/// \ingroup error
#define ORIGIN_ERROR_COMMERCE_NO_SUCH_CATALOG	(ORIGIN_ERROR + ORIGIN_LEVEL_1 + ORIGIN_ERROR_AREA_COMMERCE + 1)	///< The catalog could not be found.

/// \ingroup error
#define ORIGIN_ERROR_COMMERCE_INVALID_REPLY		(ORIGIN_ERROR + ORIGIN_LEVEL_1 + ORIGIN_ERROR_AREA_COMMERCE + 2)	///< The server reply was not understood.

/// \ingroup error
#define ORIGIN_ERROR_COMMERCE_NO_CATEGORIES		(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_COMMERCE + 3)	///< No categories were found.

/// \ingroup error
#define ORIGIN_ERROR_COMMERCE_NO_PRODUCTS		(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_COMMERCE + 4)	///< No products were found.

/// \ingroup error
#define ORIGIN_ERROR_COMMERCE_UNDERAGE_USER		(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_COMMERCE + 5)	///< The user is under age and is blocked to perform this action.

/// \ingroup error
#define ORIGIN_ERROR_COMMERCE_DEPRECATED_STORE	(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_COMMERCE + 6)	///< The user's OS is deprecated and is blocked to perform this action.


/// @}


/// \name Origin Proxy Error Codes.
/// These defines specify Origin Proxy errors.
/// @{
/// \ingroup error
#define ORIGIN_ERROR_PROXY						(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_PROXY + 0)	///< Base proxy error. You shouldn't get this error.

/// \ingroup error
#define ORIGIN_SUCCESS_PROXY_OK			        (ORIGIN_WARNING + ORIGIN_LEVEL_4 + ORIGIN_ERROR_AREA_PROXY + 200)  ///< Server success: OK.

/// \ingroup error
#define ORIGIN_SUCCESS_PROXY_CREATED			(ORIGIN_WARNING + ORIGIN_LEVEL_4 + ORIGIN_ERROR_AREA_PROXY + 201)  ///< Server success: Created.

/// \ingroup error
#define ORIGIN_SUCCESS_PROXY_ACCEPTED			(ORIGIN_WARNING + ORIGIN_LEVEL_4 + ORIGIN_ERROR_AREA_PROXY + 202)  ///< Server success: Accepted.

/// \ingroup error
#define ORIGIN_SUCCESS_PROXY_NON_AUTH_INFO		(ORIGIN_WARNING + ORIGIN_LEVEL_4 + ORIGIN_ERROR_AREA_PROXY + 203)  ///< Server success: Non-Authoritative Information.

/// \ingroup error
#define ORIGIN_SUCCESS_PROXY_NO_CONTENT			(ORIGIN_WARNING + ORIGIN_LEVEL_4 + ORIGIN_ERROR_AREA_PROXY + 204)  ///< Server success: No Content.

/// \ingroup error
#define ORIGIN_SUCCESS_RESET_CONTENT	        (ORIGIN_WARNING + ORIGIN_LEVEL_4 + ORIGIN_ERROR_AREA_PROXY + 205)  ///< Server success: Reset Content.

/// \ingroup error
#define ORIGIN_SUCCESS_PARTIAL_CONTENT		    (ORIGIN_WARNING + ORIGIN_LEVEL_4 + ORIGIN_ERROR_AREA_PROXY + 206)  ///< Server success: Partial Content.

/// \ingroup error
#define ORIGIN_ERROR_PROXY_BAD_REQUEST			(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_PROXY + 400)  ///< Server error: Bad Request

/// \ingroup error
#define ORIGIN_ERROR_PROXY_UNAUTHORIZED			(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_PROXY + 401)  ///< Server error: Unauthorized. 

/// \ingroup error
#define ORIGIN_ERROR_PROXY_PAYMENT_REQUIRED		(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_PROXY + 402)  ///< Server error: Payment Required. 

/// \ingroup error
#define ORIGIN_ERROR_PROXY_FORBIDDEN 			(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_PROXY + 403)  ///< Server error: Forbidden.

/// \ingroup error
#define ORIGIN_ERROR_PROXY_NOT_FOUND				(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_PROXY + 404)  ///< Server error: Not found.

/// \ingroup error
#define ORIGIN_ERROR_PROXY_METHOD_NOT_ALLOWED	(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_PROXY + 405)  ///< Server error: Method not Allowed.

/// \ingroup error
#define ORIGIN_ERROR_PROXY_NOT_ACCEPTABLE	    (ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_PROXY + 406)  ///< Server error: Not Acceptable.

/// \ingroup error
#define ORIGIN_ERROR_PROXY_REQUEST_TIMEOUT	    (ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_PROXY + 408)  ///< Server error: Request Timeout.

/// \ingroup error
#define ORIGIN_ERROR_PROXY_CONFLICT	            (ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_PROXY + 409)  ///< Server error: Conflict.

/// \ingroup error
#define ORIGIN_ERROR_PROXY_INTERNAL_ERROR		(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_PROXY + 500)  ///< Server error: Internal Server Error.

/// \ingroup error
#define ORIGIN_ERROR_PROXY_NOT_IMPLEMENTED		(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_PROXY + 501)  ///< Server error: Not Implemented.

/// \ingroup error
#define ORIGIN_ERROR_PROXY_BAD_GATEWAY	(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_PROXY + 502)  ///< Server error: Bad Gateway.

/// \ingroup error
#define ORIGIN_ERROR_PROXY_SERVICE_UNAVAILABLE	(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_PROXY + 503)  ///< Server error: Service Unavailable.

/// \ingroup error
#define ORIGIN_ERROR_PROXY_GATEWAY_TIMEOUT	(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_PROXY + 504)  ///< Server error: Gateway Timeout.
