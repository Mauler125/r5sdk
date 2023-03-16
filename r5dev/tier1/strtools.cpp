#include "core/stdafx.h"
#include "tier1/strtools.h"

FORCEINLINE unsigned char tolower_fast(unsigned char c)
{
	if ((c >= 'A') && (c <= 'Z'))
		return c + ('a' - 'A');
	return c;
}

//-----------------------------------------------------------------------------
// Finds a string in another string with a case insensitive test
//-----------------------------------------------------------------------------
char const* V_stristr(char const* pStr, char const* pSearch)
{
	AssertValidStringPtr(reinterpret_cast<const TCHAR*>(pStr));
	AssertValidStringPtr(reinterpret_cast<const TCHAR*>(pSearch));

	if (!pStr || !pSearch)
		return 0;

	char const* pLetter = pStr;

	// Check the entire string
	while (*pLetter != 0)
	{
		// Skip over non-matches
		if (tolower_fast((unsigned char)*pLetter) == tolower_fast((unsigned char)*pSearch))
		{
			// Check for match
			char const* pMatch = pLetter + 1;
			char const* pTest = pSearch + 1;
			while (*pTest != 0)
			{
				// We've run off the end; don't bother.
				if (*pMatch == 0)
					return 0;

				if (tolower_fast((unsigned char)*pMatch) != tolower_fast((unsigned char)*pTest))
					break;

				++pMatch;
				++pTest;
			}

			// Found a match!
			if (*pTest == 0)
				return pLetter;
		}

		++pLetter;
	}

	return 0;
}

char* V_stristr(char* pStr, char const* pSearch)
{
	AssertValidStringPtr(reinterpret_cast<const TCHAR*>(pStr));
	AssertValidStringPtr(reinterpret_cast<const TCHAR*>(pSearch));

	return (char*)V_stristr((char const*)pStr, pSearch);
}

//-----------------------------------------------------------------------------
// Finds a string in another string with a case insensitive test w/ length validation
//-----------------------------------------------------------------------------
const char* V_strnistr(const char* pStr, const char* pSearch, int64_t n)
{
	Assert(pStr);
	Assert(pSearch);
	if (!pStr || !pSearch)
		return 0;

	const char* pLetter = pStr;

	// Check the entire string
	while (*pLetter != 0)
	{
		if (n <= 0)
			return 0;

		// Skip over non-matches
		if (FastASCIIToLower(*pLetter) == FastASCIIToLower(*pSearch))
		{
			int64_t n1 = n - 1;

			// Check for match
			const char* pMatch = pLetter + 1;
			const char* pTest = pSearch + 1;
			while (*pTest != 0)
			{
				if (n1 <= 0)
					return 0;

				// We've run off the end; don't bother.
				if (*pMatch == 0)
					return 0;

				if (FastASCIIToLower(*pMatch) != FastASCIIToLower(*pTest))
					break;

				++pMatch;
				++pTest;
				--n1;
			}

			// Found a match!
			if (*pTest == 0)
				return pLetter;
		}

		++pLetter;
		--n;
	}

	return 0;
}

const char* V_strnchr(const char* pStr, char c, int64_t n)
{
	const char* pLetter = pStr;
	const char* pLast = pStr + n;

	// Check the entire string
	while ((pLetter < pLast) && (*pLetter != 0))
	{
		if (*pLetter == c)
			return pLetter;
		++pLetter;
	}
	return NULL;
}

bool V_isspace(int c)
{
	// The standard white-space characters are the following: space, tab, carriage-return, newline, vertical tab, and form-feed. In the C locale, V_isspace() returns true only for the standard white-space characters. 
	//return c == ' ' || c == 9 /*horizontal tab*/ || c == '\r' || c == '\n' || c == 11 /*vertical tab*/ || c == '\f';
	// codes of whitespace symbols: 9 HT, 10 \n, 11 VT, 12 form feed, 13 \r, 32 space

	// easy to understand version, validated:
	// return ((1 << (c-1)) & 0x80001F00) != 0 && ((c-1)&0xE0) == 0;

	// 5% faster on Core i7, 35% faster on Xbox360, no branches, validated:
#ifdef _X360
	return ((1 << (c - 1)) & 0x80001F00 & ~(-int((c - 1) & 0xE0))) != 0;
#else
// this is 11% faster on Core i7 than the previous, VC2005 compiler generates a seemingly unbalanced search tree that's faster
	switch (c)
	{
	case ' ':
	case 9:
	case '\r':
	case '\n':
	case 11:
	case '\f':
		return true;
	default:
		return false;
	}
#endif
}

int V_StrTrim(char* pStr)
{
	char* pSource = pStr;
	char* pDest = pStr;

	// skip white space at the beginning
	while (*pSource != 0 && V_isspace(*pSource))
	{
		pSource++;
	}

	// copy everything else
	char* pLastWhiteBlock = NULL;
	char* pStart = pDest;
	while (*pSource != 0)
	{
		*pDest = *pSource++;
		if (V_isspace(*pDest))
		{
			if (pLastWhiteBlock == NULL)
				pLastWhiteBlock = pDest;
		}
		else
		{
			pLastWhiteBlock = NULL;
		}
		pDest++;
	}
	*pDest = 0;

	// did we end in a whitespace block?
	if (pLastWhiteBlock != NULL)
	{
		// yep; shorten the string
		pDest = pLastWhiteBlock;
		*pLastWhiteBlock = 0;
	}

	return pDest - pStart;
}

//-----------------------------------------------------------------------------
// Purpose: Converts a UTF8 string into a unicode string
//-----------------------------------------------------------------------------
int V_UTF8ToUnicode(const char* pUTF8, wchar_t* pwchDest, int cubDestSizeInBytes)
{
	Assert(cubDestSizeInBytes >= sizeof(*pwchDest));
	pwchDest[0] = 0;
	if (!pUTF8)
		return 0;
#ifdef _WIN32
	int cchResult = MultiByteToWideChar(CP_UTF8, 0, pUTF8, -1, pwchDest, cubDestSizeInBytes / sizeof(wchar_t));
#elif POSIX
	int cchResult = mbstowcs(pwchDest, pUTF8, cubDestSizeInBytes / sizeof(wchar_t));
#endif
	pwchDest[(cubDestSizeInBytes / sizeof(wchar_t)) - 1] = 0;
	return cchResult;
}

//-----------------------------------------------------------------------------
// Purpose: Converts a unicode string into a UTF8 (standard) string
//-----------------------------------------------------------------------------
int V_UnicodeToUTF8(const wchar_t* pUnicode, char* pUTF8, int cubDestSizeInBytes)
{
	if (cubDestSizeInBytes > 0)
		pUTF8[0] = 0;

#ifdef _WIN32
	int cchResult = WideCharToMultiByte(CP_UTF8, 0, pUnicode, -1, pUTF8, cubDestSizeInBytes, NULL, NULL);
#elif POSIX
	int cchResult = 0;
	if (pUnicode && pUTF8)
		cchResult = wcstombs(pUTF8, pUnicode, cubDestSizeInBytes);
#endif

	if (cubDestSizeInBytes > 0)
		pUTF8[cubDestSizeInBytes - 1] = 0;

	return cchResult;
}

bool V_StringMatchesPattern(const char* pszSource, const char* pszPattern, int nFlags /*= 0 */)
{
	bool bExact = true;
	while (1)
	{
		if ((*pszPattern) == 0)
		{
			return ((*pszSource) == 0);
		}

		if ((*pszPattern) == '*')
		{
			pszPattern++;

			if ((*pszPattern) == 0)
			{
				return true;
			}

			bExact = false;
			continue;
		}

		int nLength = 0;

		while ((*pszPattern) != '*' && (*pszPattern) != 0)
		{
			nLength++;
			pszPattern++;
		}

		while (1)
		{
			const char* pszStartPattern = pszPattern - nLength;
			const char* pszSearch = pszSource;

			for (int i = 0; i < nLength; i++, pszSearch++, pszStartPattern++)
			{
				if ((*pszSearch) == 0)
				{
					return false;
				}

				if ((*pszSearch) != (*pszStartPattern))
				{
					break;
				}
			}

			if (pszSearch - pszSource == nLength)
			{
				break;
			}

			if (bExact == true)
			{
				return false;
			}

			if ((nFlags & PATTERN_DIRECTORY) != 0)
			{
				if ((*pszPattern) != '/' && (*pszSource) == '/')
				{
					return false;
				}
			}

			pszSource++;
		}

		pszSource += nLength;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Changes all '/' or '\' characters into separator
// Input  : *pName - 
//			cSeparator - 
//-----------------------------------------------------------------------------
void V_FixSlashes(char* pName, char cSeperator /* = CORRECT_PATH_SEPARATOR */)
{
	while (*pName)
	{
		if (*pName == INCORRECT_PATH_SEPARATOR || *pName == CORRECT_PATH_SEPARATOR)
		{
			*pName = cSeperator;
		}
		pName++;
	}
}

void V_AppendSlash(char* pStr, int strSize, char separator)
{
	int len = V_strlen(pStr);
	if (len > 0 && !PATHSEPARATOR(pStr[len - 1]))
	{
		if (len + 1 >= strSize)
			Error(eDLL_T::COMMON, EXIT_FAILURE, "V_AppendSlash: ran out of space on %s.", pStr);

		pStr[len] = separator;
		pStr[len + 1] = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *ppath - 
//-----------------------------------------------------------------------------
void V_StripTrailingSlash(char* ppath)
{
	Assert(ppath);

	int len = V_strlen(ppath);
	if (len > 0)
	{
		if (PATHSEPARATOR(ppath[len - 1]))
		{
			ppath[len - 1] = 0;
		}
	}
}

bool V_RemoveDotSlashes(char* pFilename, char separator)
{
	// Remove '//' or '\\'
	char* pIn = pFilename;
	char* pOut = pFilename;

	// (But skip a leading separator, for leading \\'s in network paths)
	if (*pIn && PATHSEPARATOR(*pIn))
	{
		*pOut = *pIn;
		++pIn;
		++pOut;
	}

	bool bPrevPathSep = false;
	while (*pIn)
	{
		bool bIsPathSep = PATHSEPARATOR(*pIn);
		if (!bIsPathSep || !bPrevPathSep)
		{
			*pOut++ = *pIn;
		}
		bPrevPathSep = bIsPathSep;
		++pIn;
	}
	*pOut = 0;

	// Get rid of "./"'s
	pIn = pFilename;
	pOut = pFilename;
	while (*pIn)
	{
		// The logic on the second line is preventing it from screwing up "../"
		if (pIn[0] == '.' && PATHSEPARATOR(pIn[1]) &&
			(pIn == pFilename || pIn[-1] != '.'))
		{
			pIn += 2;
		}
		else
		{
			*pOut = *pIn;
			++pIn;
			++pOut;
		}
	}
	*pOut = 0;

	// Get rid of a trailing "/." (needless).
	int len = V_strlen(pFilename);
	if (len > 2 && pFilename[len - 1] == '.' && PATHSEPARATOR(pFilename[len - 2]))
	{
		pFilename[len - 2] = 0;
	}

	// Each time we encounter a "..", back up until we've read the previous directory name,
	// then get rid of it.
	pIn = pFilename;
	while (*pIn)
	{
		if (pIn[0] == '.' &&
			pIn[1] == '.' &&
			(pIn == pFilename || PATHSEPARATOR(pIn[-1])) &&	// Preceding character must be a slash.
			(pIn[2] == 0 || PATHSEPARATOR(pIn[2])))			// Following character must be a slash or the end of the string.
		{
			char* pEndOfDots = pIn + 2;
			char* pStart = pIn - 2;

			// Ok, now scan back for the path separator that starts the preceding directory.
			while (1)
			{
				if (pStart < pFilename)
					return false;

				if (PATHSEPARATOR(*pStart))
					break;

				--pStart;
			}

			// Now slide the string down to get rid of the previous directory and the ".."
			memmove(pStart, pEndOfDots, strlen(pEndOfDots) + 1);

			// Start over.
			pIn = pFilename;
		}
		else
		{
			++pIn;
		}
	}

	V_FixSlashes(pFilename, separator);
	return true;
}

//-----------------------------------------------------------------------------
// small helper function shared by lots of modules
//-----------------------------------------------------------------------------
bool V_IsAbsolutePath(const char* pStr)
{
	if (!(pStr[0] && pStr[1]))
		return false;

#if defined( PLATFORM_WINDOWS )
	bool bIsAbsolute = (pStr[0] && pStr[1] == ':') ||
		((pStr[0] == '/' || pStr[0] == '\\') && (pStr[1] == '/' || pStr[1] == '\\'));
#else
	bool bIsAbsolute = (pStr[0] && pStr[1] == ':') || pStr[0] == '/' || pStr[0] == '\\';
#endif

	if (IsX360() && !bIsAbsolute)
	{
		bIsAbsolute = (V_stristr(pStr, ":") != NULL);
	}

	return bIsAbsolute;
}

#if defined(_MSC_VER) && _MSC_VER >= 1900
bool
#else
void
#endif
V_MakeAbsolutePath(char* pOut, int outLen, const char* pPath, const char* pStartingDir)
{
	if (V_IsAbsolutePath(pPath))
	{
		// pPath is not relative.. just copy it.
		V_strncpy(pOut, pPath, outLen);
	}
	else
	{
		// Make sure the starting directory is absolute..
		if (pStartingDir && V_IsAbsolutePath(pStartingDir))
		{
			V_strncpy(pOut, pStartingDir, outLen);
		}
		else
		{
#ifdef _PS3 
			{
				V_strncpy(pOut, g_pPS3PathInfo->GameImagePath(), outLen);
			}
#else
			{
				if (!_getcwd(pOut, outLen))
					Error(eDLL_T::COMMON, EXIT_FAILURE, "V_MakeAbsolutePath: _getcwd failed.");
			}
#endif

			if (pStartingDir)
			{
				V_AppendSlash(pOut, outLen);
				V_strncat(pOut, pStartingDir, outLen/*, COPY_ALL_CHARACTERS*/);
			}
		}

		// Concatenate the paths.
		V_AppendSlash(pOut, outLen);
		V_strncat(pOut, pPath, outLen/*, COPY_ALL_CHARACTERS*/);
	}

	V_FixSlashes(pOut);

	bool bRet = true;
	if (!V_RemoveDotSlashes(pOut))
	{
		V_strncpy(pOut, pPath, outLen);
		V_FixSlashes(pOut);
		bRet = false;
	}

#if defined(_MSC_VER) && _MSC_VER >= 1900
	return bRet;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Strip off the last directory from dirName
// Input  : *dirName - 
//			maxlen - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool V_StripLastDir(char* dirName, int maxlen)
{
	if (dirName[0] == 0 ||
		!V_stricmp(dirName, "./") ||
		!V_stricmp(dirName, ".\\"))
		return false;

	int len = V_strlen(dirName);

	Assert(len < maxlen);

	// skip trailing slash
	if (PATHSEPARATOR(dirName[len - 1]))
	{
		len--;
	}

	bool bHitColon = false;
	while (len > 0)
	{
		if (PATHSEPARATOR(dirName[len - 1]))
		{
			dirName[len] = 0;
			V_FixSlashes(dirName, CORRECT_PATH_SEPARATOR);
			return true;
		}
		else if (dirName[len - 1] == ':')
		{
			bHitColon = true;
		}

		len--;
	}

	// If we hit a drive letter, then we're done.
	// Ex: If they passed in c:\, then V_StripLastDir should return "" and false.
	if (bHitColon)
	{
		dirName[0] = 0;
		return false;
	}

	// Allow it to return an empty string and true. This can happen if something like "tf2/" is passed in.
	// The correct behavior is to strip off the last directory ("tf2") and return true.
	if (len == 0 && !bHitColon)
	{
		V_snprintf(dirName, maxlen, ".%c", CORRECT_PATH_SEPARATOR);
		return true;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Returns a pointer to the beginning of the unqualified file name 
//			(no path information)
// Input:	in - file name (may be unqualified, relative or absolute path)
// Output:	pointer to unqualified file name
//-----------------------------------------------------------------------------
const char* V_UnqualifiedFileName(const char* in)
{
	if (!in || !in[0])
		return in;

	// back up until the character after the first path separator we find,
	// or the beginning of the string
	const char* out = in + strlen(in) - 1;
	while ((out > in) && (!PATHSEPARATOR(*(out - 1))))
		out--;
	return out;
}

//-----------------------------------------------------------------------------
// Purpose: Composes a path and filename together, inserting a path separator
//			if need be
// Input:	path - path to use
//			filename - filename to use
//			dest - buffer to compose result in
//			destSize - size of destination buffer
//-----------------------------------------------------------------------------
void V_ComposeFileName(const char* path, const char* filename, char* dest, int destSize)
{
	V_strncpy(dest, path, destSize);
	V_FixSlashes(dest);
	V_AppendSlash(dest, destSize);
	V_strncat(dest, filename, destSize/*, COPY_ALL_CHARACTERS*/);
	V_FixSlashes(dest);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *in - 
//			*out - 
//			outSize - 
//-----------------------------------------------------------------------------
void V_StripExtension(const char* in, char* out, int outSize)
{
	// Find the last dot. If it's followed by a dot or a slash, then it's part of a 
	// directory specifier like ../../somedir/./blah.

	// scan backward for '.'
	int end = V_strlen(in) - 1;
	while (end > 0 && in[end] != '.' && !PATHSEPARATOR(in[end]))
	{
		--end;
	}

	if (end > 0 && !PATHSEPARATOR(in[end]) && end < outSize)
	{
		int nChars = MIN(end, outSize - 1);
		if (out != in)
		{
			memcpy(out, in, nChars);
		}
		out[nChars] = 0;
	}
	else
	{
		// nothing found
		if (out != in)
		{
			V_strncpy(out, in, outSize);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *path - 
//			*dest - 
//			destSize - 
// Output : void V_ExtractFileExtension
//-----------------------------------------------------------------------------
void V_ExtractFileExtension(const char* path, char* dest, int destSize)
{
	*dest = 0;
	const char* extension = V_GetFileExtension(path);
	if (NULL != extension)
		V_strncpy(dest, extension, destSize);
}

//-----------------------------------------------------------------------------
// Purpose: Returns a pointer to the file extension within a file name string
// Input:	in - file name 
// Output:	pointer to beginning of extension (after the "."), or NULL
//				if there is no extension
//-----------------------------------------------------------------------------
const char* V_GetFileExtension(const char* path)
{
	int len = V_strlen(path);
	if (len <= 1)
		return NULL;

	const char* src = path + len - 1;

	//
	// back up until a . or the start
	//
	while (src != path && *(src - 1) != '.')
		src--;

	// check to see if the '.' is part of a pathname
	if (src == path || PATHSEPARATOR(*src))
	{
		return NULL;  // no extension
	}

	return src;
}


//-----------------------------------------------------------------------------
// Purpose: Extracts the base name of a file (no path, no extension, assumes '/' or '\' as path separator)
// Input  : *in - 
//			*out - 
//			maxlen - 
//-----------------------------------------------------------------------------
void V_FileBase(const char* in, char* out, int maxlen)
{
	Assert(maxlen >= 1);
	Assert(in);
	Assert(out);

	if (!in || !in[0])
	{
		*out = 0;
		return;
	}

	int len, start, end;

	len = V_strlen(in);

	// scan backward for '.'
	end = len - 1;
	while (end && in[end] != '.' && !PATHSEPARATOR(in[end]))
	{
		end--;
	}

	if (in[end] != '.')		// no '.', copy to end
	{
		end = len - 1;
	}
	else
	{
		end--;					// Found ',', copy to left of '.'
	}

	// Scan backward for '/'
	start = len - 1;
	while (start >= 0 && !PATHSEPARATOR(in[start]))
	{
		start--;
	}

	if (start < 0 || !PATHSEPARATOR(in[start]))
	{
		start = 0;
	}
	else
	{
		start++;
	}

	// Length of new sting
	len = end - start + 1;

	int maxcopy = MIN(len + 1, maxlen);

	// Copy partial string
	V_strncpy(out, &in[start], maxcopy);
}
