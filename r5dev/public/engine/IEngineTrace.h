//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef ENGINE_IENGINETRACE_H
#define ENGINE_IENGINETRACE_H
#ifdef _WIN32
#pragma once
#endif
#include "..\gametrace.h"

abstract_class IEngineTrace
{
public:
	virtual void stub_0() const = 0;
	virtual void stub_1() const = 0;
	virtual void ClipRayToCollideable(__m128* a2, unsigned int a3, __int64* a4, void* a5) = 0;
	virtual void TraceRay(const Ray_t& ray, unsigned int fMask, void* tracefilter, trace_t pTrace) = 0;
	virtual void TraceRay(const Ray_t& ray, unsigned int fMask, trace_t pTrace) = 0;
};

#endif // ENGINE_IENGINETRACE_H
