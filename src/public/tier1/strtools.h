#pragma once

#ifdef _WIN32
#define CORRECT_PATH_SEPARATOR '\\'
#define CORRECT_PATH_SEPARATOR_S "\\"
#define INCORRECT_PATH_SEPARATOR '/'
#define INCORRECT_PATH_SEPARATOR_S "/"
#define CHARACTERS_WHICH_SEPARATE_DIRECTORY_COMPONENTS_IN_PATHNAMES ":/\\"
#define PATHSEPARATOR(c) ((c) == '\\' || (c) == '/')
#define PATHSEPARATORW(c) ((c) == L'\\' || (c) == L'/')
#elif POSIX || defined( _PS3 )
#define CORRECT_PATH_SEPARATOR '/'
#define CORRECT_PATH_SEPARATOR_S "/"
#define INCORRECT_PATH_SEPARATOR '\\'
#define INCORRECT_PATH_SEPARATOR_S "\\"
#define CHARACTERS_WHICH_SEPARATE_DIRECTORY_COMPONENTS_IN_PATHNAMES "/"
#define PATHSEPARATOR(c) ((c) == '/')
#define PATHSEPARATORW(c) ((c) == L'/')
#endif

#define COPY_ALL_CHARACTERS -1

/// Faster conversion of an ascii char to upper case. This function does not obey locale or any language
/// setting. It should not be used to convert characters for printing, but it is a better choice
/// for internal strings such as used for hash table keys, etc. It's meant to be inlined and used
/// in places like the various dictionary classes. Not obeying locale also protects you from things
/// like your hash values being different depending on the locale setting.
#define FastASCIIToUpper( c ) ( ( ( (c) >= 'a' ) && ( (c) <= 'z' ) ) ? ( (c) - 32 ) : (c) )
/// similar to FastASCIIToLower
#define FastASCIIToLower( c ) ( ( ( (c) >= 'A' ) && ( (c) <= 'Z' ) ) ? ( (c) + 32 ) : (c) )

#define V_vsnprintf vsnprintf
#define V_snprintf snprintf
#define V_strlower _strlwr
#define V_strlen strlen
#define V_strncat strncat
#define V_stricmp _stricmp
#define V_strnicmp _strnicmp
#define V_strcmp strcmp
#define V_strncmp strncmp
#define V_strstr strstr
#define V_strncpy strncpy
#define V_strdup _strdup
#define V_strcat strcat

#define V_strcasecmp V_stricmp

#define Q_vsnprintf V_vsnprintf
#define Q_snprintf V_snprintf
#define Q_strlower V_strlower
#define Q_strlen V_strlen
#define Q_strncat V_strncat
#define Q_strnistr V_strnistr
#define Q_stricmp V_stricmp
#define Q_strnicmp V_strnicmp
#define Q_strncasecmp V_strnicmp
#define Q_strcasecmp V_stricmp
#define Q_strcmp V_strcmp
#define Q_strncmp V_strncmp
#define Q_strstr V_strstr
#define Q_strncpy V_strncpy
#define Q_strdup V_strdup
#define Q_strcat V_strcat

template <size_t maxLenInCharacters> int V_vsprintf_safe(OUT_Z_ARRAY char(&pDest)[maxLenInCharacters], PRINTF_FORMAT_STRING const char* pFormat, va_list params) { return V_vsnprintf(pDest, maxLenInCharacters, pFormat, params); }
int	_V_stricmp_NegativeForUnequal(const char* s1, const char* s2);

char const* V_stristr(char const* pStr, char const* pSearch);
const char* V_strnistr(const char* pStr, const char* pSearch, ssize_t n);
const char* V_strnchr(const char* pStr, char c, ssize_t n);
bool V_isspace(int c);

inline bool V_isalnum(char c) { return isalnum((unsigned char)c) != 0; }
bool V_IsAllDigit(const char* pString);

// this is locale-unaware and therefore faster version of standard isdigit()
// It also avoids sign-extension errors.
inline bool V_isdigit(char c)
{
	return c >= '0' && c <= '9';
}

inline bool V_iswdigit(int c)
{
	return (((uint)(c - '0')) < 10);
}

void V_hextobinary(char const* in, size_t numchars, byte* out, size_t maxoutputbytes);
void V_binarytohex(const byte* in, size_t inputbytes, char* out, size_t outsize);
ssize_t V_vsnprintfRet(char* pDest, size_t maxLen, const char* pFormat, va_list params, bool* pbTruncated);

// Strip white space at the beginning and end of a string
ssize_t V_StrTrim(char* pStr);

class CUtlStringList;
void V_SplitString2(const char* pString, const char** pSeparators, ssize_t nSeparators, CUtlStringList& outStrings);
void V_SplitString(const char* pString, const char* pSeparator, CUtlStringList& outStrings);

int V_UTF8ToUnicode(const char* pUTF8, wchar_t* pwchDest, int cubDestSizeInBytes);
int V_UnicodeToUTF8(const wchar_t* pUnicode, char* pUTF8, int cubDestSizeInBytes);

int V_UTF8CharLength(const unsigned char input);
bool V_IsValidUTF8(const char* pszString);

typedef enum
{
	PATTERN_NONE = 0x00000000,
	PATTERN_DIRECTORY = 0x00000001
} TStringPattern;

// String matching using wildcards (*) for partial matches.
bool V_StringMatchesPattern(const char* szString, const char* szPattern, int flags = 0);

bool V_ComparePath(const char* a, const char* b);

void V_FixSlashes(char* pname, char separator = CORRECT_PATH_SEPARATOR);

// Adds a path separator to the end of the string if there isn't one already and the string is not empty.
// Triggers a fatal error if it would run out of space.
void V_AppendSlash(INOUT_Z_CAP(strSize) char* pStr, size_t strSize, char separator = CORRECT_PATH_SEPARATOR);

// Remove the final characters of ppath if it's '\' or '/'.
void V_StripTrailingSlash(char* ppath);

// This removes "./" and "../" from the pathname. pFilename should be a full pathname.
// Returns false if it tries to ".." past the root directory in the drive (in which case 
// it is an invalid path).
bool V_RemoveDotSlashes(char* pFilename, char separator = CORRECT_PATH_SEPARATOR);

// Returns true if the path could be normalized.
bool V_NormalizePath(char* pfilePath, char separator);

// Returns true if the path is an absolute path.
bool V_IsAbsolutePath(IN_Z const char* pPath);

// Returns true if the path is valid.
bool V_IsValidPath(const char* pStr);

// If pPath is a relative path, this function makes it into an absolute path
// using the current working directory as the base, or pStartingDir if it's non-NULL.
// Returns false if it runs out of room in the string, or if pPath tries to ".." past the root directory.
#if defined(_MSC_VER) && _MSC_VER >= 1900
bool
#else
void
#endif
V_MakeAbsolutePath(char* pOut, size_t outLen, const char* pPath, const char* pStartingDir = NULL);
inline void V_MakeAbsolutePath(char* pOut, size_t outLen, const char* pPath, const char* pStartingDir, bool bLowercaseName)
{
	V_MakeAbsolutePath(pOut, outLen, pPath, pStartingDir);
	if (bLowercaseName)
	{
		V_strlower(pOut);
	}
}

// Remove the final directory from the path
size_t V_StripLastDir(char* dirName, size_t maxLen);
// Returns a pointer to the unqualified file name (no path) of a file name
const char* V_UnqualifiedFileName(const char* in);
const wchar_t* V_UnqualifiedFileName(const wchar_t* in);

// Given a path and a filename, composes "path\filename", inserting the (OS correct) separator if necessary
void V_ComposeFileName(const char* path, const char* filename, char* dest, size_t destSize);

// Remove any extension from in and return resulting string in out
void V_StripExtension(const char* in, char* out, size_t outLen);

// Returns a pointer to the file extension or NULL if one doesn't exist
const char* V_GetFileExtension(const char* path, const bool keepDot = false);

// Copy out the file extension into dest
void V_ExtractFileExtension(const char* path, char* dest, size_t destSize);
bool V_ExtractFilePath(const char* path, char* dest, size_t destSize);

// Extracts the base name of a file (no path, no extension, assumes '/' or '\' as path separator)
void V_FileBase(const char* in, OUT_Z_CAP(maxlen) char* out, size_t maxlen);
