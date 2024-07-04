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

#include "Shared/Include/SharedAlloc.h"

static void* rdAllocDefault(size_t size, rdAllocHint)
{
	return malloc(size);
}

static void rdFreeDefault(void *ptr)
{
	free(ptr);
}

static rdAllocFunc* sRecastAllocFunc = rdAllocDefault;
static rdFreeFunc* sRecastFreeFunc = rdFreeDefault;

void rdAllocSetCustom(rdAllocFunc* allocFunc, rdFreeFunc* freeFunc)
{
	sRecastAllocFunc = allocFunc ? allocFunc : rdAllocDefault;
	sRecastFreeFunc = freeFunc ? freeFunc : rdFreeDefault;
}

void* rdAlloc(size_t size, rdAllocHint hint)
{
	return sRecastAllocFunc(size, hint);
}

void rdFree(void* ptr)
{
	if (ptr != NULL)
	{
		sRecastFreeFunc(ptr);
	}
}
