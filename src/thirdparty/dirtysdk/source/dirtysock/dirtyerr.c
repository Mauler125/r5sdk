/*H********************************************************************************/
/*!
    \File dirtyerr.c

    \Description
        Dirtysock platform independent debug error routines.

    \Copyright
        Copyright (c) 2014 Electronic Arts Inc.

    \Version 09/16/2014 (cvienneau) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtyerr.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function DirtyErrGetHResult

    \Description
        create a unique error code for use across DirtySDK

    \Input uFacility     - the module id the hResult is being generated for (only bottom 11 bits)
    \Input iCode         - the error code
    \Input bFailure      - true if the code represents a failure

    \Output uint32_t     - hResult value

    \Notes
    \verbatim
        Description of HRESULT:
         http://msdn.microsoft.com/en-us/library/cc231198.aspx
         Bit  [ 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 09, 08, 07 06, 05, 04, 03, 02, 01, 00]
         Field[  S,  R,  C,  N,  X,  - - - - - - - - - - - FACILITY - - - - - , - - - - - - - - - - - - - - - - CODE - - - - - - - - - - - - ]
         S - Severity - indicates success/fail;  0 - Success, 1 - Failure
         R - Reserved portion of the facility code, corresponds to NT's second severity bit; 1 - Severe Failure
         C - Customer. This bit specifies if the value is customer-defined or Microsoft-defined; 0 - Microsoft-defined, 1 - Customer-defined
         N - Reserved portion of the facility code. Used to indicate a mapped NT status value.
         X - Reserved portion of the facility code. Reserved for internal use. Used to indicate HRESULT values that are not status values, but are instead message ids for display strings.
         FACILITY - indicates the system service that is responsible for the error.
         CODE - is the facility's status code
    \endverbatim
    \Version 09/16/2014 (cvienneau)
*/
/********************************************************************************F*/
uint32_t DirtyErrGetHResult(uint16_t uFacility, int16_t iCode, uint8_t bFailure)
{
    uint32_t hResult = 0;

    if (bFailure)
    {
        hResult |= 0x80000000;      //set the "Severity" bit, usually we use an hResult to describe failures so this is the usual case
    }
    hResult |= 0x20000000;          //set the "Customer" bit, we aren't MS so we'll always set this
    uFacility &= ~(0xF800);         //the top 5 bits of the facility passed in do not belong to the user, we will clear them out so they don't mess up the upper bits (we could maybe assert this)
    hResult |= (uFacility << 16);   //set the "FACILITY", the module producing the error
    hResult |= (uint16_t)iCode;     //set the "CODE", the error id
    return(hResult);
}

/*F********************************************************************************/
/*!
    \Function DirtyErrDecodeHResult

    \Description
        break a hresult back into its components

    \Input hResult      - the hresult to be decoded
    \Input pFacility    - return the module id the hResult is being generated for
    \Input pCode        - return the error code
    \Input pCustomer    - return true if the customer bit is set (note DS will always set the customer bit in DirtyErrGetHResult). 
    \Input pFailure     - return true if the code represents a failure

    \Version 01/17/2017 (cvienneau)
*/
/********************************************************************************F*/
void DirtyErrDecodeHResult(uint32_t hResult, uint16_t* pFacility, int16_t* pCode, uint8_t* pCustomer, uint8_t* pFailure)
{
    if (pFailure != NULL)
        *pFailure = (hResult & 0x80000000) >> 31;

    if (pCustomer != NULL)
        *pCustomer = (hResult & 0x20000000) >> 29;

    if (pFacility != NULL)
        *pFacility = (hResult & 0x7FF0000) >> 16;
    
    if (pCode != NULL)
        *pCode = (hResult & 0x0000FFFF);
}