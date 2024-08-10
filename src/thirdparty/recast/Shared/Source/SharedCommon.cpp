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

#include "Shared/Include/SharedMath.h"
#include "Shared/Include/SharedCommon.h"

//////////////////////////////////////////////////////////////////////////////////////////

float rdCalcSlopeAngle(const float* v1, const float* v2)
{
	const float deltaX = v2[0] - v1[0];
	const float deltaY = v2[1] - v1[1];
	const float deltaZ = v2[2] - v1[2];

	const float horizontalDistance = rdMathSqrtf((deltaX*deltaX)+(deltaY*deltaY));
	const float slopeAngleRadians = rdMathAtan2f(deltaZ, horizontalDistance);
	const float slopeAngleDegrees = slopeAngleRadians*(180.0f/RD_PI);

	return slopeAngleDegrees;
}

void rdClosestPtPointTriangle(float* closest, const float* p,
							  const float* a, const float* b, const float* c)
{
	// Check if P in vertex region outside A
	float ab[3], ac[3], ap[3];
	rdVsub(ab, b, a);
	rdVsub(ac, c, a);
	rdVsub(ap, p, a);
	float d1 = rdVdot(ab, ap);
	float d2 = rdVdot(ac, ap);
	if (d1 <= 0.0f && d2 <= 0.0f)
	{
		// barycentric coordinates (1,0,0)
		rdVcopy(closest, a);
		return;
	}
	
	// Check if P in vertex region outside B
	float bp[3];
	rdVsub(bp, p, b);
	float d3 = rdVdot(ab, bp);
	float d4 = rdVdot(ac, bp);
	if (d3 >= 0.0f && d4 <= d3)
	{
		// barycentric coordinates (0,1,0)
		rdVcopy(closest, b);
		return;
	}
	
	// Check if P in edge region of AB, if so return projection of P onto AB
	float vc = d1*d4 - d3*d2;
	if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
	{
		// barycentric coordinates (1-v,v,0)
		float v = d1 / (d1 - d3);
		closest[0] = a[0] + v * ab[0];
		closest[1] = a[1] + v * ab[1];
		closest[2] = a[2] + v * ab[2];
		return;
	}
	
	// Check if P in vertex region outside C
	float cp[3];
	rdVsub(cp, p, c);
	float d5 = rdVdot(ab, cp);
	float d6 = rdVdot(ac, cp);
	if (d6 >= 0.0f && d5 <= d6)
	{
		// barycentric coordinates (0,0,1)
		rdVcopy(closest, c);
		return;
	}
	
	// Check if P in edge region of AC, if so return projection of P onto AC
	float vb = d5*d2 - d1*d6;
	if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
	{
		// barycentric coordinates (1-w,0,w)
		float w = d2 / (d2 - d6);
		closest[0] = a[0] + w * ac[0];
		closest[1] = a[1] + w * ac[1];
		closest[2] = a[2] + w * ac[2];
		return;
	}
	
	// Check if P in edge region of BC, if so return projection of P onto BC
	float va = d3*d6 - d5*d4;
	if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
	{
		// barycentric coordinates (0,1-w,w)
		float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
		closest[0] = b[0] + w * (c[0] - b[0]);
		closest[1] = b[1] + w * (c[1] - b[1]);
		closest[2] = b[2] + w * (c[2] - b[2]);
		return;
	}
	
	// P inside face region. Compute Q through its barycentric coordinates (u,v,w)
	float denom = 1.0f / (va + vb + vc);
	float v = vb * denom;
	float w = vc * denom;
	closest[0] = a[0] + ab[0] * v + ac[0] * w;
	closest[1] = a[1] + ab[1] * v + ac[1] * w;
	closest[2] = a[2] + ab[2] * v + ac[2] * w;
}

bool rdIntersectSegmentPoly2D(const float* p0, const float* p1,
							  const float* verts, int nverts,
							  float& tmin, float& tmax,
							  int& segMin, int& segMax)
{
	static const float EPS = 0.000001f;
	
	tmin = 0;
	tmax = 1;
	segMin = -1;
	segMax = -1;
	
	float dir[3];
	rdVsub(dir, p1, p0);
	
	for (int i = 0, j = nverts-1; i < nverts; j=i++)
	{
		float edge[3], diff[3];
		rdVsub(edge, &verts[i*3], &verts[j*3]);
		rdVsub(diff, p0, &verts[j*3]);
		const float n = rdVperp2D(edge, diff);
		const float d = rdVperp2D(dir, edge);
		if (rdMathFabsf(d) < EPS)
		{
			// S is nearly parallel to this edge
			if (n < 0)
				return false;
			else
				continue;
		}
		const float t = n / d;
		if (d < 0)
		{
			// segment S is entering across this edge
			if (t > tmin)
			{
				tmin = t;
				segMin = j;
				// S enters after leaving polygon
				if (tmin > tmax)
					return false;
			}
		}
		else
		{
			// segment S is leaving across this edge
			if (t < tmax)
			{
				tmax = t;
				segMax = j;
				// S leaves before entering polygon
				if (tmax < tmin)
					return false;
			}
		}
	}
	
	return true;
}

float rdDistancePtSegSqr2D(const float* pt, const float* p, const float* q, float& t)
{
	float pqx = q[0] - p[0];
	float pqy = q[1] - p[1];
	float dx = pt[0] - p[0];
	float dy = pt[1] - p[1];
	float d = pqx*pqx + pqy*pqy;
	t = pqx*dx + pqy*dy;
	if (d > 0) t /= d;
	if (t < 0) t = 0;
	else if (t > 1) t = 1;
	dx = p[0] + t*pqx - pt[0];
	dy = p[1] + t*pqy - pt[1];
	return dx*dx + dy*dy;
}

void rdCalcPolyCenter(float* tc, const unsigned short* idx, int nidx, const float* verts)
{
	tc[0] = 0.0f;
	tc[1] = 0.0f;
	tc[2] = 0.0f;
	for (int j = 0; j < nidx; ++j)
	{
		const float* v = &verts[idx[j]*3];
		tc[0] += v[0];
		tc[1] += v[1];
		tc[2] += v[2];
	}
	const float s = 1.0f / nidx;
	tc[0] *= s;
	tc[1] *= s;
	tc[2] *= s;
}

bool rdClosestHeightPointTriangle(const float* p, const float* a, const float* b, const float* c, float& h)
{
	float v0[3], v1[3], v2[3];

	rdVsub(v0, c, a);
	rdVsub(v1, b, a);
	rdVsub(v2, p, a);

	// Compute scaled barycentric coordinates
	float denom = v0[0] * v1[1] - v0[1] * v1[0];
	if (rdMathFabsf(denom) < RD_EPS)
		return false;

	float u = v1[1] * v2[0] - v1[0] * v2[1];
	float v = v0[0] * v2[1] - v0[1] * v2[0];

	if (denom < 0) {
		denom = -denom;
		u = -u;
		v = -v;
	}

	// If point lies inside the triangle, return interpolated zcoord.
	if (u >= 0.0f && v >= 0.0f && (u + v) <= denom) {
		h = a[2] + (v0[2] * u + v1[2] * v) / denom;
		return true;
	}
	return false;
}

/// @par
///
/// All points are projected onto the xy-plane, so the z-values are ignored.
bool rdPointInPolygon(const float* pt, const float* verts, const int nverts)
{
	// TODO: Replace pnpoly with triArea2D tests?
	int i, j;
	bool c = false;
	for (i = 0, j = nverts-1; i < nverts; j = i++)
	{
		const float* vi = &verts[i*3];
		const float* vj = &verts[j*3];
		if (((vi[1] > pt[1]) != (vj[1] > pt[1])) &&
			(pt[0] < (vj[0]-vi[0]) * (pt[1]-vi[1]) / (vj[1]-vi[1]) + vi[0]) )
			c = !c;
	}
	return c;
}

bool rdDistancePtPolyEdgesSqr(const float* pt, const float* verts, const int nverts,
							  float* ed, float* et)
{
	// TODO: Replace pnpoly with triArea2D tests?
	int i, j;
	bool c = false;
	for (i = 0, j = nverts-1; i < nverts; j = i++)
	{
		const float* vi = &verts[i*3];
		const float* vj = &verts[j*3];
		if (((vi[1] > pt[1]) != (vj[1] > pt[1])) &&
			(pt[0] < (vj[0]-vi[0]) * (pt[1]-vi[1]) / (vj[1]-vi[1]) + vi[0]) )
			c = !c;
		ed[j] = rdDistancePtSegSqr2D(pt, vj, vi, et[j]);
	}
	return c;
}

static void projectPoly(const float* axis, const float* poly, const int npoly,
						float& rmin, float& rmax)
{
	rmin = rmax = rdVdot2D(axis, &poly[0]);
	for (int i = 1; i < npoly; ++i)
	{
		const float d = rdVdot2D(axis, &poly[i*3]);
		rmin = rdMin(rmin, d);
		rmax = rdMax(rmax, d);
	}
}

inline bool overlapRange(const float amin, const float amax,
						 const float bmin, const float bmax,
						 const float eps)
{
	return ((amin+eps) > bmax || (amax-eps) < bmin) ? false : true;
}

/// @par
///
/// All vertices are projected onto the xy-plane, so the z-values are ignored.
bool rdOverlapPolyPoly2D(const float* polya, const int npolya,
						 const float* polyb, const int npolyb)
{
	const float eps = 1e-4f;
	
	for (int i = 0, j = npolya-1; i < npolya; j=i++)
	{
		const float* va = &polya[j*3];
		const float* vb = &polya[i*3];
		const float n[3] = { vb[1]-va[1], 0, -(vb[0]-va[0]) };
		float amin,amax,bmin,bmax;
		projectPoly(n, polya, npolya, amin,amax);
		projectPoly(n, polyb, npolyb, bmin,bmax);
		if (!overlapRange(amin,amax, bmin,bmax, eps))
		{
			// Found separating axis
			return false;
		}
	}
	for (int i = 0, j = npolyb-1; i < npolyb; j=i++)
	{
		const float* va = &polyb[j*3];
		const float* vb = &polyb[i*3];
		const float n[3] = { vb[1]-va[1], 0, -(vb[0]-va[0]) };
		float amin,amax,bmin,bmax;
		projectPoly(n, polya, npolya, amin,amax);
		projectPoly(n, polyb, npolyb, bmin,bmax);
		if (!overlapRange(amin,amax, bmin,bmax, eps))
		{
			// Found separating axis
			return false;
		}
	}
	return true;
}

// Returns a random point in a convex polygon.
// Adapted from Graphics Gems article.
void rdRandomPointInConvexPoly(const float* pts, const int npts, float* areas,
							   const float s, const float t, float* out)
{
	// Calc triangle areas
	float areasum = 0.0f;
	for (int i = 2; i < npts; i++) {
		areas[i] = rdTriArea2D(&pts[0], &pts[i*3], &pts[(i-1)*3]);
		areasum += rdMax(0.001f, areas[i]);
	}
	// Find sub triangle weighted by area.
	const float thr = s*areasum;
	float acc = 0.0f;
	float u = 1.0f;
	int tri = npts - 1;
	for (int i = 2; i < npts; i++) {
		const float dacc = areas[i];
		if (thr >= acc && thr < (acc+dacc))
		{
			u = (thr - acc) / dacc;
			tri = i;
			break;
		}
		acc += dacc;
	}
	
	float v = rdMathSqrtf(t);
	
	const float a = 1 - v;
	const float b = (1 - u) * v;
	const float c = u * v;
	const float* pa = &pts[0];
	const float* pb = &pts[tri*3];
	const float* pc = &pts[(tri-1)*3];
	
	out[0] = a*pa[0] + b*pb[0] + c*pc[0];
	out[1] = a*pa[1] + b*pb[1] + c*pc[1];
	out[2] = a*pa[2] + b*pb[2] + c*pc[2];
}

bool rdIntersectSegSeg2D(const float* ap, const float* aq,
						 const float* bp, const float* bq,
						 float& s, float& t)
{
	float u[3], v[3], w[3];
	rdVsub(u,aq,ap);
	rdVsub(v,bq,bp);
	rdVsub(w,ap,bp);
	float d = rdVperp2D(u,v);
	if (rdMathFabsf(d) < RD_EPS) return false;
	s = rdVperp2D(v,w) / d;
	t = rdVperp2D(u,w) / d;
	return true;
}

float rdDistancePtLine2d(const float* pt, const float* p, const float* q)
{
	float pqx = q[0] - p[0];
	float pqy = q[1] - p[1];
	float dx = pt[0] - p[0];
	float dy = pt[1] - p[1];
	float d = pqx * pqx + pqy * pqy;
	float t = pqx * dx + pqy * dy;
	if (d != 0) t /= d;
	dx = p[0] + t * pqx - pt[0];
	dy = p[1] + t * pqy - pt[1];
	return dx * dx + dy * dy;
}

static const unsigned char XP = 1 << 0;
static const unsigned char ZP = 1 << 1;
static const unsigned char XM = 1 << 2;
static const unsigned char ZM = 1 << 3;

unsigned char rdClassifyPointOutsideBounds(const float* pt, const float* bmin, const float* bmax)
{
	unsigned char outcode = 0; 
	outcode |= (pt[0] >= bmax[0]) ? XP : 0;
	outcode |= (pt[1] >= bmax[1]) ? ZP : 0;
	outcode |= (pt[0] < bmin[0])  ? XM : 0;
	outcode |= (pt[1] < bmin[1])  ? ZM : 0;

	switch (outcode)
	{
	case XP: return 0;
	case XP|ZP: return 1;
	case ZP: return 2;
	case XM|ZP: return 3;
	case XM: return 4;
	case XM|ZM: return 5;
	case ZM: return 6;
	case XP|ZM: return 7;
	};

	return 0xff;
}
