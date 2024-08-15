#include "Shared/Include/SharedMath.h"
#include "Shared/Include/SharedCommon.h"
#include "Detour/Include/DetourNavMesh.h"
#include "Detour/Include/DetourNavMeshQuery.h"
#include "DebugUtils/Include/DetourDump.h"

bool duDumpTraverseLinkDetail(const dtNavMesh& mesh, const dtNavMeshQuery* query, const int traverseType, duFileIO* const io)
{
	if (!io)
	{
		printf("duDumpTraverseLinkDetail: input IO is null.\n");
		return false;
	}
	if (!io->isWriting())
	{
		printf("duDumpTraverseLinkDetail: input IO not writing.\n");
		return false;
	}

	char buf[2048];
	int bufCount = 0;

	rdTempVector<unsigned char> distanceVec;
	rdTempVector<float> slopeVec;

	int totTraverseLinkCount = 0;
	const int tileCount = mesh.getTileCount();

	for (int i = 0; i < tileCount; i++)
	{
		const dtMeshTile* tile = mesh.getTile(i);
		const dtMeshHeader* header = tile->header;

		bool writeTileDetail = false;

		for (int j = 0; j < header->polyCount; ++j)
		{
			const dtPoly* startPoly = &tile->polys[j];

			if (startPoly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)	// Skip off-mesh links.
				continue;

			if (tile->links[j].ref == 0)
				continue;

			// Iterate through links in the poly.
			for (int k = startPoly->firstLink; k != DT_NULL_LINK; k = tile->links[k].next)
			{
				const dtLink* link = &tile->links[k];

				// Skip "normal" links (non-jumping ones).
				if (link->traverseType == DT_NULL_TRAVERSE_TYPE)
					continue;

				// Filter out anything not matching input.
				if (traverseType != -1 && link->traverseType != traverseType)
					continue;

				const dtPoly* endPoly;
				const dtMeshTile* endTile;

				if (dtStatusFailed(mesh.getTileAndPolyByRef(link->ref, &endTile, &endPoly)))
					continue;

				if (endPoly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)	// Skip off-mesh links.
					continue;

				if (!writeTileDetail)
				{
					io->write("{\n", 2);

					bufCount = snprintf(buf, sizeof(buf), "\ttile: %d\n", i);
					io->write(buf, bufCount);

					writeTileDetail = true;
				}

				io->write("\t{\n", 3);

				float startPos[3];
				float endPos[3];

				query->getEdgeMidPoint(mesh.getPolyRefBase(tile) | (dtPolyRef)j, link->ref, startPos);
				query->getEdgeMidPoint(link->ref, mesh.getPolyRefBase(tile) | (dtPolyRef)j, endPos);

				// note(amos): the lowest slope is the highest slope in reverse
				// as we always have a reverse link; store the absolute value!!
				const float slopeAngle = rdMathFabsf(rdCalcSlopeAngle(startPos, endPos));

				//const bool hasReverseLink = link->reverseLink != DT_NULL_TRAVERSE_REVERSE_LINK;

				bufCount = snprintf(buf, sizeof(buf), "\t\tlink: %d\n", k);
				io->write(buf, bufCount);
				bufCount = snprintf(buf, sizeof(buf), "\t\tstartPos: <%g, %g, %g>\n", startPos[0], startPos[1], startPos[2]);
				io->write(buf, bufCount);
				bufCount = snprintf(buf, sizeof(buf), "\t\tendPos: <%g, %g, %g>\n", endPos[0], endPos[1], endPos[2]);
				io->write(buf, bufCount);
				bufCount = snprintf(buf, sizeof(buf), "\t\tslopeAngle: %g\n", slopeAngle);
				io->write(buf, bufCount);
				bufCount = snprintf(buf, sizeof(buf), "\t\ttraverseType: %hhu\n", link->traverseType);
				io->write(buf, bufCount);
				bufCount = snprintf(buf, sizeof(buf), "\t\ttraverseDist: %hhu\n", link->traverseDist);
				io->write(buf, bufCount);
				bufCount = snprintf(buf, sizeof(buf), "\t\treverseLink: %hu\n", link->reverseLink);
				io->write(buf, bufCount);

				io->write("\t}\n", 3);

				distanceVec.push_back(link->traverseDist);
				slopeVec.push_back(slopeAngle);

				totTraverseLinkCount++;
			}
		}

		if (writeTileDetail)
			io->write("}\n", 2);
	}

	std::sort(distanceVec.begin(), distanceVec.end());
	std::sort(slopeVec.begin(), slopeVec.end());

	if (distanceVec.size() > 0)
	{
		const int lowestDist = distanceVec[0];
		const int highestDist = distanceVec[distanceVec.size()-1];

		bufCount = snprintf(buf, sizeof(buf), "lowestDist: %d\n", lowestDist);
		io->write(buf, bufCount);

		bufCount = snprintf(buf, sizeof(buf), "highestDist: %d\n", highestDist);
		io->write(buf, bufCount);
	}

	if (slopeVec.size() > 0)
	{
		const float lowestSlope = slopeVec[0];
		const float highestSlope = slopeVec[slopeVec.size()-1];

		bufCount = snprintf(buf, sizeof(buf), "lowestSlope: %g\n", lowestSlope);
		io->write(buf, bufCount);

		bufCount = snprintf(buf, sizeof(buf), "highestSlope: %g\n", highestSlope);
		io->write(buf, bufCount);
	}

	bufCount = snprintf(buf, sizeof(buf), "totalLinkCount: %d\n", totTraverseLinkCount);
	io->write(buf, bufCount);

	return true;
}
