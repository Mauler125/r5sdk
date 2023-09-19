//===========================================================================//
//
// Purpose: Switch between experimental/finished 'standard' headers.
//
// $NoKeywords: $
//===========================================================================//
#ifndef EXPERIMENTAL_H
#define EXPERIMENTAL_H

///////////////////////////////////////////////////////////////////////////////
// FILESYSTEM
///////////////////////////////////////////////////////////////////////////////
#ifndef INCLUDE_STD_FILESYSTEM_EXPERIMENTAL
#   if defined(__cpp_lib_filesystem) // Check for feature test macro for <filesystem>.
#       define INCLUDE_STD_FILESYSTEM_EXPERIMENTAL 0

// Check for feature test macro for <experimental/filesystem>.
#   elif defined(__cpp_lib_experimental_filesystem)
#       define INCLUDE_STD_FILESYSTEM_EXPERIMENTAL 1

#   elif !defined(__has_include) // We can't check if headers exist (assuming experimental to be safe).
#       define INCLUDE_STD_FILESYSTEM_EXPERIMENTAL 1

#   elif __has_include(<filesystem>) // Check if the header "<filesystem>" exists.

// If we're compiling on Visual Studio and are not compiling with C++17, we need to use experimental.
#       ifdef _MSC_VER

// Check and include header that defines "_HAS_CXX17".
#           if __has_include(<yvals_core.h>)
#               include <yvals_core.h>

#               if defined(_HAS_CXX17) && _HAS_CXX17
#                   define INCLUDE_STD_FILESYSTEM_EXPERIMENTAL 0
#               endif
#           endif

// If the marco isn't defined yet, that means any of the other VS specific checks failed, so we need to use experimental.
#           ifndef INCLUDE_STD_FILESYSTEM_EXPERIMENTAL
#               define INCLUDE_STD_FILESYSTEM_EXPERIMENTAL 1
#           endif

#       else // #ifdef _MSC_VER
#           define INCLUDE_STD_FILESYSTEM_EXPERIMENTAL 0
#       endif

#   elif __has_include(<experimental/filesystem>)
#       define INCLUDE_STD_FILESYSTEM_EXPERIMENTAL 1

#   else
#       error Could not find system header "<filesystem>" or "<experimental/filesystem>"
#   endif

#   if INCLUDE_STD_FILESYSTEM_EXPERIMENTAL
#       include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#   else // We have a recent compiler and can use the finished version.
// Include it
#       include <filesystem>
namespace fs = std::filesystem;
#   endif
#endif// !INCLUDE_STD_FILESYSTEM_EXPERIMENTAL

#endif // EXPERIMENTAL_H