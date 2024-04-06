/*H********************************************************************************/
/*!
    \File dirtymem.h

    \Description
        DirtySock memory allocation routines.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 10/12/2005 (jbrookes) First Version
    \Version 11/19/2008 (mclouatre) Adding pMemGroupUserData to mem groups
*/
/********************************************************************************H*/

#ifndef _dirtymem_h
#define _dirtymem_h

/*!
\Moduledef DirtyMem DirtyMem
\Modulemember DirtySock
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"

/*** Defines **********************************************************************/

/*!
    All DirtySock modules have their memory identifiers defined here.
*/
// comm modules
#define COMMSRP_MEMID           ('csrp')
#define COMMUDP_MEMID           ('cudp')

// crypt modules
#define CRYPTRSA_MEMID          ('crsa')
#define CRYPTRAND_MEMID         ('rand')

// dirtysock modules
#define DIRTYAUTH_MEMID         ('dath')
#define DIRTYCERT_MEMID         ('dcrt')
#define DIRTYCM_MEMID           ('dhcm')
#define DIRTYSESSMGR_MEMID      ('dsmg')
#define DIRTYWEBAPI_MEMID       ('weba')
#define DIRTYEVENT_DISP_MEMID   ('semd')
#define SOCKET_MEMID            ('dsoc')
#define NETCONN_MEMID           ('ncon')
#define DIRTYTHREAD_MEMID       ('dthr')

// game modules
#define CONNAPI_MEMID           ('conn')
#define NETGAMEDIST_MEMID       ('ngdt')
#define NETGAMEDISTSERV_MEMID   ('ngds')
#define NETGAMELINK_MEMID       ('nglk')
#define NETGAMEUTIL_MEMID       ('ngut')

// graph modules
#define DIRTYGRAPH_MEMID        ('dgph')
#define DIRTYJPG_MEMID          ('djpg')
#define DIRTYPNG_MEMID          ('dpng')

// misc modules
#define LOBBYLAN_MEMID          ('llan')
#define USERAPI_MEMID           ('uapi')
#define USERLISTAPI_MEMID       ('ulst')
#define WEBLOG_MEMID            ('wlog')
#define PRIVILEGEAPI_MEMID      ('priv')

// proto modules
#define PROTOADVT_MEMID         ('padv')
#define PROTOHTTP_MEMID         ('phtp')
#define HTTPSERV_MEMID          ('hsrv')
#define HTTPMGR_MEMID           ('hmgr')
#define PROTOMANGLE_MEMID       ('pmgl')
#define PROTOPING_MEMID         ('ppng')
#define PINGMGR_MEMID           ('lpmg')
#define PROTOSSL_MEMID          ('pssl')
#define PROTOSTREAM_MEMID       ('pstr')
#define PROTOTUNNEL_MEMID       ('ptun')
#define PROTOUDP_MEMID          ('pudp')
#define PROTOUPNP_MEMID         ('pupp')
#define PROTOWEBSOCKET_MEMID    ('webs')

// util modules
#define DISPLIST_MEMID          ('ldsp')
#define HASHER_MEMID            ('lhsh')
#define SORT_MEMID              ('lsor')
#define HPACK_MEMID             ('hpak')
#define PROTOBUF_MEMID          ('pbuf')

// qos module
#define QOSAPI_MEMID            ('dqos')
#define QOS_CLIENT_MEMID        ('qosc')
#define QOS_COMMON_MEMID        ('qcom')

// voip module
#define VOIP_MEMID              ('voip')
#define VOIPNARRATE_MEMID       ('vnar')
#define VOIPTRANSCRIBE_MEMID    ('vscr')
#define VOIP_PLATFORM_MEMID     ('vplt')  //<! memory allocated by 1st-party voip lib (xone: game chat 2, others: unused)

// voiptunnel module
#define VOIPTUNNEL_MEMID        ('vtun')

// web modules
#define NETRSRC_MEMID           ('nrsc')
#define WEBOFFER_MEMID          ('webo')


/*** Macros ***********************************************************************/

#if !DIRTYCODE_DEBUG
 #define DirtyMemDebugAlloc(_pMem, _iSize, _iMemModule, _iMemGroup, _pMemGroupUserData) {;}
 #define DirtyMemDebugFree(_pMem, _iSize, _iMemModule, _iMemGroup, _pMemGroupUserData) {;}
#endif

/*** Type Definitions *************************************************************/
//#if defined(DIRTYCODE_DLL)

//! Used in the dll mode to specify DirtyMemAlloc function
typedef void *(DirtyMemAllocT)(int32_t iSize, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData);

//! Used in the dll mode to specify DirtyMemFree function
typedef void (DirtyMemFreeT)(void *pMem, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData);

//#endif
/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

//! enter memory group
DIRTYCODE_API void DirtyMemGroupEnter(int32_t iGroup, void *pMemGroupUserData);

//! leave memory group
DIRTYCODE_API void DirtyMemGroupLeave(void);

//! get current memory group
DIRTYCODE_API void DirtyMemGroupQuery(int32_t *pMemGroup, void **ppMemGroupUserData);

#if DIRTYCODE_DEBUG
//! display memory allocation info to debug output (debug only)
DIRTYCODE_API void DirtyMemDebugAlloc(void *pMem, int32_t iSize, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData);

//! display memory free info to debug output (debug only)
DIRTYCODE_API void DirtyMemDebugFree(void *pMem, int32_t iSize, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData);
#endif

/*
 Memory allocation routines - not implemented in the lib; these must be supplied by the user.
 In DLL mode use DirtyMemFuncSet to set the DirtyMemAlloc and DirtyMemFree before using any DS functions.
 If its not set default allocator will be used.
*/

#if defined(DIRTYCODE_DLL)
//! Use to set the Memory Alloc and Free functions
DIRTYCODE_API void DirtyMemFuncSet(DirtyMemAllocT *pMemAlloc, DirtyMemFreeT *pMemFree);
#endif

//! allocate memory
DIRTYCODE_API void *DirtyMemAlloc(int32_t iSize, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData);

//! free memory
DIRTYCODE_API void DirtyMemFree(void *pMem, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData);


#ifdef __cplusplus
}
#endif

//@}

#endif // _dirtymem_h

