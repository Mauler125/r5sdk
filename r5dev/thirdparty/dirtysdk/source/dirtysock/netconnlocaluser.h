/*H********************************************************************************/
/*!
    \File netconnlocaluser.h

    \Description
        Wrapper for EA::User::IEAUser functionality

    \Copyright
        Copyright (c) 2017 Electronic Arts Inc.

    \Version 10/24/2017 (amakoukji) First Version
*/
/********************************************************************************H*/

#ifndef _netconnlocaluser_h
#define _netconnlocaluser_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/
struct NetConnRefT;

//! user event callback function prototype
typedef void (NetConnAddLocalUserCallbackT)(struct NetConnLocalUserRefT *pCommonRef, int32_t iLocalUserIndex, const EA::User::IEAUser *pIEAUser);
typedef void (NetConnRemoveLocalUserCallbackT)(struct NetConnLocalUserRefT *pCommonRef, int32_t iLocalUserIndex, const EA::User::IEAUser *pIEAUser);


typedef enum NetConnIEAUserEventTypeE
{
    NETCONN_EVENT_IEAUSER_ADDED = 0,
    NETCONN_EVENT_IEAUSER_REMOVED
} NetConnIEAUserEventTypeE;

typedef struct NetConnIEAUserEventT
{
    struct NetConnIEAUserEventT *pNext; //!< linked list
    const EA::User::IEAUser *pIEAUser;  //!< IEAUser reference
    NetConnIEAUserEventTypeE eEvent;    //!< event type
    int32_t iLocalUserIndex;            //!< local user index
} NetConnIEAUserEventT;

typedef struct NetConnLocalUserRefT
{
    // module memory group
    int32_t              iMemGroup;                               //!< module mem group id
    void                 *pMemGroupUserData;                      //!< user data associated with mem group

    int32_t iDebugLevel;

    NetConnIEAUserEventT *pIEAUserFreeEventList;                  //!< list of free IEAUser
    NetConnIEAUserEventT *pIEAUserEventList;                      //!< list of pending NetConnIEAUserEvents - populated by customers with NetConnAddLocaUser()/NetConnRemoveUser()

    NetConnAddLocalUserCallbackT *pAddUserCb;
    NetConnRemoveLocalUserCallbackT *pRemoveUserCb;

    NetConnRefT *pNetConn;                                        //!< parent

    NetCritT     crit;
} NetConnLocalUserRefT;


/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// handle shutdown functionality
void NetConnLocalUserDestroy(NetConnLocalUserRefT *pLocalUserRef);

// handle startup functionality
NetConnLocalUserRefT* NetConnLocalUserInit(NetConnRefT *pNetConn, NetConnAddLocalUserCallbackT *pAddUserCb, NetConnRemoveLocalUserCallbackT *pRemoveUserCb);

// handle add user functionality
int32_t NetConnLocalUserAdd(int32_t iLocalUserIndex, const EA::User::IEAUser *pLocalUser);

// handle remove user functionality
int32_t NetConnLocalUserRemove(int32_t iLocalUserIndex, const EA::User::IEAUser *pLocalUser);

// handle user update functionality
void NetConnLocalUserUpdate(NetConnLocalUserRefT *pLocalUserRef);

#ifdef __cplusplus
}
#endif

#endif // _netconnlocaluser_h

