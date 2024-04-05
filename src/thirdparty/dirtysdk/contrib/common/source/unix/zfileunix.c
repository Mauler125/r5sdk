/*H********************************************************************************/
/*!
    \File zfilepc.c

    \Description
        Basic file operations (open, read, write, close, size).

    \Notes
        None.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 11/18/1004 (jbrookes) First Version
    \Version 1.1 03/16/2005 (jfrank)   Updates for common sample libraries
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/dirtylib.h"

#include "zfile.h"
#include "zlib.h"
#include "zmem.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _ZFilePathTranslate

    \Description
        Translate a path from dos-style to unix-style

    \Input *pPathName   - path name to translate
    \Input *pStrBuf     - [out] output for translated path
    \Input iBufLen      - size of output buffer

    \Output
        char *          - pointer to translated path

    \Version 10/10/2013 (jbrookes) 
*/
/********************************************************************************F*/
static char *_ZFilePathTranslate(const char *pPathName, char *pStrBuf, int32_t iBufLen)
{
    char *pPath;

    // copy pathname
    ds_strnzcpy(pStrBuf, pPathName, iBufLen);

    // translate any backward slashes to forward slashes
    for (pPath = pStrBuf; *pPath != '\0'; pPath += 1)
    {
        if (*pPath == '\\')
        {
            *pPath = '/';
        }
    }
    return(pStrBuf);
}

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function ZFileOpen

    \Description
        Open a file.

    \Input *pFileName   - file name to open
    \Input uFlags       - flags dictating how to open the file

    \Output
        int32_t             - file descriptor, or -1 if there was an error

    \Version 1.0 03/16/2005 (jfrank) First Version
*/
/********************************************************************************F*/
ZFileT ZFileOpen(const char *pFileName, uint32_t uFlags)
{
    char strFileName[4096];
    char strMode[16] = "";
    FILE *pFile;

    if ((pFileName == NULL) || (pFileName[0] == '\0'))
    {
        return(ZFILE_INVALID);
    }

    pFileName = _ZFilePathTranslate(pFileName, strFileName, sizeof(strFileName));

    ds_memclr(strMode, sizeof(strMode));

    // map zfile flags to Win32 Mode flags
    if (uFlags & ZFILE_OPENFLAG_APPEND)
    {
        strcat(strMode, "a");
        if (uFlags & ZFILE_OPENFLAG_RDONLY)
            strcat(strMode, "+");
    }
    else if (uFlags & ZFILE_OPENFLAG_RDONLY)
    {
        strcat(strMode, "r");
        if (uFlags & ZFILE_OPENFLAG_WRONLY)
            strcat(strMode, "+");
    }
    else if (uFlags & ZFILE_OPENFLAG_WRONLY)
    {
        strcat(strMode, "w");
    }
    if (uFlags & ZFILE_OPENFLAG_BINARY)
    {
        strcat(strMode, "b");
    }

    if ((pFile = fopen(pFileName, strMode)) != NULL)
    {
        return((ZFileT)pFile);
    }
    else
    {
        return(ZFILE_INVALID);
    }
}

/*F********************************************************************************/
/*!
    \Function ZFileClose

    \Description
        Close a file

    \Input iFileId  - file descriptor

    \Output    int32_t  - return value from fclose()

    \Version 1.0 11/18/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t ZFileClose(ZFileT iFileId)
{
    return(fclose((FILE *)iFileId));
}

/*F********************************************************************************/
/*!
    \Function ZFileRead

    \Description
        Read from a file.

    \Input *pData   - pointer to buffer to read to
    \Input iSize    - amount of data to read
    \Input iFileId  - file descriptor

    \Output
        Number of bytes read

    \Version 1.0 11/18/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t ZFileRead(ZFileT iFileId, void *pData, int32_t iSize)
{
    int32_t iResult = (int32_t)fread(pData, 1, iSize, (FILE *)iFileId);
    return(ferror((FILE *)iFileId) ? -1 : iResult);
}

/*F********************************************************************************/
/*!
    \Function ZFileWrite

    \Description
        Write to a file.

    \Input *pData   - pointer to buffer to write from
    \Input iSize    - amount of data to write
    \Input iFileId  - file descriptor

    \Output
        Number of bytes written

    \Version 1.0 11/18/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t ZFileWrite(ZFileT iFileId, void *pData, int32_t iSize)
{
    int32_t iResult;

    if (iFileId == ZFILE_INVALID)
    {
        return(0);
    }
    if ((pData == NULL) || (iSize == 0))
    {
        return(0);
    }

    iResult = (int32_t)fwrite(pData, 1, iSize, (FILE *)iFileId);
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function ZFileSeek

    \Description
        Seek to location in file.

    \Input iFileId  - file id to seek
    \Input iOffset  - offset to seek to
    \Input uFlags   - seek mode (ZFILE_SEEKFLAG_*)

    \Output
        int64_t         - resultant seek location, or -1 on error

    \Version 03/16/2005 (jfrank) First Version
*/
/********************************************************************************F*/
int64_t ZFileSeek(ZFileT iFileId, int64_t iOffset, uint32_t uFlags)
{
    int64_t iResult;
    int32_t iFlags=0;

    if (uFlags == ZFILE_SEEKFLAG_CUR)
    {
        iFlags = SEEK_CUR;
    }
    else if (uFlags == ZFILE_SEEKFLAG_END)
    {
        iFlags = SEEK_END;
    }
    else if (uFlags == ZFILE_SEEKFLAG_SET)
    {
        iFlags = SEEK_SET;
    }

    fseek((FILE *)iFileId, iOffset, iFlags);
    iResult = ftell((FILE *)iFileId);
    return((iResult >= 0) ? iResult : -1);
}

/*F********************************************************************************/
/*!
    \Function ZFileDelete

    \Description
        Delete a file.

    \Input *pFileName - filename of file to delete

    \Output int32_t - 0=success, error code otherwise

    \Version 03/23/2005 (jfrank) First Version
*/
/********************************************************************************F*/
int32_t ZFileDelete(const char *pFileName)
{
    char strFileName[4096];
    int32_t iResult;

    if ((pFileName == NULL) || (pFileName[0] == '\0'))
    {
        return(ZFILE_ERROR_FILENAME);
    }

    pFileName = _ZFilePathTranslate(pFileName, strFileName, sizeof(strFileName));

    iResult = remove(pFileName);
    if (iResult == 0)
    {
        return(ZFILE_ERROR_NONE);
    }
    else
    {
        return(ZFILE_ERROR_FILEDELETE);
    }
}

/*F********************************************************************************/
/*!
    \Function ZFileStat

    \Description
        Get File Stat information on a file/dir.

    \Input *pFileName - filename/dir to stat
    \Input *pStat

    \Output int32_t - 0=success, error code otherwise

    \Version 03/25/2005 (jfrank) First Version
*/
/********************************************************************************F*/
int32_t ZFileStat(const char *pFileName, ZFileStatT *pFileStat)
{
    #if !defined(DIRTYCODE_NX)
    char strFileName[4096];
    struct stat FileStat;
    int32_t iResult;

    // check for error conditions
    if (pFileName == NULL)
    {
        return(ZFILE_ERROR_FILENAME);
    }
    if (pFileStat == NULL)
    {
        return(ZFILE_ERROR_NULLPOINTER);
    }

    pFileName = _ZFilePathTranslate(pFileName, strFileName, sizeof(strFileName));

    // get file status
    iResult = stat(pFileName, &FileStat);

    // check for some specific errors
    if ((iResult == -1) && (errno == ENOENT))
    {
        return(ZFILE_ERROR_NOSUCHFILE);
    }
    else if (errno == EACCES)
    {
        return(ZFILE_ERROR_PERMISSION);
    }
    else if (iResult != 0)
    {
        return(ZFILE_ERROR_FILESTAT);
    }

    // clear the incoming buffer
    ds_memclr(pFileStat, sizeof(ZFileStatT));
    // copy from the PC-specific structures
    pFileStat->iSize = FileStat.st_size;
    pFileStat->uTimeAccess = (uint32_t)FileStat.st_atime;
    pFileStat->uTimeCreate = (uint32_t)FileStat.st_ctime;
    pFileStat->uTimeModify = (uint32_t)FileStat.st_mtime;
    // get the file modes
    if (S_ISDIR(FileStat.st_mode))
    {
        pFileStat->uMode |= ZFILESTAT_MODE_DIR;
    }
    if (S_ISREG(FileStat.st_mode))
    {
        pFileStat->uMode |= ZFILESTAT_MODE_FILE;
    }
    if (FileStat.st_mode & S_IRUSR)
    {
        pFileStat->uMode |= ZFILESTAT_MODE_READ;
    }
    if (FileStat.st_mode & S_IWUSR)
    {
        pFileStat->uMode |= ZFILESTAT_MODE_WRITE;
    }
    if (FileStat.st_mode & S_IXUSR)
    {
        pFileStat->uMode |= ZFILESTAT_MODE_EXECUTE;
    }
    #endif

    // done - return no error
    return(ZFILE_ERROR_NONE);
}

/*F********************************************************************************/
/*!
    \Function ZFileRename

    \Description
        Rename a file.

    \Input *pOldname - old name
    \Input *pNewname - new name

    \Output int32_t - 0=success, error code otherwise

    \Version 03/30/2005 (jfrank) First Version
*/
/********************************************************************************F*/
int32_t ZFileRename(const char *pOldname, const char *pNewname)
{
    int32_t iResult;

    // check for error conditions
    if ((pOldname == NULL) || (pNewname == NULL))
    {
        return(ZFILE_ERROR_NULLPOINTER);
    }

    // rename the file
    iResult = rename(pOldname, pNewname);

    return((iResult == 0) ? ZFILE_ERROR_NONE : ZFILE_ERROR_FILERENAME);
}

/*F********************************************************************************/
/*!
    \Function ZFileMkdir

    \Description
        Make a directory, recursively

    \Input *pPathName   - directory path to create

    \Output
        int32_t         - 0=success, error code otherwise

    \Version 01/25/2012 (jbrookes)
*/
/********************************************************************************F*/
int32_t ZFileMkdir(const char *pPathName)
{
    char strPathName[4096], *pPath, cTerm = '\0';

    pPath = _ZFilePathTranslate(pPathName, strPathName, sizeof(strPathName));

    // traverse pathname, making each directory component as we go
    for ( ; ; pPath += 1)
    {
        if (*pPath == '/')
        {
            cTerm = *pPath;
            *pPath = '\0';
        }
        if (*pPath == '\0')
        {
            int32_t iResult;
            if ((iResult = mkdir(strPathName, S_IRWXU)) != 0)
            {
                if (errno == EEXIST)
                {
                    ZPrintfDbg(("zfilepc: directory %s already exists\n", strPathName));
                }
                else
                {
                    ZPrintfDbg(("zfileunix: could not create directory '%s' err=%s\n", strPathName, DirtyErrGetName(errno)));
                    return(-1);
                }
            }
        }
        if (cTerm != '\0')
        {
            *pPath = cTerm;
            cTerm = '\0';
        }
        if (*pPath == '\0')
        {
            break;
        }
    }
    return(0);
}


