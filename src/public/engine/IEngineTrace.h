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
#include "..\ihandleentity.h"
#include "ICollideable.h"

//-----------------------------------------------------------------------------
// The standard trace filter... NOTE: Most normal traces inherit from CTraceFilter!!!
//-----------------------------------------------------------------------------
enum TraceType_t
{
	TRACE_EVERYTHING = 0,
	TRACE_WORLD_ONLY,				// NOTE: This does *not* test static props!!!
	TRACE_ENTITIES_ONLY,			// NOTE: This version will *not* test static props
	TRACE_EVERYTHING_FILTER_PROPS,	// NOTE: This version will pass the IHandleEntity for props through the filter, unlike all other filters
};

//-----------------------------------------------------------------------------
// Classes are expected to inherit these + implement the ShouldHitEntity method
//-----------------------------------------------------------------------------
abstract_class ITraceFilter
{
public:
	virtual ~ITraceFilter() {};
	virtual bool ShouldHitEntity(IHandleEntity* pEntity, int contentsMask) = 0;
	virtual TraceType_t	GetTraceType() const = 0;
	virtual bool Unknown() const = 0;
};

class CTraceFilter : public ITraceFilter
{
public:
	//virtual TraceType_t	GetTraceType() const
	//{
	//	return TRACE_EVERYTHING;
	//}
};

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
abstract_class IEngineTrace
{
public:
	virtual void stub_0() const = 0;
	virtual void stub_1() const = 0;
	virtual void ClipRayToCollideable(const Ray_t& ray, unsigned int fMask, ICollideable* pEntity, trace_t* pTrace) = 0;
	virtual void TraceRayFiltered(const Ray_t& ray, unsigned int fMask, ITraceFilter* pTracefilter, trace_t* pTrace) = 0;
	virtual void TraceRay(const Ray_t& ray, unsigned int fMask, trace_t* pTrace) = 0;
};


#endif // ENGINE_IENGINETRACE_H
