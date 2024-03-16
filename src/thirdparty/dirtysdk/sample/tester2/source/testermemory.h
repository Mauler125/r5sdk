/*H********************************************************************************/
/*!
    \File testermemory.h

    \Description
        Tester memory management.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 11/01/2005 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _testermemory_h
#define _testermemory_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// enable/disable verbose dirtysock memory debugging
 void TesterMemorySetDebug(uint32_t bDebug);

#ifdef __cplusplus
};
#endif

#endif // _testermemory_h

