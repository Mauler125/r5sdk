/*H********************************************************************************/
/*!
    \File netgameerr.h

    \Description
        Error codes for GameDist functions (errors always negative)

    \Copyright
        Copyright (c) Electronic Arts 1998-2007

    \Version 1.0 23/06/2009 (jrainy) First Version
*/
/********************************************************************************H*/

#ifndef _netgameerr_h
#define _netgameerr_h

/*!
\Moduledef NetGameErr NetGameErr
\Modulemember Game
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Defines **********************************************************************/

#define GMDIST_ORPHAN                       (-1)    //<! error - orphan
#define GMDIST_OVERFLOW                     (-2)    //<! error - overflow
#define GMDIST_INVALID                      (-3)    //<! error - invalid
#define GMDIST_BADSETUP                     (-4)    //<! error - badly setup number of players and/or index
#define GMDIST_SENDPROC_FAILED              (-5)    //<! error condition - SendProc failed
#define GMDIST_QUEUE_FULL                   (-6)    //<! error condition - too many queued packets
#define GMDIST_QUEUE_MEMORY                 (-7)    //<! error condition - queue has insufficient space left
#define GMDIST_STREAM_FAILURE               (-8)    //<! error - stream failed
#define GMDIST_NO_STREAM                    (-9)    //<! error - stream failed (no ref)
#define GMDIST_DELETED                      (-10)   //<! error - deleted
#define GMDIST_PEEK_ERROR                   (-11)   //<! error peek
#define GMDIST_INPUTLOCAL_FAILED            (-12)   //<! error inputlocal
#define GMDIST_INPUTQUERY_FAILED            (-13)   //<! error inputquery
#define GMDIST_DISCONNECTED                 (-14)   //<! error disconnected
#define GMDIST_INPUTLOCAL_FAILED_INVALID    (-15)   //<! error inputlocal with invalid.
#define GMDIST_INPUTLOCAL_FAILED_MULTI      (-16)   //<! error inputlocal with invalid.
#define GMDIST_INPUTLOCAL_FAILED_WINDOW     (-17)   //<! error inputlocal with invalid.
#define GMDIST_OVERFLOW_MULTI               (-18)   //<! error - overflow
#define GMDIST_OVERFLOW_WINDOW              (-19)   //<! error - overflow
#define GMDIST_DESYNCED                     (-20)   //<! error - desynced
#define GMDIST_DESYNCED_ALL_PLAYERS         (-21)   //<! error - desynced all players

#define GMLINK_DIRTYCAST_DELETED              (-64)   //<! error - GameServer had the link deleted
#define GMLINK_DIRTYCAST_CONNECTION_TIMEDOUT  (-65)   //<! error - GameServer connection timed out
#define GMLINK_DIRTYCAST_CONNECTION_FAILURE   (-66)   //<! error - GameServer initial connection failed
#define GMLINK_DIRTYCAST_PEER_DISCONNECT      (-67)   //<! error - GameServer peer diconnected

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

//@}

#endif // _netgameerr_h
