#pragma once

#include "mathlib/mathlib.h"

// EVERYTHING IN HERE STILL NEEDS TESTING!!!!

struct Ray_t
{
	VectorAligned m_Start;
	VectorAligned m_Delta;
	VectorAligned m_StartOffset;
	VectorAligned m_Extents;
	char gap2C[0x10];
	void* m_pWorldAxisTransform;
	bool m_IsRay;
	bool m_IsSwept;

	void Init(Vector3D const& start, Vector3D const& end)
	{
		m_Delta = end - start;

		m_IsSwept = (m_Delta.LengthSqr() != 0);

		m_Extents.Init();

		m_pWorldAxisTransform = NULL;
		m_IsRay = true;

		m_StartOffset.Init();
		m_Start = start;
	}
};

struct csurface_t
{
	const char* name;
	short surfaceProp;
	uint16_t flags;
};

struct cplanetrace_t
{
	Vector3D normal;
	float dist;
};

struct trace_t
{
	Vector3D start;
	float unk1;
	Vector3D endpos;
	float unk2;
	cplanetrace_t plane;
	float fraction;
	int contents;
	bool allsolid;
	bool startsolid;
	char gap3A[0x6];
	csurface_t surface;
	float fractionleftsolid;
	int hitgroup;
	short physicsBone;
	char gap5A[0x6];
	void* hit_entity;
	int hitbox;
	char gap6C[0x114];
}; //Size: 0x0180

class CEngineTrace
{
	virtual void stub_0() const = 0;
	virtual void stub_1() const = 0;
	virtual void ClipRayToCollideable(__m128* a2, unsigned int a3, __int64* a4, void* a5) = 0;
	virtual void TraceRay(const Ray_t& ray, unsigned int fMask, void* tracefilter, trace_t pTrace) = 0;
	virtual void TraceRay(const Ray_t& ray, unsigned int fMask, trace_t pTrace) = 0;
};

/* ==== CENGINETRACE ======================================================================================================================================================= */

inline CEngineTrace* g_pEngineTrace = nullptr;

///////////////////////////////////////////////////////////////////////////////
void CEngineTrace_Attach();
void CEngineTrace_Detach();

///////////////////////////////////////////////////////////////////////////////
class VEngine_Trace : public IDetour
{
	virtual void GetAdr(void) const { }
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VEngine_Trace);