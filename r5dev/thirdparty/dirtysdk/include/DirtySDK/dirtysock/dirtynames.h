/*H*************************************************************************************/
/*!
    \File    dirtynames.h

    \Description
        This module provides helper functions for manipulating persona and master
        account name strings.

    \Copyright
        Copyright (c) Electronic Arts 2002-2003

    \Version    1.0        12/10/03 (DBO) First Version
*/
/*************************************************************************************H*/

#ifndef _dirtynames_h
#define _dirtynames_h

/*!
\Moduledef DirtyNames DirtyNames
\Modulemember DirtySock
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// compare two names for equality.  Ignore case and strip non-printable ascii characters.
DIRTYCODE_API int32_t DirtyUsernameCompare(const char *pName1, const char *pName2);

// determine if pMatch is a substring of pSrc.  Ignore case and strip non-printable ascii characters.
DIRTYCODE_API int32_t DirtyUsernameSubstr(const char *pSrc, const char *pMatch);

// generate the hash code for the given name.  Ignore case and strip non-printable ascii characters.
DIRTYCODE_API uint32_t DirtyUsernameHash(const char *pName);

// create the canonical form of the given name.  Ignore case and strip non-printable ascii characters.
DIRTYCODE_API int32_t DirtyNameCreateCanonical(const char *pName, char * pCanonical, size_t uLen);

// convert a char to xlat table index using its ascii char representation.  
int32_t toXlatIndex(const char str);

#ifdef __cplusplus
}
#endif

//@}

#endif // _dirtynames_h

