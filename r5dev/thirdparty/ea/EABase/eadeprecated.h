/*-----------------------------------------------------------------------------
 * eadeprecatedbase.h
 *
 * Copyright (c) Electronic Arts Inc. All rights reserved.
 *---------------------------------------------------------------------------*/

#pragma once
#ifndef INCLUDED_eadeprecated_H
#define INCLUDED_eadeprecated_H

#include <EABase/config/eacompilertraits.h>


// ------------------------------------------------------------------------
// Documentation on deprecated attribute: https://en.cppreference.com/w/cpp/language/attributes/deprecated
// Documentation on SimVer version numbers: http://simver.org/
// 
// These macros provide a structured formatting to C++ deprecated annotation messages. This ensures
//  that the required information is presented in a standard format for developers and tools.
// 
// Example usage:
//  
// Current package version : current_ver
// Future version for code removal : major_ver, minor_ver, change_ver
// Deprecation comment : ""
//
// 	EA_DEPRECATED(current_ver, major_ver, minor_ver, change_ver, tag, "Do not use deprecated function")
// 	void TestFunc() {}
// 
// 	EA_DEPRECATED(current_ver, major_ver, minor_ver, change_ver, tag, "Do not use deprecated typedef")
// 	typedef int TestTypedef;
// 
// 	EA_DEPRECATED(current_ver, major_ver, minor_ver, change_ver, tag, "Do not use deprecated variable")
// 	int TestVariable;
// 
// 	EA_DEPRECATED_STRUCT(current_ver, major_ver, minor_ver, change_ver, tag, "Do not use deprecated struct")
// 	TestStruct {};
// 
// 	EA_DEPRECATED_CLASS(current_ver, major_ver, minor_ver, change_ver, tag, "Do not use deprecated class")
// 	TestClass {};
// 
// 	union TestUnion
// 	{
// 		EA_DEPRECATED(current_ver, major_ver, minor_ver, change_ver, tag, "Do not use deprecated data member") int n;
// 	};
// 
// 	EA_DEPRECATED_ENUM(current_ver, major_ver, minor_ver, change_ver, tag, "Do not use deprecated enumeration")
// 	TestEnumeration { TestEnumeration_Value1, TestEnumeration_Value2 };
// 
// 	enum TestEnumerator 
// 	{ 
// 		TestEnumerator_Value1 EA_DEPRECATED_ENUMVALUE(current_ver, major_ver, minor_ver, change_ver, tag, "Do not use deprecated enum value") = 5,
// 		TestEnumerator_Value2 = 4
// 	};
// 
// 	EA_DISABLE_DEPRECATED(current_ver, major_ver, minor_ver, change_ver, tag, "Suppress the deprecated warning until the given Release")
// 	void TestFunc() {}
// 	EA_RESTORE_DEPRECATED()
// 

// ------------------------------------------------------------------------
// Create an integer version number which can be compared with numerical operators
#define EA_CREATE_VERSION(MAJOR, MINOR, PATCH) \
	(((MAJOR) * 1000000) + (((MINOR) + 1) * 10000) + (((PATCH) + 1) * 100))

// ------------------------------------------------------------------------
// INTERNAL MACROS - DO NOT USE DIRECTLY
//
// When EA_DEPRECATED_API_EXPIRED_IS_ERROR this macro produce a static asset on code that is past the expiry date.

#if defined(EA_DEPRECATED_API_EXPIRED_IS_ERROR) && EA_DEPRECATED_API_EXPIRED_IS_ERROR
	#define EA_DEPRECATED_BEFORETYPE(_moduleVersion, _major_version, _minor_version, _patch_version, _annotation)	\
		static_assert(_moduleVersion < EA_CREATE_VERSION(_major_version,_minor_version,_patch_version), "This API has been deprecated and needs to be removed");
#else
	#define EA_DEPRECATED_BEFORETYPE(_moduleVersion, _major_version, _minor_version, _patch_version, _annotation)
#endif


// ------------------------------------------------------------------------
// INTERNAL MACROS - DO NOT USE DIRECTLY
//
// When EA_IGNORE_DEPRECATION is set deprecation annotation will not be produced

#if defined(EA_IGNORE_DEPRECATION) && EA_IGNORE_DEPRECATION
	#define EA_DEPRECATED_AFTERTYPE(_major_version, _minor_version, _patch_version, _annotation, _msg)
#else
	#define EA_DEPRECATED_AFTERTYPE(_major_version, _minor_version, _patch_version, _annotation, _msg)	\
		EA_DEPRECATED_MESSAGE(_msg. This API will be removed in _major_version._minor_version._patch_version _annotation)
#endif

// ------------------------------------------------------------------------
// INTERNAL MACROS - DO NOT USE DIRECTLY
//
// Simple case

#define EA_DEPRECATED_SIMPLE(_moduleVersion, _major_version, _minor_version, _patch_version, _annotation, _msg)				\
	EA_DEPRECATED_BEFORETYPE(_moduleVersion, _major_version, _minor_version, _patch_version, _annotation)					\
	EA_DEPRECATED_AFTERTYPE(_major_version, _minor_version, _patch_version, _annotation, _msg)


// ------------------------------------------------------------------------
// INTERNAL MACROS - DO NOT USE DIRECTLY
//
// Macro which inserts the keyword to correctly format the deprecation annotation
#define EA_DEPRECATED_TYPE(_moduleVersion, _major_version, _minor_version, _patch_version, _annotation, _msg, _keyword)		\
	EA_DEPRECATED_BEFORETYPE(_moduleVersion, _major_version, _minor_version, _patch_version, _annotation)					\
	_keyword																												\
	EA_DEPRECATED_AFTERTYPE(_major_version, _minor_version, _patch_version, _annotation, _msg)



// ------------------------------------------------------------------------
// PUBLIC MACROS
// See file header comment for example usage.

// ------------------------------------------------------------------------
// 	EA_DEPRECATED(current_ver, major_ver, minor_ver, change_ver, tag, "Do not use deprecated function")
// 	void TestFunc() {}
// 
// 	EA_DEPRECATED(current_ver, major_ver, minor_ver, change_ver, tag, "Do not use deprecated typedef")
// 	typedef int TestTypedef;
// 
// 	EA_DEPRECATED(current_ver, major_ver, minor_ver, change_ver, tag, "Do not use deprecated variable")
// 	int TestVariable;

#define EA_DEPRECATED_API(_moduleVersion, _major_version, _minor_version, _patch_version, _annotation, _msg)						\
			EA_STRINGIFY(EA_DEPRECATED_SIMPLE(_moduleVersion, _major_version, _minor_version, _patch_version, _annotation, _msg))


// ------------------------------------------------------------------------
// 	EA_DEPRECATED_STRUCT(current_ver, major_ver, minor_ver, change_ver, tag, "Do not use deprecated struct")
// 	TestStruct {};

#define EA_DEPRECATED_STRUCT(_moduleVersion, _major_version, _minor_version, _patch_version, _annotation, _msg)					\
			EA_STRINGIFY(EA_DEPRECATED_TYPE(_moduleVersion, _major_version, _minor_version, _patch_version, _annotation, _msg, struct))


// ------------------------------------------------------------------------
// 	EA_DEPRECATED_CLASS(current_ver, major_ver, minor_ver, change_ver, tag, "Do not use deprecated class")
// 	TestClass {};

#define EA_DEPRECATED_CLASS(_moduleVersion, _major_version, _minor_version, _patch_version, _annotation, _msg)		\
			EA_STRINGIFY(EA_DEPRECATED_TYPE(_moduleVersion, _major_version, _minor_version, _patch_version, _annotation, _msg, class))


// ------------------------------------------------------------------------
// 	EA_DEPRECATED_ENUM(current_ver, major_ver, minor_ver, change_ver, tag, "Do not use deprecated enumeration")
// 	TestEnumeration { TestEnumeration_Value1, TestEnumeration_Value2 };

#define EA_DEPRECATED_ENUM(_moduleVersion, _major_version, _minor_version, _patch_version, _annotation, _msg)		\
			EA_STRINGIFY(EA_DEPRECATED_TYPE(_moduleVersion, _major_version, _minor_version, _patch_version, _annotation, _msg, enum))


// ------------------------------------------------------------------------
// 	enum TestEnumerator 
// 	{ 
// 		TestEnumerator_Value1 EA_DEPRECATED_ENUMVALUE(current_ver, major_ver, minor_ver, change_ver, tag, "Do not use deprecated enum value") = 5,
// 		TestEnumerator_Value2 = 4
// 	};
#define EA_DEPRECATED_ENUMVALUE(_value, _moduleVersion, _major_version, _minor_version, _patch_version, _annotation, _msg)		\
			_value EA_STRINGIFY(EA_DEPRECATED_AFTERTYPE(_moduleVersion, _major_version, _minor_version, _patch_version, _annotation))


// ------------------------------------------------------------------------
// Suppress deprecated warnings around a block of code, see file comment for full usage.
//  EA_DISABLE_DEPRECATED(current_ver, major_ver, minor_ver, change_ver, tag, "Suppress the deprecated warning until the given Release")

#define EA_DISABLE_DEPRECATED(_moduleVersion, _major_version, _minor_version, _patch_version, _annotation, _msg)				\
	EA_STRINGIFY(EA_DEPRECATED_BEFORETYPE(_moduleVersion, _major_version, _minor_version, _patch_version, _annotation))	\
	EA_DISABLE_VC_WARNING(4996);																								\
	EA_DISABLE_CLANG_WARNING(-Wdeprecated-declarations);

// ------------------------------------------------------------------------
// Restore the compiler warnings
//  EA_RESTORE_DEPRECATED()

#define EA_RESTORE_DEPRECATED()													\
	EA_RESTORE_CLANG_WARNING();													\
	EA_RESTORE_VC_WARNING();

#endif // Header include guard