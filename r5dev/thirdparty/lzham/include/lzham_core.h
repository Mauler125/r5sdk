// File: lzham_core.h
// See Copyright Notice and license at the end of include/lzham.h
#pragma once

#if defined(_MSC_VER)
   #pragma warning (disable: 4127) // conditional expression is constant
#endif

#if defined(_XBOX) && !defined(LZHAM_ANSI_CPLUSPLUS)
   // X360
   #include <xtl.h>
   #define _HAS_EXCEPTIONS 0
   #define NOMINMAX

   #define LZHAM_PLATFORM_X360 1
   #define LZHAM_USE_WIN32_API 1
   #define LZHAM_USE_WIN32_ATOMIC_FUNCTIONS 1
   #define LZHAM_64BIT_POINTERS 0
   #define LZHAM_CPU_HAS_64BIT_REGISTERS 1
   #define LZHAM_BIG_ENDIAN_CPU 1
   #define LZHAM_USE_UNALIGNED_INT_LOADS 1
   #define LZHAM_RESTRICT __restrict
   #define LZHAM_FORCE_INLINE __forceinline
   #define LZHAM_NOTE_UNUSED(x) (void)x

#elif defined(WIN32) && !defined(LZHAM_ANSI_CPLUSPLUS)
   // MSVC or MinGW, x86 or x64, Win32 API's for threading and Win32 Interlocked API's or GCC built-ins for atomic ops.
   #ifdef NDEBUG
      // Ensure checked iterators are disabled.
      #define _SECURE_SCL 0
      #define _HAS_ITERATOR_DEBUGGING 0
   #endif
   #ifndef _DLL
      // If we're using the DLL form of the run-time libs, we're also going to be enabling exceptions because we'll be building CLR apps.
      // Otherwise, we disable exceptions for a small speed boost.
      //#define _HAS_EXCEPTIONS 0
   #endif
   #define NOMINMAX

    #ifndef _WIN32_WINNT
      #define _WIN32_WINNT 0x500
   #endif

   #ifndef WIN32_LEAN_AND_MEAN
      #define WIN32_LEAN_AND_MEAN
   #endif

   #include <windows.h>

   #define LZHAM_USE_WIN32_API 1

   #if defined(__MINGW32__) || defined(__MINGW64__)
      #define LZHAM_USE_GCC_ATOMIC_BUILTINS 1
   #else
      #define LZHAM_USE_WIN32_ATOMIC_FUNCTIONS 1
   #endif

   #define LZHAM_PLATFORM_PC 1

   #ifdef _WIN64
      #define LZHAM_PLATFORM_PC_X64 1
      #define LZHAM_64BIT_POINTERS 1
      #define LZHAM_CPU_HAS_64BIT_REGISTERS 1
      #define LZHAM_LITTLE_ENDIAN_CPU 1
   #else
      #define LZHAM_PLATFORM_PC_X86 1
      #define LZHAM_64BIT_POINTERS 0
      #define LZHAM_CPU_HAS_64BIT_REGISTERS 0
      #define LZHAM_LITTLE_ENDIAN_CPU 1
   #endif

   #define LZHAM_USE_UNALIGNED_INT_LOADS 1
   #define LZHAM_RESTRICT __restrict
   #define LZHAM_FORCE_INLINE __forceinline

   #if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
      #define LZHAM_USE_MSVC_INTRINSICS 1
   #endif

   #define LZHAM_NOTE_UNUSED(x) (void)x

#elif defined(__GNUC__) && !defined(LZHAM_ANSI_CPLUSPLUS)
   // GCC x86 or x64, pthreads for threading and GCC built-ins for atomic ops.
   #define LZHAM_PLATFORM_PC 1

   #if defined(_WIN64) || defined(__MINGW64__) || defined(_LP64) || defined(__LP64__)
      #define LZHAM_PLATFORM_PC_X64 1
      #define LZHAM_64BIT_POINTERS 1
      #define LZHAM_CPU_HAS_64BIT_REGISTERS 1
   #else
      #define LZHAM_PLATFORM_PC_X86 1
      #define LZHAM_64BIT_POINTERS 0
      #define LZHAM_CPU_HAS_64BIT_REGISTERS 0
   #endif

   #define LZHAM_USE_UNALIGNED_INT_LOADS 1

   #define LZHAM_LITTLE_ENDIAN_CPU 1

   #define LZHAM_USE_PTHREADS_API 1
   #define LZHAM_USE_GCC_ATOMIC_BUILTINS 1

   #define LZHAM_RESTRICT

   #if defined(__clang__)
      #define LZHAM_FORCE_INLINE inline
   #else
      #define LZHAM_FORCE_INLINE inline __attribute__((__always_inline__,__gnu_inline__))
   #endif

   #define LZHAM_NOTE_UNUSED(x) (void)x
#else
   // Vanilla ANSI-C/C++
   // No threading support, unaligned loads are NOT okay.
   #if defined(_WIN64) || defined(__MINGW64__) || defined(_LP64) || defined(__LP64__)
      #define LZHAM_64BIT_POINTERS 1
      #define LZHAM_CPU_HAS_64BIT_REGISTERS 1
   #else
      #define LZHAM_64BIT_POINTERS 0
      #define LZHAM_CPU_HAS_64BIT_REGISTERS 0
   #endif

   #define LZHAM_USE_UNALIGNED_INT_LOADS 0

   #if __BIG_ENDIAN__
      #define LZHAM_BIG_ENDIAN_CPU 1
   #else
      #define LZHAM_LITTLE_ENDIAN_CPU 1
   #endif

   #define LZHAM_USE_GCC_ATOMIC_BUILTINS 0
   #define LZHAM_USE_WIN32_ATOMIC_FUNCTIONS 0

   #define LZHAM_RESTRICT
   #define LZHAM_FORCE_INLINE inline

   #define LZHAM_NOTE_UNUSED(x) (void)x

#include <windows.h>
#endif

#if LZHAM_LITTLE_ENDIAN_CPU
   const bool c_lzham_little_endian_platform = true;
#else
   const bool c_lzham_little_endian_platform = false;
#endif

const bool c_lzham_big_endian_platform = !c_lzham_little_endian_platform;

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <malloc.h>
#include <stdarg.h>
#include <memory.h>
#include <limits.h>
#include <algorithm>
#include <errno.h>

#include "lzham.h"
#include "lzham_config.h"
#include "lzham_types.h"
#include "lzham_assert.h"
#include "lzham_platform.h"

#include "lzham_helpers.h"
#include "lzham_traits.h"
#include "lzham_mem.h"
#include "lzham_math.h"
#include "lzham_utils.h"
#include "lzham_vector.h"
