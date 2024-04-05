/*H********************************************************************************/
/*!
    \File zfile.c

    \Description
        Platform-inspecific host file operations.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 02/16/2005 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <string.h>

#include "DirtySDK/platform.h"
#include "zmem.h"
#include "zlib.h"
#include "zfile.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function ZFileSize

    \Description
        Get file size.

    \Input iFileId  - file id to get size of

    \Output
        int32_t         - file size, or -1 on error

    \Version 02/16/2005 (jbrookes)
*/
/********************************************************************************F*/
int64_t ZFileSize(ZFileT iFileId)
{
    int64_t iEnd = ZFileSeek(iFileId, 0, ZFILE_SEEKFLAG_END);
    ZFileSeek(iFileId, 0, ZFILE_SEEKFLAG_SET);
    return(iEnd);
}

/*F********************************************************************************/
/*!
    \Function ZFileLoad

    \Description
        Open a file and read it into memory.

    \Input *pFilename   - pointer to name of file to read
    \Input *pFileSize   - [out] storage for file size
    \Input uOpenFlags   - open flags, or zero for default (READONLY)

    \Output
        char *          - pointer to file data, or NULL

    \Version 02/16/2005 (jbrookes)
*/
/********************************************************************************F*/
char *ZFileLoad(const char *pFileName, int32_t *pFileSize, uint32_t uOpenFlags)
{
    int64_t iFileSize64;
    int32_t iFileSize, iResult;
    char *pFileMem;
    ZFileT iFileId;

    // determine open flags
    if (uOpenFlags == 0)
    {
        uOpenFlags = ZFILE_OPENFLAG_RDONLY;
    }

    // open the file
    iFileId = ZFileOpen(pFileName, uOpenFlags);
    if (iFileId == ZFILE_INVALID)
    {
        printf("zfile: unable to open file '%s'\n", pFileName);
        return(NULL);
    }

    // get the file size
    iFileSize64 = ZFileSize(iFileId);
    if (iFileSize64 < 0)
    {
        printf("zfile: unable to get size of file '%s'\n", pFileName);
        return(NULL);
    }
    iFileSize = (int32_t)iFileSize64; // this function does not support files >2GB

    // allocate and clear memory for the file
    pFileMem = (char *) ZMemAlloc(iFileSize+1);
    if (pFileMem == NULL)
    {
        printf("zfile: unable to allocate %d bytes to load file '%s'\n", iFileSize+1, pFileName);
        return(NULL);
    }
    ds_memclr(pFileMem, iFileSize+1);

    // read file into memory
    iResult = ZFileRead(iFileId, pFileMem, iFileSize);
    if (iResult <= 0)
    {
        printf("zfile: unable to read file '%s'\n", pFileName);
        return(NULL);
    }
    /* null-terminate; we do this in addition to the memset up above because 
       under windows the size of a text file on disk may be larger than the
       file in memory */
    pFileMem[iResult] = '\0';

    // if size parameter is not null, set it
    if (pFileSize != NULL)
    {
        *pFileSize = iResult;
    }

    // close the file
    iResult = ZFileClose(iFileId);
    if (iResult < 0)
    {
        printf("zfile: error closing file '%s'\n", pFileName);
    }

    // return pointer to memory
    return(pFileMem);
}


/*F********************************************************************************/
/*!
    \Function ZFileSave

    \Description
        Save data to a file. (OVERWRITE and CREATE by default)

    \Input *pFilename   - pointer to name of file to read
    \Input *pData       - pointer to data to save to a file
    \Input  iSize       - amount of data to save to the file
    \Input uOpenFlags   - open flags, or zero for default (WRONLY|CREATE)

    \Output
        int32_t             - 0 if success, error code otherwise

    \Version 03/18/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t ZFileSave(const char *pFileName, const char *pData, int32_t iSize, uint32_t uOpenFlags)
{
    int32_t iResult;
    ZFileT iFileId;

    // determine what flags to use
    if (uOpenFlags == 0)
    {
        uOpenFlags = ZFILE_OPENFLAG_WRONLY | ZFILE_OPENFLAG_CREATE;
    }

    // open the file
    iFileId = ZFileOpen(pFileName, uOpenFlags);
    if (iFileId == ZFILE_INVALID)
    {
        ZPrintf("zfile: unable to open file '%s' with flags 0x%X\n", pFileName, uOpenFlags);
        return(ZFILE_ERROR_FILEOPEN);
    }

    // now write the data
    iResult = ZFileWrite(iFileId, (char *)pData, iSize);
    if(iResult != iSize)
    {
        ZPrintf("zfile: write error - size to write [%d] size written [%d]\n", iSize, iResult);
        return(ZFILE_ERROR_FILEWRITE);
    }

    // and close the file
    iResult = ZFileClose(iFileId);
    if(iResult != 0)
    {
        ZPrintf("zfile: close error [%d=0x%X]\n", iResult, iResult);
        return(ZFILE_ERROR_FILECLOSE);
    }

    // else successful save occurred
    return(ZFILE_ERROR_NONE);
}

