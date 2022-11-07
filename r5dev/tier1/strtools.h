#pragma once

#ifdef _WIN32
#define CORRECT_PATH_SEPARATOR '\\'
#define INCORRECT_PATH_SEPARATOR '/'
#elif POSIX
#define CORRECT_PATH_SEPARATOR '/'
#define INCORRECT_PATH_SEPARATOR '\\'
#endif

#define V_snprintf snprintf
#define V_strlower _strlwr
#define V_strlen strlen
#define V_strncat strncat
#define V_stricmp _stricmp
#define V_strnicmp _strnicmp
#define V_strcmp strcmp

#define Q_snprintf V_snprintf
#define Q_strlower V_strlower
#define Q_strlen V_strlen
#define Q_strncat V_strncat
#define Q_stricmp V_stricmp
#define Q_strnicmp V_strnicmp
#define Q_strncasecmp V_strnicmp
#define Q_strcasecmp V_stricmp
#define Q_strcmp V_strcmp

char const* V_stristr(char const* pStr, char const* pSearch);

int V_UTF8ToUnicode(const char* pUTF8, wchar_t* pwchDest, int cubDestSizeInBytes);
int V_UnicodeToUTF8(const wchar_t* pUnicode, char* pUTF8, int cubDestSizeInBytes);
void V_FixSlashes(char* pname, char separator = CORRECT_PATH_SEPARATOR);