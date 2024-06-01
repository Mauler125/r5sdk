/*H*************************************************************************************/
/*!

    \File t2new.cpp

    \Description
        This implements code in support of memory allocation in C++ code

    \Copyright
        Copyright (c) Electronic Arts 2017.  ALL RIGHTS RESERVED.

    \Version 12/01/2017 (eesponda)
*/
/*************************************************************************************H*/

/*** Include files *********************************************************************/

#include <new>

void* operator new[](size_t size, unsigned long, unsigned long, char const*, int, unsigned int, char const*, int)
{
    return operator new(size);
}
void* operator new[](size_t size, char const*, int, unsigned int, char const*, int)
{
    return operator new(size);
}

