//
// Copyright (c) 2009-2010 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#ifndef RECASTDETOURDEFS_H
#define RECASTDETOURDEFS_H

/// Signed to avoid warnings when comparing to int loop indexes, and common error with comparing to zero.
/// MSVC2010 has a bug where ssize_t is unsigned (!!!).
typedef intptr_t rdSizeType;
#define RD_SIZE_MAX INTPTR_MAX

/// Macros to hint to the compiler about the likeliest branch. Please add a benchmark that demonstrates a performance
/// improvement before introducing use cases.
#if defined(__GNUC__) || defined(__clang__)
#define rdLikely(x) __builtin_expect((x), true)
#define rdUnlikely(x) __builtin_expect((x), false)
#else
#define rdLikely(x) (x)
#define rdUnlikely(x) (x)
#endif

/// Macros to forcefully inline a function.
#if defined(__GNUC__) || defined(__clang__)
#define rdForceInline static inline
#else
#define rdForceInline __forceinline
#endif

#endif // RECASTDETOURDEFS_H
