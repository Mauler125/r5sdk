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

#ifndef RECASTDETOURCONST_H
#define RECASTDETOURCONST_H

/// The total number of bits in an bit cell integer.
static const int RD_BITS_PER_BIT_CELL = 32;

/// An value which indicates an invalid index within a mesh.
/// @note This does not necessarily indicate an error.
/// @see rcPolyMesh::polys
static const unsigned short RD_MESH_NULL_IDX = 0xffff;

/// Detail triangle edge flags used for various functions and fields.
/// For an example, see dtNavMesh::connectTraverseLinks().
enum rdDetailTriEdgeFlags
{
	RD_DETAIL_EDGE_BOUNDARY = 1<<0,		///< Detail triangle edge is part of the poly boundary
};

#endif // RECASTDETOURCONST_H
