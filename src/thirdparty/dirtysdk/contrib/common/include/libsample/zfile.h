/*H********************************************************************************/
/*!
    \File Zfile.h

    \Description
        Host file operations.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 02/16/2005 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _Zfile_h
#define _Zfile_h

/*** Include files ****************************************************************/

#if (defined(DIRTYCODE_LINUX)) || (defined(DIRTYCODE_APPLEIOS))
#include <stdio.h>
#endif

/*** Defines **********************************************************************/

#if (defined(DIRTYCODE_LINUX)) || (defined(DIRTYCODE_APPLEIOS))
#define ZFILE_INVALID               (NULL)
#else
#define ZFILE_INVALID               (-1)    //!< an invalid ZfileT handle
#endif

#define ZFILE_OPENFLAG_RDONLY       (1)
#define ZFILE_OPENFLAG_WRONLY       (2)
#define ZFILE_OPENFLAG_RDWR         (ZFILE_OPENFLAG_RDONLY|ZFILE_OPENFLAG_WRONLY)
#define ZFILE_OPENFLAG_CREATE       (4)
#define ZFILE_OPENFLAG_BINARY       (8)
#define ZFILE_OPENFLAG_APPEND       (16)

#define ZFILE_SEEKFLAG_CUR          (1)
#define ZFILE_SEEKFLAG_END          (2)
#define ZFILE_SEEKFLAG_SET          (3)

#define ZFILE_ERROR_NONE            (0)     //!< no error
#define ZFILE_ERROR_FILEOPEN        (-1)    //!< generic error opening the file (reading or writing)
#define ZFILE_ERROR_FILECLOSE       (-2)    //!< generic error closing the file
#define ZFILE_ERROR_FILEWRITE       (-3)    //!< generic error occurred writing to the file
#define ZFILE_ERROR_FILEDELETE      (-4)    //!< generic error deleting the file
#define ZFILE_ERROR_FILESTAT        (-5)    //!< generic error trying to fstat a file
#define ZFILE_ERROR_FILERENAME      (-6)    //!< generic error renaming a file
#define ZFILE_ERROR_FILENAME        (-7)    //!< bad filename
#define ZFILE_ERROR_NULLPOINTER     (-8)    //!< null pointer passed in where data was expected
#define ZFILE_ERROR_NOSUCHFILE      (-9)    //!< file does not exist
#define ZFILE_ERROR_PERMISSION      (-10)   //!< permission denied (on opening/writing)

#define ZFILE_PATHFILE_LENGTHMAX    (512)   //!< max length of a path/filename string

/*** Macros ***********************************************************************/

#define ZFILESTAT_MODE_READ             (0x01)      //!< file has read flag set
#define ZFILESTAT_MODE_WRITE            (0x02)      //!< file has write flag set
#define ZFILESTAT_MODE_EXECUTE          (0x04)      //!< file has execute flag set
#define ZFILESTAT_MODE_PERMISSIONMASK   (0x07)      //!< mask for mode flags

#define ZFILESTAT_MODE_FILE             (0x10)      //!< item is a file
#define ZFILESTAT_MODE_DIR              (0x20)      //!< item is a directory
#define ZFILESTAT_MODE_FILETYPEMASK     (0x30)      //!< mask for file type flags

/*** Type Definitions *************************************************************/

typedef struct ZFileStatT
{
    int64_t iSize;                  //!< file size in bytes
    uint32_t uTimeCreate;           //!< file creation time, seconds since epoch
    uint32_t uTimeAccess;           //!< last access time, seconds since epoch
    uint32_t uTimeModify;           //!< last modification time, seconds since epoch
    uint16_t uMode;                 //!< file mode (file/dir and R/W/X)
    uint16_t uPad1;                 //!< pad out to even boundary
} ZFileStatT;

#if !defined(DIRTYCODE_LINUX) && !defined(DIRTYCODE_APPLEIOS)
typedef intptr_t ZFileT;
#else
typedef FILE * ZFileT;
#endif

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/*
 Platform-specific implementations (Zfileplatform.c)
*/

// open a file
DIRTYCODE_API ZFileT ZFileOpen(const char *pFileName, uint32_t uFlags);

// close a file
DIRTYCODE_API int32_t ZFileClose(ZFileT iFileId);

// read from a file
DIRTYCODE_API int32_t ZFileRead(ZFileT iFileId, void *pData, int32_t iSize);

// write to a file
DIRTYCODE_API int32_t ZFileWrite(ZFileT iFileId, void *pData, int32_t iSize);

// seek in a file
DIRTYCODE_API int64_t ZFileSeek(ZFileT iFileId, int64_t iOffset, uint32_t uFlags);

// delete a file
DIRTYCODE_API int32_t ZFileDelete(const char *pFileName);

// get file status information
DIRTYCODE_API int32_t ZFileStat(const char *pFileName, ZFileStatT *pFileStat);

// rename a file
DIRTYCODE_API int32_t ZFileRename(const char *pOldname, const char *pNewname);

// create a directory
DIRTYCODE_API int32_t ZFileMkdir(const char *pPathName);

/*
 Platform-inspecific implementations (Zfile.c)
*/

// get file size
DIRTYCODE_API int64_t ZFileSize(ZFileT iFileId);

// open a file and load it into memory and null-terminate (in case it is a text file)
DIRTYCODE_API char *ZFileLoad(const char *pFileName, int32_t *pFileSize, uint32_t bBinary);

// save (overwrite) null-terminated data to a file
DIRTYCODE_API int32_t ZFileSave(const char *pFileName, const char *pData, int32_t iSize, uint32_t uFlags);

#ifdef __cplusplus
};
#endif

#endif // _Zfile_h

