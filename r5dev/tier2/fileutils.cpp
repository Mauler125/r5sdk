//===== Copyright © 2005-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Helper methods + classes for file access.
//
//===========================================================================//
#include "tier1/strtools.h"
#include "tier1/utlbuffer.h"
#include "tier2/fileutils.h"
#include "filesystem/filesystem.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Builds a directory which is a subdirectory of the current mod.
// Input  : *pSubDir -
//			*pBuf -
//			nBufLen - 
//-----------------------------------------------------------------------------
void GetModSubdirectory(const char* pSubDir, char* pBuf, ssize_t nBufLen)
{
	// Compute starting directory.
	Assert(FileSystem()->GetSearchPath( "MOD", false, NULL, 0) < nBufLen );
	FileSystem()->GetSearchPath( "MOD", false, pBuf, nBufLen );
	char* pSemi = strchr( pBuf, ';' );
	if ( pSemi )
	{
		*pSemi = 0;
	}

	V_StripTrailingSlash( pBuf );
	size_t currentLength = strlen( pBuf );

	if ( pSubDir )
	{
		Q_strncat( pBuf, "\\", nBufLen-currentLength );
		currentLength += 1; // Account for the added '\\' character.
		Q_strncat( pBuf, pSubDir, nBufLen-currentLength );
	}

	V_FixSlashes( pBuf );
}

//-----------------------------------------------------------------------------
// Purpose: Builds a directory which is a subdirectory of the current mod's *content*.
// Input  : *pSubDir -
//			*pBuf - 
//			nBufLen - 
//-----------------------------------------------------------------------------
void GetModContentSubdirectory( const char *pSubDir, char *pBuf, ssize_t nBufLen )
{
	char pTemp[ MAX_PATH ];
	GetModSubdirectory( pSubDir, pTemp, sizeof(pTemp) );
	ComputeModContentFilename( pTemp, pBuf, nBufLen );
}

//-----------------------------------------------------------------------------
// Purpose: Generates a filename under the 'game' subdirectory given a subdirectory of 'content'.
// Input  : *pContentFileName -
//			*pBuf - 
//			nBufLen - 
//-----------------------------------------------------------------------------
void ComputeModFilename( const char *pContentFileName, char *pBuf, ssize_t nBufLen )
{
	char pRelativePath[ MAX_PATH ];
	if ( !FileSystem()->FullPathToRelativePath(pContentFileName, pRelativePath, sizeof(pRelativePath)))
	{
		Q_strncpy( pBuf, pContentFileName, nBufLen );
		return;
	}

	char pGameRoot[ MAX_PATH ];
	FileSystem()->GetSearchPath("GAME", false, pGameRoot, sizeof(pGameRoot));
	char *pSemi = strchr( pGameRoot, ';' );
	if ( pSemi )
	{
		*pSemi = 0;
	}

	V_ComposeFileName( pGameRoot, pRelativePath, pBuf, nBufLen );
}

//-----------------------------------------------------------------------------
// Purpose: Generates a filename under the 'content' subdirectory given a subdirectory of 'game'.
// Input  : *pGameFileName -
//			*pBuf - 
//			nBufLen - 
//-----------------------------------------------------------------------------
void ComputeModContentFilename( const char *pGameFileName, char *pBuf, ssize_t nBufLen )
{
	char pRelativePath[ MAX_PATH ];
	if ( !FileSystem()->FullPathToRelativePath( pGameFileName, pRelativePath, sizeof(pRelativePath) ) )
	{
		Q_strncpy( pBuf, pGameFileName, nBufLen );
		return;
	}

	char pContentRoot[ MAX_PATH ];
	FileSystem()->GetSearchPath( "CONTENT", false, pContentRoot, sizeof(pContentRoot) );
	char *pSemi = strchr( pContentRoot, ';' );
	if ( pSemi )
	{
		*pSemi = 0;
	}

	V_ComposeFileName( pContentRoot, pRelativePath, pBuf, nBufLen );
}

//-----------------------------------------------------------------------------
// Purpose: Search start directory, recurse into sub directories collecting all files matching the target name.
// Input  : &fileList - 
//			*szStartDirectory - 
//			*szTargetFileName - 
//			*pathID - 
//			separator - 
//-----------------------------------------------------------------------------
void RecursiveFindFilesMatchingName( CUtlVector< CUtlString > &fileList, const char* szStartDirectory, const char* szTargetFileName, const char *pPathID, char separator )
{
	char searchString[MAX_PATH];
	Q_snprintf( searchString, sizeof( searchString ), "%s/*.*", szStartDirectory );
	V_FixSlashes( searchString );
	
	FileFindHandle_t handle;
	const char* curFile = FileSystem()->FindFirstEx( searchString, pPathID, &handle );
	while ( curFile )
	{
		if ( *curFile != '.' && FileSystem()->FindIsDirectory( handle ) )
		{	
			char newSearchPath[MAX_PATH];
			Q_snprintf( newSearchPath, sizeof( newSearchPath ), "%s/%s", szStartDirectory, curFile );
			RecursiveFindFilesMatchingName( fileList, newSearchPath, szTargetFileName, pPathID, separator);
		}
		else if ( V_StringMatchesPattern( curFile, szTargetFileName ) )
		{
			CUtlString outFile;
			outFile.Format( "%s/%s", szStartDirectory, curFile );
			V_FixSlashes( outFile.Get(), separator );
			fileList.AddToTail( outFile );
		}

		curFile = FileSystem()->FindNext( handle );
	}
	FileSystem()->FindClose( handle );
}

//-----------------------------------------------------------------------------
// Builds a list of all files under a directory with a particular extension.
// Input  : &fileList - 
//			*pDirectory - 
//			*pExtension - 
//			*pPathID - 
//			separator - 
//-----------------------------------------------------------------------------
void AddFilesToList( CUtlVector< CUtlString > &fileList, const char *pDirectory, const char *pExtension, const char* pPathID, char separator )
{
	char pSearchString[MAX_PATH];
	Q_snprintf( pSearchString, MAX_PATH, "%s/*", pDirectory );

	bool bIsAbsolute = V_IsAbsolutePath( pDirectory );

	// get the list of files.
	FileFindHandle_t hFind;
	const char *pFoundFile = FileSystem()->FindFirstEx( pSearchString, pPathID, &hFind );

	// add all the items.
	CUtlVector< CUtlString > subDirs;
	for ( ; pFoundFile; pFoundFile = FileSystem()->FindNext( hFind ) )
	{
		char pChildPath[MAX_PATH];
		Q_snprintf( pChildPath, MAX_PATH, "%s/%s", pDirectory, pFoundFile );

		if ( FileSystem()->FindIsDirectory( hFind ) )
		{
			if ( Q_strnicmp( pFoundFile, ".", 2 ) && Q_strnicmp( pFoundFile, "..", 3 ) )
			{
				subDirs.AddToTail( pChildPath );
			}
			continue;
		}

		// Check the extension matches.
		if ( pExtension && Q_stricmp( V_GetFileExtension( pFoundFile ), pExtension ) )
			continue;

		char pFullPathBuf[MAX_PATH];
		char *pFullPath = pFullPathBuf;
		if ( !bIsAbsolute )
		{
			FileSystem()->RelativePathToFullPath( pChildPath, pPathID, pFullPathBuf, sizeof(pFullPathBuf) );
		}
		else
		{
			pFullPath = pChildPath;
		}

		V_strlower( pFullPath );
		V_FixSlashes( pFullPath, separator );
		fileList.AddToTail( pFullPath );
	}

	FileSystem()->FindClose(hFind);

	int nCount = subDirs.Count();
	for ( int i = 0; i < nCount; ++i )
	{
		AddFilesToList( fileList, subDirs[i], pExtension, pPathID, separator );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns the search path as a list of paths.
// Input  : &pathList -
//			*pPathID - 
//-----------------------------------------------------------------------------
void GetSearchPath(CUtlVector< CUtlString >& pathList, const char* pPathID)
{
	const ssize_t nMaxLen = FileSystem()->GetSearchPath(pPathID, false, NULL, 0);
	char* pBuf = (char*)stackalloc(nMaxLen);
	FileSystem()->GetSearchPath(pPathID, false, pBuf, nMaxLen);

	char* pSemi;
	while (NULL != (pSemi = strchr(pBuf, ';')))
	{
		*pSemi = 0;
		pathList.AddToTail(pBuf);
		pBuf = pSemi + 1;
	}
	pathList.AddToTail(pBuf);
}

//-----------------------------------------------------------------------------
// Purpose: Given file name in the current dir generate a full path to it.
// Input  : *pFileName -
//			*pPathID - 
//			*pBuf - 
//			nBufLen - 
// Output: True on success, false otherwise.
//-----------------------------------------------------------------------------
bool GenerateFullPath(const char* pFileName, char const* pPathID, char* pBuf, ssize_t nBufLen)
{
	if (V_IsAbsolutePath(pFileName))
	{
		V_strncpy(pBuf, pFileName, nBufLen);
		return true;
	}

	const char* pFullPath = FileSystem()->RelativePathToFullPath(pFileName, pPathID, pBuf, nBufLen);
	if (pFullPath && V_IsAbsolutePath(pFullPath))
		return true;

	char pDir[MAX_PATH];
	if (!FileSystem()->GetCurrentDirectory(pDir, sizeof(pDir)))
		return false;

	V_ComposeFileName(pDir, pFileName, pBuf, nBufLen);
	V_RemoveDotSlashes(pBuf);
	return true;
}
