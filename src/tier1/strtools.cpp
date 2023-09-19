#include "tier1/strtools.h"

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
		if (FastASCIIToLower((unsigned char)*pLetter) == FastASCIIToLower((unsigned char)*pSearch))
		{
			// Check for match
			char const* pMatch = pLetter + 1;
			char const* pTest = pSearch + 1;
			while (*pTest != 0)
			{
				// We've run off the end; don't bother.
				if (*pMatch == 0)
					return 0;

				if (FastASCIIToLower((unsigned char)*pMatch) != FastASCIIToLower((unsigned char)*pTest))
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
const char* V_strnistr(const char* pStr, const char* pSearch, ssize_t n)
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
			ssize_t n1 = n - 1;

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

const char* V_strnchr(const char* pStr, char c, ssize_t n)
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

ssize_t V_StrTrim(char* pStr)
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
// Purpose: Converts a UTF-8 string into a unicode string
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
// Purpose: Converts a unicode string into a UTF-8 (standard) string
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

//-----------------------------------------------------------------------------
// Purpose: Returns the UTF-8 character length
//-----------------------------------------------------------------------------
int V_UTF8CharLength(const unsigned char input)
{
	if ((input & 0xFE) == 0xFC)
		return 6;
	if ((input & 0xFC) == 0xF8)
		return 5;
	if ((input & 0xF8) == 0xF0)
		return 4;
	else if ((input & 0xF0) == 0xE0)
		return 3;
	else if ((input & 0xE0) == 0xC0)
		return 2;
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Checks if a given string only contains UTF-8 characters
//-----------------------------------------------------------------------------
bool V_IsValidUTF8(const char* pszString)
{
	char c;
	const char* it;

	while (true)
	{
		while (true)
		{
			c = *pszString;
			it = pszString++;
			if (c < 0)
			{
				break;
			}
			if (!c)
			{
				return true;
			}
		}

		char s = *pszString;
		if ((*pszString & 0xC0) != 0x80)
		{
			break;
		}

		pszString = it + 2;
		if (c >= 0xE0u)
		{
			int n = (*pszString & 0x3F) | (((s & 0x3F) | ((c & 0xF) << 6)) << 6);
			if ((*pszString & 0xC0) != 0x80)
			{
				return false;
			}

			pszString = it + 3;
			if (c >= 0xF0u)
			{
				if ((*pszString & 0xC0) != 0x80 || ((n << 6) | (*pszString & 0x3Fu)) > 0x10FFFF)
				{
					return false;
				}

				pszString = it + 4;
			}
			else if ((n - 0xD800) <= 0x7FF)
			{
				return false;
			}
		}
		else if (c < 0xC2u)
		{
			return false;
		}
	}
	return false;
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

		ptrdiff_t nLength = 0;

		while ((*pszPattern) != '*' && (*pszPattern) != 0)
		{
			nLength++;
			pszPattern++;
		}

		while (1)
		{
			const char* pszStartPattern = pszPattern - nLength;
			const char* pszSearch = pszSource;

			for (ptrdiff_t i = 0; i < nLength; i++, pszSearch++, pszStartPattern++)
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
// Purpose: Compares file paths, ignores case and path separators
// Input  : *a - 
//          *b - 
// Output : true if equal, false otherwise
//-----------------------------------------------------------------------------
bool V_ComparePath(const char* a, const char* b)
{
	if (strlen(a) != strlen(b))
	{
		return false;
	}

	// Case and separator invariant
	for (; *a; a++, b++)
	{
		if (*a == *b)
		{
			continue;
		}
		if (FastASCIIToLower(*a) == FastASCIIToLower(*b))
		{
			continue;
		}
		if ((*a == '/' || *a == '\\') &&
			(*b == '/' || *b == '\\'))
		{
			continue;
		}
		return false;
	}
	return true;
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

void V_AppendSlash(char* pStr, size_t strSize, char separator)
{
	size_t len = V_strlen(pStr);
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

	size_t len = V_strlen(ppath);
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
	size_t len = V_strlen(pFilename);
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
// Purpose: normalizes the file path
// Input  : *pfilePath - 
//			separator - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool V_NormalizePath(char* pfilePath, char separator)
{
	char v2; // al
	char v3; // r9
	char* v5; // rbx
	char* i; // r8
	char v7; // dl
	char* v8; // r8
	char v9; // al
	char* j; // rcx
	char v11; // dl
	__int64 v12; // rax
	__int64 v13; // rax
	char v14; // cl
	char v15; // al
	char* v16; // rcx
	char v17; // al
	char v18; // al
	_BYTE* v19; // rdx
	char* k; // rcx
	__int64 v21; // r8
	char l; // al

	v2 = *pfilePath;
	v3 = 0;
	v5 = pfilePath;
	for (i = pfilePath; v2; v3 = v7)
	{
		if (v2 == '\\')
		{
			v2 = '\\';
		}
		else if (v2 != '/')
		{
			v7 = 0;
		LABEL_7:
			*pfilePath++ = v2;
			goto LABEL_8;
		}
		v7 = 1;
		if (!v3)
			goto LABEL_7;
	LABEL_8:
		v2 = *++i;
	}
	*pfilePath = 0;
	v8 = v5;
	v9 = *v5;
	for (j = v5; *j; v9 = *j)
	{
		if (v9 == '.' && ((v11 = j[1], v11 == '\\') || v11 == '/') && (j == v5 || (v9 = '.', *(j - 1) != '.')))
		{
			v12 = 2i64;
		}
		else
		{
			*v8++ = v9;
			v12 = 1i64;
		}
		j += v12;
	}
	*v8 = 0;
	v13 = -1i64;
	do
		++v13;
	while (v5[v13]);
	if (v13 > 2 && v5[v13 - 1] == '.')
	{
		v14 = v5[v13 - 2];
		if (v14 == '\\' || v14 == '/')
			v5[v13 - 2] = 0;
	}
	v15 = *v5;
	v16 = v5;
	if (*v5)
	{
		do
		{
			if (v15 == '.' && v16[1] == '.' && 
				(v16 == v5 || (v17 = *(v16 - 1), v17 == '\\') || v17 == '/') &&
				((v18 = v16[2], v19 = (uint8_t*)v16 + 2, !v18) || v18 == '\\' || v18 == '/'))
			{
				for (k = v16 - 2; ; --k)
				{
					if (k < v5)
						return false;
					if (*k == '\\' || *k == '/')
						break;
				}
				v21 = -1i64;
				do
					++v21;
				while (v19[v21]);
				memmove(k, v19, v21 + 1);
				v16 = v5;
			}
			else
			{
				++v16;
			}
			v15 = *v16;
		} while (*v16);
		for (l = *v5; l; ++v5)
		{
			if (l == '/' || l == '\\')
				*v5 = separator;
			l = v5[1];
		}
	}
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

	if V_CONSTEXPR(IsX360() && !bIsAbsolute)
	{
		bIsAbsolute = (V_stristr(pStr, ":") != NULL);
	}

	return bIsAbsolute;
}

//-----------------------------------------------------------------------------
// Purpose: Sanity-check to verify that a path is a relative path inside the game dir
// Taken From: engine/cmd.cpp
//-----------------------------------------------------------------------------
bool V_IsValidPath(const char* pStr)
{
	if (!pStr)
	{
		return false;
	}

	if (Q_strlen(pStr) <= 0    ||
		V_IsAbsolutePath(pStr) || // to protect absolute paths
		Q_strstr(pStr, ".."))     // to protect relative paths
	{
		return false;
	}

	return true;
}

#if defined(_MSC_VER) && _MSC_VER >= 1900
bool
#else
void
#endif
V_MakeAbsolutePath(char* pOut, size_t outLen, const char* pPath, const char* pStartingDir)
{
	if (V_IsAbsolutePath(pPath))
	{
		// pPath is not relative.. just copy it.
		V_strncpy(pOut, pPath, outLen);
		pOut[outLen - 1] = '\0';
	}
	else
	{
		// Make sure the starting directory is absolute..
		if (pStartingDir && V_IsAbsolutePath(pStartingDir))
		{
			V_strncpy(pOut, pStartingDir, outLen);
			pOut[outLen - 1] = '\0';
		}
		else
		{
#ifdef _PS3 
			{
				V_strncpy(pOut, g_pPS3PathInfo->GameImagePath(), outLen);
			}
#else
			{
#pragma warning(push) // Disabled type conversion warning, as some implementations of '_getcwd' take a size_t.
#pragma warning(disable : 4267)
				if (!_getcwd(pOut, outLen))
					Error(eDLL_T::COMMON, EXIT_FAILURE, "V_MakeAbsolutePath: _getcwd failed.");
#pragma warning(pop)
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

	if (!V_NormalizePath(pOut, CORRECT_PATH_SEPARATOR))
		Error(eDLL_T::COMMON, EXIT_FAILURE, "V_MakeAbsolutePath: tried to \"..\" past the root.");

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
//			maxLen - 
//			*newLen - 
// Output : Returns the new length of the string
//-----------------------------------------------------------------------------
size_t V_StripLastDir(char* dirName, size_t maxLen)
{
	Assert(dirName);

	if (dirName[0] == '\0')
		return 0;

	size_t len = V_strlen(dirName);
	Assert(len < maxLen);

	if (!V_stricmp(dirName, "./") ||
		!V_stricmp(dirName, ".\\"))
		return len;

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
			dirName[len] = '\0';
			return len;
		}
		else if (dirName[len - 1] == ':')
		{
			bHitColon = true;
		}

		len--;
	}

	// If we hit a drive letter, then we're done.
	// Ex: If they passed in c:\, then V_StripLastDir should
	// turn the string into "" and return 0.
	if (bHitColon)
	{
		dirName[0] = '\0';
		return 0;
	}

	// Allow it to return an empty string and 0. This can happen if something like "tf2/" is passed in.
	// The correct behavior is to strip off the last directory ("tf2") and return the new length.
	if (len == 0)
	{
		int ret = V_snprintf(dirName, maxLen, ".%c", CORRECT_PATH_SEPARATOR);

		// snprintf failed, turn the string into "" and return 0.
		if (ret < 0)
		{
			Assert(0);

			dirName[0] = '\0';
			return 0;
		}

		return ret;
	}

	return len;
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
void V_ComposeFileName(const char* path, const char* filename, char* dest, size_t destSize)
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
void V_StripExtension(const char* in, char* out, size_t outSize)
{
	// Find the last dot. If it's followed by a dot or a slash, then it's part of a 
	// directory specifier like ../../somedir/./blah.

	if (!in || !in[0] || !outSize)
		return;

	// scan backward for '.'
	size_t end = V_strlen(in) - 1;
	while (end > 0 && in[end] != '.' && !PATHSEPARATOR(in[end]))
	{
		--end;
	}

	if (end > 0 && !PATHSEPARATOR(in[end]) && end < outSize)
	{
		size_t nChars = MIN(end, outSize - 1);
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
void V_ExtractFileExtension(const char* path, char* dest, size_t destSize)
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
	size_t len = V_strlen(path);
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
void V_FileBase(const char* in, char* out, size_t maxlen)
{
	Assert(maxlen >= 1);
	Assert(in);
	Assert(out);

	if (!in || !in[0])
	{
		*out = 0;
		return;
	}

	size_t len, start, end;

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

	size_t maxcopy = MIN(len + 1, maxlen);

	// Copy partial string
	V_strncpy(out, &in[start], maxcopy);
}
