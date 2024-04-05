#ifndef SERVER_LIVEAPI_H
#define SERVER_LIVEAPI_H

#define LIVEAPI_MAJOR_VERSION 0
#define LIVEAPI_MINOR_VERSION 1
#define LIVEAPI_REVISION "Rev: " MKSTRING(LIVEAPI_MAJOR_VERSION) "." MKSTRING(LIVEAPI_MINOR_VERSION)

extern void Script_RegisterLiveAPIFunctions(CSquirrelVM* const s);
extern void Script_RegisterLiveAPIEnums(CSquirrelVM* const s);

#endif // SERVER_LIVEAPI_H
