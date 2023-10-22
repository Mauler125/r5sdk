#ifndef LOCALIZE_H
#define LOCALIZE_H

// direct references to localized strings
typedef unsigned long StringIndex_t;
const unsigned long INVALID_LOCALIZE_STRING_INDEX = (StringIndex_t)-1;

abstract_class ILocalize : public IAppSystem
{
public:
	virtual bool LoadLocalizationFileLists() = 0;

	// adds the contents of a file to the localization table
	virtual bool AddFile(const char* fileName, const char* pPathID = NULL) = 0;

	// Remove all strings from the table
	virtual void RemoveAll() = 0;

	// Finds the localized text for tokenName
	virtual wchar_t* Find(char const* tokenName) = 0;

	virtual void* FindIndex_Unknown(StringIndex_t index) = 0;

	// converts an english string to unicode
	// returns the number of wchar_t in resulting string, including null terminator
	virtual int ConvertANSIToUnicode(const char* ansi, OUT_Z_BYTECAP(unicodeBufferSizeInBytes) wchar_t* unicode, ssize_t unicodeBufferSizeInBytes) = 0;

	// converts an unicode string to an english string
	// unrepresentable characters are converted to system default
	// returns the number of characters in resulting string, including null terminator
	virtual int ConvertUnicodeToANSI(const wchar_t* unicode, OUT_Z_BYTECAP(ansiBufferSize) char* ansi, ssize_t ansiBufferSize) = 0;


	virtual StringIndex_t FindIndex(StringIndex_t index) = 0;

	//!!! TODO !!!
  //void* func_80[3];


  //virtual void ConstructString(CLocalize*, char*, __int64, __int64, ...);

  //__int64(__fastcall* GetNameByIndex)(CLocalize*, int);
  //__int64(__fastcall* GetValueByIndex)(CLocalize*, int);
  //__int64(__fastcall* GetFirstStringIndex)(CLocalize*);
  //__int64(__fastcall* GetNextStringIndex)(CLocalize*, int);


  //void* func_C0[6];


  //__int64(__fastcall* ReloadLocalizationFiles)(CLocalize*);


  //void* func_F8[6];


  //__int64(__fastcall* ConvertANSIToUCS2)(CLocalize*, const char*, void*, __int64);
  //void(__fastcall* ConvertUCS2ToANSI)(CLocalize*, __int16*, char*, int);
};

inline const char* const g_LanguageNames[] = {
	"english",
	"german",
	"french",
	"italian",
	"korean",
	"spanish",
	"schinese",
	"tchinese",
	"russian",
	"japanese",
	"portuguese",
	"polish",
};

#endif // LOCALIZE_H
