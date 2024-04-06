/*H********************************************************************************/
/*!
    \File testerconsole.c

    \Description
        This module buffers console output for the tester application.
        In essence, it is just a large FIFO which knows how to handle
        newline characters as expected.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 09/15/1999 (gschaefer) First Version
    \Version 11/08/1999 (gschaefer) Cleanup and revision
    \Version 03/29/2005 (jfrank)    Update for Tester2
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>
#include "DirtySDK/dirtysock.h"
#include "libsample/zmem.h"
#include "testercomm.h"
#include "testerregistry.h"
#include "testerconsole.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

struct TesterConsoleT   //!< console state structure
{
    char *pBuf;         //!< pointer to console buffer
    int32_t iLen;           //!< length of console buffer
    int32_t iInp;           //!< fifo input offset
    int32_t iOut;           //!< fifo output offset
    int32_t iWrap;          //!< flag to indicate overflow handling
    int32_t iRefcon;        //!< reference constant for callback
    void *pRefptr;      //!< reference pointer for callback
};

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function TesterConsoleOutput

    \Description
        Add text to console buffer.

    \Input *pRef     - console reference
    \Input *pText    - text to add to buffer (\n newlines are fine)

    \Output None

    \Version 09/15/1999 (gschaefer)
*/
/********************************************************************************F*/
static void _TesterConsoleAddText(TesterConsoleT *pRef, const char *pText)
{
    char ch;
    
    // insert complete string
    for ( ; (ch = *pText++) != 0; ) 
    {
        // save the data and advance index
        pRef->pBuf[pRef->iInp] = ch;
        pRef->iInp = (pRef->iInp+1) % pRef->iLen;

        // check for overflow
        if (pRef->iInp == pRef->iOut) 
        {
            if (!pRef->iWrap) 
            {
                // restore input index to old value
                pRef->iInp = (pRef->iInp+pRef->iLen-1) % pRef->iLen;
                break;
            }
            // delete oldest character
            pRef->iOut = (pRef->iOut+1) % pRef->iLen;
        }
    }
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function TesterConsoleCreate

    \Description
        Create instance of a console buffer.

    \Input   iSize - buffer size (recommended 4K min)
    \Input   iWrap - flag to indicate overflow action (true=wrap over oldest)
    \Input iRefcon - constant passed back during flush callback
    \Input pRefptr - pointer passed back during flush callback

    \Output TesterConsoleT * - new console pointer

    \Version 09/15/1999 (gschaefer)
*/
/********************************************************************************F*/
TesterConsoleT *TesterConsoleCreate(int32_t iSize, int32_t iWrap)
{
    TesterConsoleT *pRef;

    pRef = ZMemAlloc(sizeof(*pRef));
    if (pRef == NULL)
        return(NULL);

    pRef->iLen = iSize;

    pRef->iWrap = iWrap;
    pRef->iInp = pRef->iOut = 0;
    pRef->pBuf = ZMemAlloc(pRef->iLen+1);
    
    TesterRegistrySetPointer("CONSOLE", pRef);

    return(pRef);
}


/*F********************************************************************************/
/*!
    \Function TesterConsoleCreate

    \Description
        Connect a console to a particular ref set.

    \Input   *pRef - console reference
    \Input iRefcon - constant passed back during flush callback
    \Input pRefptr - pointer passed back during flush callback

    \Output None

    \Version 03/29/2005 (jfrank)
*/
/********************************************************************************F*/
void TesterConsoleConnect(TesterConsoleT *pRef, int32_t iRefcon, void *pRefptr)
{
    pRef->iRefcon = iRefcon;
    pRef->pRefptr = pRefptr;
}


/*F********************************************************************************/
/*!
    \Function TesterConsoleClear

    \Description
        Clear the console

    \Input   *pRef - console reference

    \Output None

    \Version 04/07/2005 (jfrank)
*/
/********************************************************************************F*/
void TesterConsoleClear(TesterConsoleT *pRef)
{
    // check for errors
    if(pRef == NULL)
        return;

    // wipe both the data and the head/tail pointers
    ds_memclr(pRef->pBuf, (pRef->iLen)+1);
    pRef->iInp = pRef->iOut = 0;
}


/*F********************************************************************************/
/*!
    \Function TesterConsoleOutput

    \Description
        Add text to console buffer.

    \Input *pRef     - console reference
    \Input  iMsgType - message type to prepend to the console output, NONE for none
    \Input *pText    - text to add to buffer (\n newlines are fine)

    \Output None

    \Version 04/04/2005 (jfrank)
*/
/********************************************************************************F*/
void TesterConsoleOutput(TesterConsoleT *pRef, int32_t iMsgType, const char *pText)
{
    // add the content of the text
    _TesterConsoleAddText(pRef, pText);
}


/*F********************************************************************************/
/*!
    \Function TesterConsoleFlush

    \Description
        Flush buffer data to output handler (calls output handler).

    \Input  *pRef - console reference
    \Input *pProc - Address of output handler

    \Output None

    \Version 09/15/1999 (gschaefer)
*/
/********************************************************************************F*/
void TesterConsoleFlush(TesterConsoleT *pRef, TesterConsoleDisplayCbT *pProc)
{
    if (pRef->iOut < pRef->iInp) 
    {
        // grab data in one chunk
        pRef->pBuf[pRef->iInp] = 0;
        (*pProc)(pRef->pBuf + pRef->iOut, pRef->iInp - pRef->iOut, pRef->iRefcon, pRef->pRefptr);
        pRef->iOut = pRef->iInp;
    } 
    else if (pRef->iOut > pRef->iInp) 
    {
        // grab data in two chunks
        pRef->pBuf[pRef->iLen] = 0;
        (*pProc)(pRef->pBuf + pRef->iOut, pRef->iLen - pRef->iOut, pRef->iRefcon, pRef->pRefptr);
        pRef->pBuf[pRef->iInp] = 0;
        (*pProc)(pRef->pBuf, pRef->iInp, pRef->iRefcon, pRef->pRefptr);
        pRef->iOut = pRef->iInp;
    }
}


/*F********************************************************************************/
/*!
    \Function TesterConsoleDestroy

    \Description
        Release resources and destroy console module.

    \Input  *pRef - console reference

    \Output None

    \Version 09/15/1999 (gschaefer)
*/
/********************************************************************************F*/
void TesterConsoleDestroy(TesterConsoleT *pRef)
{
    ZMemFree(pRef->pBuf);
    ZMemFree(pRef);
    TesterRegistrySetPointer("CONSOLE", NULL);
}

