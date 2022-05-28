#include "core/stdafx.h"


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
	{
		pUTF8[0] = 0;
	}

#ifdef _WIN32
	int cchResult = WideCharToMultiByte(CP_UTF8, 0, pUnicode, -1, pUTF8, cubDestSizeInBytes, NULL, NULL);
#elif POSIX
	int cchResult = 0;
	if (pUnicode && pUTF8)
		cchResult = wcstombs(pUTF8, pUnicode, cubDestSizeInBytes);
#endif

	if (cubDestSizeInBytes > 0)
	{
		pUTF8[cubDestSizeInBytes - 1] = 0;
	}

	return cchResult;
}