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

#include "Recast/Include/Recast.h"
#include "Shared/Include/SharedAlloc.h"
#include "Shared/Include/SharedAssert.h"

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

namespace
{
/// Allocates and constructs an object of the given type, returning a pointer.
/// @param[in]		allocLifetime	Allocation lifetime hint
template<typename T>
T* rcNew(const rdAllocHint allocLifetime)
{
	T* ptr = (T*)rdAlloc(sizeof(T), allocLifetime);
	::new(rdNewTag(), (void*)ptr) T();
	return ptr;
}

/// Destroys and frees an object allocated with rcNew.
/// @param[in]     ptr    The object pointer to delete.
template<typename T>
void rcDelete(T* ptr)
{
	if (ptr)
	{
		ptr->~T();
		rdFree((void*)ptr);
	}
}
} // anonymous namespace

void rcContext::log(const rcLogCategory category, const char* format, ...)
{
	if (!m_logEnabled)
	{
		return;
	}
	static const int MSG_SIZE = 512;
	char msg[MSG_SIZE];
	va_list argList;
	va_start(argList, format);
	int len = vsnprintf(msg, MSG_SIZE, format, argList);
	if (len >= MSG_SIZE)
	{
		len = MSG_SIZE - 1;
		msg[MSG_SIZE - 1] = '\0';

		const char* errorMessage = "Log message was truncated";
		doLog(RC_LOG_ERROR, errorMessage, (int)strlen(errorMessage));
	}
	va_end(argList);
	doLog(category, msg, len);
}

void rcContext::doResetLog()
{
	// Defined out of line to fix the weak v-tables warning
}

rcHeightfield* rcAllocHeightfield()
{
	return rcNew<rcHeightfield>(RD_ALLOC_PERM);
}

void rcFreeHeightField(rcHeightfield* heightfield)
{
	rcDelete(heightfield);
}

rcHeightfield::rcHeightfield()
: width()
, height()
, bmin()
, bmax()
, cs()
, ch()
, spans()
, pools()
, freelist()
{
}

rcHeightfield::~rcHeightfield()
{
	// Delete span array.
	rdFree(spans);
	// Delete span pools.
	while (pools)
	{
		rcSpanPool* next = pools->next;
		rdFree(pools);
		pools = next;
	}
}

rcCompactHeightfield* rcAllocCompactHeightfield()
{
	return rcNew<rcCompactHeightfield>(RD_ALLOC_PERM);
}

void rcFreeCompactHeightfield(rcCompactHeightfield* compactHeightfield)
{
	rcDelete(compactHeightfield);
}

rcCompactHeightfield::rcCompactHeightfield()
: width()
, height()
, spanCount()
, walkableHeight()
, walkableClimb()
, borderSize()
, maxDistance()
, maxRegions()
, bmin()
, bmax()
, cs()
, ch()
, cells()
, spans()
, dist()
, areas()
{
}

rcCompactHeightfield::~rcCompactHeightfield()
{
	rdFree(cells);
	rdFree(spans);
	rdFree(dist);
	rdFree(areas);
}

rcHeightfieldLayerSet* rcAllocHeightfieldLayerSet()
{
	return rcNew<rcHeightfieldLayerSet>(RD_ALLOC_PERM);
}

void rcFreeHeightfieldLayerSet(rcHeightfieldLayerSet* layerSet)
{
	rcDelete(layerSet);
}

rcHeightfieldLayerSet::rcHeightfieldLayerSet()
: layers()
, nlayers()
{
}

rcHeightfieldLayerSet::~rcHeightfieldLayerSet()
{
	for (int i = 0; i < nlayers; ++i)
	{
		rdFree(layers[i].heights);
		rdFree(layers[i].areas);
		rdFree(layers[i].cons);
	}
	rdFree(layers);
}


rcContourSet* rcAllocContourSet()
{
	return rcNew<rcContourSet>(RD_ALLOC_PERM);
}

void rcFreeContourSet(rcContourSet* contourSet)
{
	rcDelete(contourSet);
}

rcContourSet::rcContourSet()
: conts()
, nconts()
, bmin()
, bmax()
, cs()
, ch()
, width()
, height()
, borderSize()
, maxError()
{
}

rcContourSet::~rcContourSet()
{
	for (int i = 0; i < nconts; ++i)
	{
		rdFree(conts[i].verts);
		rdFree(conts[i].rverts);
	}
	rdFree(conts);
}

rcPolyMesh* rcAllocPolyMesh()
{
	return rcNew<rcPolyMesh>(RD_ALLOC_PERM);
}

void rcFreePolyMesh(rcPolyMesh* polyMesh)
{
	rcDelete(polyMesh);
}

rcPolyMesh::rcPolyMesh()
: verts()
, polys()
, regs()
, flags()
, areas()
, nverts()
, npolys()
, maxpolys()
, nvp()
, bmin()
, bmax()
, cs()
, ch()
, borderSize()
, maxEdgeError()
{
}

rcPolyMesh::~rcPolyMesh()
{
	rdFree(verts);
	rdFree(polys);
	rdFree(regs);
	rdFree(flags);
	rdFree(areas);
}

rcPolyMeshDetail* rcAllocPolyMeshDetail()
{
	return rcNew<rcPolyMeshDetail>(RD_ALLOC_PERM);
}

void rcFreePolyMeshDetail(rcPolyMeshDetail* detailMesh)
{
	if (detailMesh == NULL)
	{
		return;
	}
	rdFree(detailMesh->meshes);
	rdFree(detailMesh->verts);
	rdFree(detailMesh->tris);
	rdFree(detailMesh);
}

rcPolyMeshDetail::rcPolyMeshDetail()
: meshes()
, verts()
, tris()
, nmeshes()
, nverts()
, ntris()
{
}

void rcCalcBounds(const float* verts, int numVerts, float* minBounds, float* maxBounds)
{
	// Calculate bounding box.
	rdVcopy(minBounds, verts);
	rdVcopy(maxBounds, verts);
	for (int i = 1; i < numVerts; ++i)
	{
		const float* v = &verts[i * 3];
		rdVmin(minBounds, v);
		rdVmax(maxBounds, v);
	}
}

void rcCalcGridSize(const float* minBounds, const float* maxBounds, const float cellSize, int* sizeX, int* sizeY)
{
	*sizeX = (int)((maxBounds[0] - minBounds[0]) / cellSize + 0.5f);
	*sizeY = (int)((maxBounds[1] - minBounds[1]) / cellSize + 0.5f);
}

bool rcCreateHeightfield(rcContext* context, rcHeightfield& heightfield, int sizeX, int sizeZ,
                         const float* minBounds, const float* maxBounds,
                         float cellSize, float cellHeight)
{
	rdIgnoreUnused(context);

	heightfield.width = sizeX;
	heightfield.height = sizeZ;
	rdVcopy(heightfield.bmin, minBounds);
	rdVcopy(heightfield.bmax, maxBounds);
	heightfield.cs = cellSize;
	heightfield.ch = cellHeight;
	heightfield.spans = (rcSpan**)rdAlloc(sizeof(rcSpan*) * heightfield.width * heightfield.height, RD_ALLOC_PERM);
	if (!heightfield.spans)
	{
		return false;
	}
	memset(heightfield.spans, 0, sizeof(rcSpan*) * heightfield.width * heightfield.height);
	return true;
}

static void calcTriNormal(const float* v0, const float* v1, const float* v2, float* faceNormal)
{
	float e0[3], e1[3];
	rdVsub(e0, v1, v0);
	rdVsub(e1, v2, v0);
	rdVcross(faceNormal, e0, e1);
	rdVnormalize(faceNormal);
}

void rcMarkWalkableTriangles(rcContext* context, const float walkableSlopeAngle,
                             const float* verts, const int numVerts,
                             const int* tris, const int numTris,
                             unsigned char* triAreaIDs)
{
	rdIgnoreUnused(context);
	rdIgnoreUnused(numVerts);

	const float walkableThr = cosf(walkableSlopeAngle / 180.0f * RD_PI);

	float norm[3];

	for (int i = 0; i < numTris; ++i)
	{
		const int* tri = &tris[i * 3];
		calcTriNormal(&verts[tri[0] * 3], &verts[tri[1] * 3], &verts[tri[2] * 3], norm);
		// Check if the face is walkable.
		if (norm[2] > walkableThr)
		{
			triAreaIDs[i] = RC_WALKABLE_AREA;
		}
	}
}

void rcClearUnwalkableTriangles(rcContext* context, const float walkableSlopeAngle,
                                const float* verts, int numVerts,
                                const int* tris, int numTris,
                                unsigned char* triAreaIDs)
{
	rdIgnoreUnused(context);
	rdIgnoreUnused(numVerts);

	// The minimum Z value for a face normal of a triangle with a walkable slope.
	const float walkableLimitZ = cosf(walkableSlopeAngle / 180.0f * RD_PI);

	float faceNormal[3];
	for (int i = 0; i < numTris; ++i)
	{
		const int* tri = &tris[i * 3];
		calcTriNormal(&verts[tri[0] * 3], &verts[tri[1] * 3], &verts[tri[2] * 3], faceNormal);
		// Check if the face is walkable.
		if (faceNormal[2] <= walkableLimitZ)
		{
			triAreaIDs[i] = RC_NULL_AREA;
		}
	}
}

int rcGetHeightFieldSpanCount(rcContext* context, const rcHeightfield& heightfield)
{
	rdIgnoreUnused(context);

	const int numCols = heightfield.width * heightfield.height;
	int spanCount = 0;
	for (int columnIndex = 0; columnIndex < numCols; ++columnIndex)
	{
		for (rcSpan* span = heightfield.spans[columnIndex]; span != NULL; span = span->next)
		{
			if (span->area != RC_NULL_AREA)
			{
				spanCount++;
			}
		}
	}
	return spanCount;
}

bool rcBuildCompactHeightfield(rcContext* context, const int walkableHeight, const int walkableClimb,
                               const rcHeightfield& heightfield, rcCompactHeightfield& compactHeightfield)
{
	rdAssert(context);

	rcScopedTimer timer(context, RC_TIMER_BUILD_COMPACTHEIGHTFIELD);

	const int xSize = heightfield.width;
	const int ySize = heightfield.height;
	const int spanCount = rcGetHeightFieldSpanCount(context, heightfield);

	// Fill in header.
	compactHeightfield.width = xSize;
	compactHeightfield.height = ySize;
	compactHeightfield.spanCount = spanCount;
	compactHeightfield.walkableHeight = walkableHeight;
	compactHeightfield.walkableClimb = walkableClimb;
	compactHeightfield.maxRegions = 0;
	rdVcopy(compactHeightfield.bmin, heightfield.bmin);
	rdVcopy(compactHeightfield.bmax, heightfield.bmax);
	compactHeightfield.bmax[2] += walkableHeight * heightfield.ch;
	compactHeightfield.cs = heightfield.cs;
	compactHeightfield.ch = heightfield.ch;
	compactHeightfield.cells = (rcCompactCell*)rdAlloc(sizeof(rcCompactCell) * xSize * ySize, RD_ALLOC_PERM);
	if (!compactHeightfield.cells)
	{
		context->log(RC_LOG_ERROR, "rcBuildCompactHeightfield: Out of memory 'chf.cells' (%d)", xSize * ySize);
		return false;
	}
	memset(compactHeightfield.cells, 0, sizeof(rcCompactCell) * xSize * ySize);
	compactHeightfield.spans = (rcCompactSpan*)rdAlloc(sizeof(rcCompactSpan) * spanCount, RD_ALLOC_PERM);
	if (!compactHeightfield.spans)
	{
		context->log(RC_LOG_ERROR, "rcBuildCompactHeightfield: Out of memory 'chf.spans' (%d)", spanCount);
		return false;
	}
	memset(compactHeightfield.spans, 0, sizeof(rcCompactSpan) * spanCount);
	compactHeightfield.areas = (unsigned char*)rdAlloc(sizeof(unsigned char) * spanCount, RD_ALLOC_PERM);
	if (!compactHeightfield.areas)
	{
		context->log(RC_LOG_ERROR, "rcBuildCompactHeightfield: Out of memory 'chf.areas' (%d)", spanCount);
		return false;
	}
	memset(compactHeightfield.areas, RC_NULL_AREA, sizeof(unsigned char) * spanCount);

	const int MAX_HEIGHT = 0xffff;

	// Fill in cells and spans.
	int currentCellIndex = 0;
	const int numColumns = xSize * ySize;
	for (int columnIndex = 0; columnIndex < numColumns; ++columnIndex)
	{
		const rcSpan* span = heightfield.spans[columnIndex];
			
		// If there are no spans at this cell, just leave the data to index=0, count=0.
		if (span == NULL)
		{
			continue;
		}
			
		rcCompactCell& cell = compactHeightfield.cells[columnIndex];
		cell.index = currentCellIndex;
		cell.count = 0;

		for (; span != NULL; span = span->next)
		{
			if (span->area != RC_NULL_AREA)
			{
				const int bot = (int)span->smax;
				const int top = span->next ? (int)span->next->smin : MAX_HEIGHT;
				compactHeightfield.spans[currentCellIndex].z = (unsigned short)rdClamp(bot, 0, 0xffff);
				compactHeightfield.spans[currentCellIndex].h = (unsigned char)rdClamp(top - bot, 0, 0xff);
				compactHeightfield.areas[currentCellIndex] = span->area;
				currentCellIndex++;
				cell.count++;
			}
		}
	}
	
	// Find neighbour connections.
	const int MAX_LAYERS = RC_NOT_CONNECTED - 1;
	int maxLayerIndex = 0;
	const int yStride = xSize; // for readability
	for (int y = 0; y < ySize; ++y)
	{
		for (int x = 0; x < xSize; ++x)
		{
			const rcCompactCell& cell = compactHeightfield.cells[x + y * yStride];
			for (int i = (int)cell.index, ni = (int)(cell.index + cell.count); i < ni; ++i)
			{
				rcCompactSpan& span = compactHeightfield.spans[i];

				for (int dir = 0; dir < 4; ++dir)
				{
					rcSetCon(span, dir, RC_NOT_CONNECTED);
					const int neighborX = x + rcGetDirOffsetX(dir);
					const int neighborY = y + rcGetDirOffsetY(dir);
					// First check that the neighbour cell is in bounds.
					if (neighborX < 0 || neighborY < 0 || neighborX >= xSize || neighborY >= ySize)
					{
						continue;
					}

					// Iterate over all neighbour spans and check if any of the is
					// accessible from current cell.
					const rcCompactCell& neighborCell = compactHeightfield.cells[neighborX + neighborY * yStride];
					for (int k = (int)neighborCell.index, nk = (int)(neighborCell.index + neighborCell.count); k < nk; ++k)
					{
						const rcCompactSpan& neighborSpan = compactHeightfield.spans[k];
						const int bot = rdMax(span.z, neighborSpan.z);
						const int top = rdMin(span.z + span.h, neighborSpan.z + neighborSpan.h);

						// Check that the gap between the spans is walkable,
						// and that the climb height between the gaps is not too high.
						if ((top - bot) >= walkableHeight && rdAbs((int)neighborSpan.z - (int)span.z) <= walkableClimb)
						{
							// Mark direction as walkable.
							const int layerIndex = k - (int)neighborCell.index;
							if (layerIndex < 0 || layerIndex > MAX_LAYERS)
							{
								maxLayerIndex = rdMax(maxLayerIndex, layerIndex);
								continue;
							}
							rcSetCon(span, dir, layerIndex);
							break;
						}
					}
				}
			}
		}
	}

	if (maxLayerIndex > MAX_LAYERS)
	{
		context->log(RC_LOG_ERROR, "rcBuildCompactHeightfield: Heightfield has too many layers %d (max: %d)",
		         maxLayerIndex, MAX_LAYERS);
	}

	return true;
}
