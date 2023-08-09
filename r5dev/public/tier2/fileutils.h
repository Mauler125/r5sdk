//===== Copyright © 2005-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Helper methods + classes for file access.
//
//===========================================================================//
#ifndef FILEUTILS_H
#define FILEUTILS_H

#if defined( _WIN32 )
#pragma once
#endif
#include "filesystem/filesystem.h"

// Builds a directory which is a subdirectory of the current mod.
void GetModSubdirectory( const char *pSubDir, char *pBuf, ssize_t nBufLen );

// Builds a directory which is a subdirectory of the current mod's *content*.
void GetModContentSubdirectory( const char *pSubDir, char *pBuf, ssize_t nBufLen );

// Generates a filename under the 'game' subdirectory given a subdirectory of 'content'.
void ComputeModFilename( const char *pContentFileName, char *pBuf, ssize_t nBufLen );

// Generates a filename under the 'content' subdirectory given a subdirectory of 'game'.
void ComputeModContentFilename( const char *pGameFileName, char *pBuf, ssize_t nBufLen );

// Finds all files matching the a name within a directory and its sub directories. Output entries are paths to found files (relative to and including szStartDirectory).
void RecursiveFindFilesMatchingName( CUtlVector< CUtlString > &fileList, const char* szStartDirectory, const char* szTargetFileName, const char *pPathID, char separator = CORRECT_PATH_SEPARATOR);

// Builds a list of all files under a directory with a particular extension.
void AddFilesToList( CUtlVector< CUtlString > &fileList, const char *pDirectory, const char *pExtension = nullptr, const char* pPathID = nullptr, char separator = CORRECT_PATH_SEPARATOR );

// Returns the search path as a list of paths.
void GetSearchPath( CUtlVector< CUtlString > &pathList, const char *pPathID );

// Given file name generate a full path using the following rules.
// 1. if its full path already return.
// 2. if its a relative path try to find it under the path id.
// 3. if find fails treat relative path as relative to the current dir.
bool GenerateFullPath( const char *pFileName, char const *pPathID, char *pBuf, ssize_t nBufLen );

#endif // FILEUTILS_H
