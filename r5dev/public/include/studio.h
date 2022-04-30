#ifndef STUDIO_H
#define STUDIO_H
#include "mathlib/vector.h"

#pragma pack(push, 1)
struct studiohdr_t
{
	int id;                  // 'IDST'
	int version;             // R5 = '6'
	int checksum;
	int tableIndex;          // Offset

	char name[0x40];

	int length;              // size of data

	Vector3 eyeposition;     // ideal eye position
	Vector3 illumposition;   // illumination center
	Vector3 hull_min;        // ideal movement hull size
	Vector3 hull_max;
	Vector3 view_bbmin;      // clipping bounding box
	Vector3 view_bbmax;

	int flags;

	int numbones;            // bones
	int boneindex;

	int numbonecontrollers;
	int bonecontrollerindex;

	int numhitboxsets;
	int hitboxsetindex;

	int numlocalanim;        // animations/poses
	int localanimindex;      // animation descriptions

	int numlocalseq;         // sequences
	int localseqindex;

	int activitylistversion; // initialization flag - have the sequences been indexed ?
	int eventsindexed;

	int numtextures;
	int textureindex;

	int numcdtextures;
	int cdtextureindex;

	int numskinref;      // Total number of references (submeshes)
	int numskinfamilies; // Total skins per reference
	int skinindex;       // Offset to data

	int numbodyparts;
	int bodypartindex;

	int numlocalattachments;
	int localattachmentindex;

	uint8_t Unknown2[0x14];

	int submeshLodsIndex;

	uint8_t Unknown3[0x64];
	int boneRemapInfoIndex;
	int boneRemapCount;
};
#pragma pack(pop)

class CStudioHdr
{
public:
	static int LookupSequence(CStudioHdr* pStudio, const char* pszName);

	int64_t m_nUnk0;
	studiohdr_t* m_pStudioHdr;
	uint8_t m_Pad[0x85A]; // Compatible between S0 - S3.
	void* m_pMdlCacheVTable;
};

#endif // STUDIO_H
