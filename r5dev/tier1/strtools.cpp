#include "tier1/strtools.h"

//-----------------------------------------------------------------------------
// Convert upper case characters to lower
//-----------------------------------------------------------------------------
static int FastToLower(char c)
{
	int i = (unsigned char)c;
	if (i < 0x80)
	{
		// Brutally fast branchless ASCII tolower():
		i += (((('A' - 1) - i) & (i - ('Z' + 1))) >> 26) & 0x20;
	}
	else
	{
		i += isupper(i) ? 0x20 : 0;
	}
	return i;
}

//-----------------------------------------------------------------------------
// Allocate a string buffer
//-----------------------------------------------------------------------------
char* AllocString(const char* pStr, ssize_t nMaxChars)
{
	const ssize_t allocLen = (nMaxChars == -1)
		? strlen(pStr) + 1
		: Min((ssize_t)strlen(pStr), nMaxChars) + 1;

	char* const pOut = new char[allocLen];
	V_strncpy(pOut, pStr, allocLen);

	return pOut;
}

//-----------------------------------------------------------------------------
// A special high-performance case-insensitive compare function
// returns 0 if strings match exactly
// returns >0 if strings match in a case-insensitive way, but do not match exactly
// returns <0 if strings do not match even in a case-insensitive way
//-----------------------------------------------------------------------------
int	_V_stricmp_NegativeForUnequal(const char* s1, const char* s2)
{
	// It is not uncommon to compare a string to itself. Since stricmp
	// is expensive and pointer comparison is cheap, this simple test
	// can save a lot of cycles, and cache pollution.
	if (s1 == s2)
		return 0;

	uint8 const* pS1 = (uint8 const*)s1;
	uint8 const* pS2 = (uint8 const*)s2;
	int iExactMatchResult = 1;
	for (;;)
	{
		int c1 = *(pS1++);
		int c2 = *(pS2++);
		if (c1 == c2)
		{
			// strings are case-insensitive equal, coerce accumulated
			// case-difference to 0/1 and return it
			if (!c1) return !iExactMatchResult;
		}
		else
		{
			if (!c2)
			{
				// c2=0 and != c1  =>  not equal
				return -1;
			}
			iExactMatchResult = 0;
			c1 = FastASCIIToLower(c1);
			c2 = FastASCIIToLower(c2);
			if (c1 != c2)
			{
				// strings are not equal
				return -1;
			}
		}
		c1 = *(pS1++);
		c2 = *(pS2++);
		if (c1 == c2)
		{
			// strings are case-insensitive equal, coerce accumulated
			// case-difference to 0/1 and return it
			if (!c1) return !iExactMatchResult;
		}
		else
		{
			if (!c2)
			{
				// c2=0 and != c1  =>  not equal
				return -1;
			}
			iExactMatchResult = 0;
			c1 = FastASCIIToLower(c1);
			c2 = FastASCIIToLower(c2);
			if (c1 != c2)
			{
				// strings are not equal
				return -1;
			}
		}
	}
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

bool V_IsAllDigit(const char* pString)
{
	while (*pString)
	{
		if (!V_isdigit(*pString))
		{
			return false;
		}

		pString++;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the 4 bit nibble for a hex character
// Input  : c - 
// Output : unsigned char
//-----------------------------------------------------------------------------
static unsigned char V_nibble(char c)
{
	if ((c >= '0') &&
		(c <= '9'))
	{
		return (unsigned char)(c - '0');
	}

	if ((c >= 'A') &&
		(c <= 'F'))
	{
		return (unsigned char)(c - 'A' + 0x0a);
	}

	if ((c >= 'a') &&
		(c <= 'f'))
	{
		return (unsigned char)(c - 'a' + 0x0a);
	}

	return '0';
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *in - 
//			numchars - 
//			*out - 
//			maxoutputbytes - 
//-----------------------------------------------------------------------------
void V_hextobinary(char const* in, size_t numchars, byte* out, size_t maxoutputbytes)
{
	size_t len = V_strlen(in);
	numchars = Min(len, numchars);
	// Make sure it's even
	numchars = (numchars) & ~0x1;

	// Must be an even # of input characters (two chars per output byte)
	Assert(numchars >= 2);

	memset(out, 0x00, maxoutputbytes);

	byte* p;
	size_t i;

	p = out;
	for (i = 0;
		(i < numchars) && 
		((size_t)(p - out) < maxoutputbytes);
		i += 2, p++)
	{
		*p = (V_nibble(in[i]) << 4) | V_nibble(in[i + 1]);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *in - 
//			inputbytes - 
//			*out - 
//			outsize - 
//-----------------------------------------------------------------------------
void V_binarytohex(const byte* in, size_t inputbytes, char* out, size_t outsize)
{
	Assert(outsize >= 1);
	char doublet[10];
	int i;

	out[0] = 0;

	for (i = 0; i < inputbytes; i++)
	{
		unsigned char c = in[i];
		V_snprintf(doublet, sizeof(doublet), "%02x", c);
		V_strncat(out, doublet, outsize);
	}
}


ssize_t V_vsnprintfRet(char* pDest, size_t maxLen, const char* pFormat, va_list params, bool* pbTruncated)
{
	Assert(maxLen > 0);

	ssize_t len = _vsnprintf(pDest, maxLen, pFormat, params);
	const bool bTruncated = (len < 0) || (len >= (ssize_t)maxLen);

	if (pbTruncated)
	{
		*pbTruncated = bTruncated;
	}

	if (bTruncated && maxLen > 0)
	{
		len = maxLen - 1;
		pDest[maxLen - 1] = 0;
	}

	return len;
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

void V_SplitString2(const char* pString, const char** pSeparators, ssize_t nSeparators, CUtlStringList& outStrings)
{
	outStrings.Purge();
	const char* pCurPos = pString;

	while (true)
	{
		ssize_t iFirstSeparator = -1;
		const char* pFirstSeparator = nullptr;

		for (ssize_t i = 0; i < nSeparators; i++)
		{
			const char* const pTest = V_stristr(pCurPos, pSeparators[i]);

			if (pTest && (!pFirstSeparator || pTest < pFirstSeparator))
			{
				iFirstSeparator = i;
				pFirstSeparator = pTest;
			}
		}

		if (pFirstSeparator)
		{
			// Split on this separator and continue on.
			const ssize_t separatorLen = strlen(pSeparators[iFirstSeparator]);

			if (pFirstSeparator > pCurPos)
			{
				outStrings.AddToTail(AllocString(pCurPos, pFirstSeparator - pCurPos));
			}

			pCurPos = pFirstSeparator + separatorLen;
		}
		else
		{
			// Copy the rest of the string
			if (strlen(pCurPos))
			{
				outStrings.AddToTail(AllocString(pCurPos, -1));
			}

			return;
		}
	}
}

void V_SplitString(const char* pString, const char* pSeparator, CUtlStringList& outStrings)
{
	V_SplitString2(pString, &pSeparator, 1, outStrings);
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

	if (cchResult <= 0 || cchResult > cubDestSizeInBytes)
	{
		if (cchResult != cubDestSizeInBytes || pUTF8[cubDestSizeInBytes - 1] != '\0')
		{
			*pUTF8 = '\0';
		}
	}
	else
		pUTF8[cchResult] = '\0';

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
// Makes a relative path
//-----------------------------------------------------------------------------
bool V_MakeRelativePath(const char* pFullPath, const char* pDirectory, char* pRelativePath, const size_t nBufLen)
{
	Assert(nBufLen);
	pRelativePath[0] = 0;

	const char* pPath = pFullPath;
	const char* pDir = pDirectory;

	// Strip out common parts of the path
	const char* pLastCommonPath = NULL;
	const char* pLastCommonDir = NULL;
	while (*pPath && (FastToLower(*pPath) == FastToLower(*pDir) ||
		(PATHSEPARATOR(*pPath) && (PATHSEPARATOR(*pDir) || (*pDir == 0)))))
	{
		if (PATHSEPARATOR(*pPath))
		{
			pLastCommonPath = pPath + 1;
			pLastCommonDir = pDir + 1;
		}
		if (*pDir == 0)
		{
			--pLastCommonDir;
			break;
		}
		++pDir; ++pPath;
	}

	// Nothing in common
	if (!pLastCommonPath)
		return false;

	// For each path separator remaining in the dir, need a ../
	size_t nOutLen = 0;
	bool bLastCharWasSeparator = true;
	for (; *pLastCommonDir; ++pLastCommonDir)
	{
		if (PATHSEPARATOR(*pLastCommonDir))
		{
			pRelativePath[nOutLen++] = '.';
			pRelativePath[nOutLen++] = '.';
			pRelativePath[nOutLen++] = CORRECT_PATH_SEPARATOR;
			bLastCharWasSeparator = true;
		}
		else
		{
			bLastCharWasSeparator = false;
		}
	}

	// Deal with relative paths not specified with a trailing slash
	if (!bLastCharWasSeparator)
	{
		pRelativePath[nOutLen++] = '.';
		pRelativePath[nOutLen++] = '.';
		pRelativePath[nOutLen++] = CORRECT_PATH_SEPARATOR;
	}

	// Copy the remaining part of the relative path over, fixing the path separators
	for (; *pLastCommonPath; ++pLastCommonPath)
	{
		if (PATHSEPARATOR(*pLastCommonPath))
		{
			pRelativePath[nOutLen++] = CORRECT_PATH_SEPARATOR;
		}
		else
		{
			pRelativePath[nOutLen++] = *pLastCommonPath;
		}

		// Check for overflow
		if (nOutLen == nBufLen - 1)
			break;
	}

	pRelativePath[nOutLen] = 0;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Strip off the last directory from dirName
// Input  : *dirName - 
//			maxLen - 
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
	Assert(in);

	const char* out = in;

	while (*in)
	{
		if (PATHSEPARATOR(*in))
		{
			// +1 to skip the slash
			out = in + 1;
		}

		in++;
	}

	return out;
}

const wchar_t* V_UnqualifiedFileName(const wchar_t* in)
{
	Assert(in);

	const wchar_t* out = in;

	while (*in)
	{
		if (PATHSEPARATORW(*in))
		{
			// +1 to skip the slash
			out = in + 1;
		}

		in++;
	}

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
		size_t nChars = Min(end, outSize - 1);
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
// Purpose: Returns a pointer to the file extension within a file name string
// Input:	in - file name 
// Output:	pointer to beginning of extension (after the "."), or the passed
//				in string if there is no extension
//-----------------------------------------------------------------------------
const char* V_GetFileExtension(const char* path, const bool keepDot)
{
	Assert(path);

	const char* out = nullptr;

	while (*path)
	{
		if (*path == '.')
		{
			out = path;

			if (!keepDot)
				out++;
		}
		else if (PATHSEPARATOR(*path))
		{
			// didn't reach the file name yet, reset
			out = nullptr;
		}

		path++;
	}

	return out ? out : path;
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
// Purpose: 
// Input  : *path - 
//			*dest - 
//			destSize - 
// Output : void V_ExtractFilePath
//-----------------------------------------------------------------------------
bool V_ExtractFilePath(const char* path, char* dest, size_t destSize)
{
	Assert(destSize >= 1);
	if (destSize < 1)
	{
		return false;
	}

	// Last char
	const size_t len = V_strlen(path);
	const char* src = path + (len ? len - 1 : 0);

	// back up until a \ or the start
	while (src != path && !PATHSEPARATOR(*(src - 1)))
	{
		src--;
	}

	const ssize_t copysize = Min(size_t(src - path), destSize - 1);
	memcpy(dest, path, copysize);
	dest[copysize] = 0;

	return copysize != 0 ? true : false;
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

	size_t maxcopy = Min(len + 1, maxlen);

	// Copy partial string
	V_strncpy(out, &in[start], maxcopy);
}
