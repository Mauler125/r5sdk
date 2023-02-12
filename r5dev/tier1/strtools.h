#pragma once

#ifdef _WIN32
#define CORRECT_PATH_SEPARATOR '\\'
#define INCORRECT_PATH_SEPARATOR '/'
#elif POSIX
#define CORRECT_PATH_SEPARATOR '/'
#define INCORRECT_PATH_SEPARATOR '\\'
#endif

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


char const* V_stristr(char const* pStr, char const* pSearch);
const char* V_strnistr(const char* pStr, const char* pSearch, int64_t n);
const char* V_strnchr(const char* pStr, char c, int64_t n);
bool V_isspace(int c);

int V_UTF8ToUnicode(const char* pUTF8, wchar_t* pwchDest, int cubDestSizeInBytes);
int V_UnicodeToUTF8(const wchar_t* pUnicode, char* pUTF8, int cubDestSizeInBytes);
void V_FixSlashes(char* pname, char separator = CORRECT_PATH_SEPARATOR);