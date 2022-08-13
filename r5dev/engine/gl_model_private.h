//=============================================================================//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#include "vpc/keyvalues.h"
#include "mathlib/vector.h"
#include "common/qlimits.h"
#include "datacache/imdlcache.h"
#include "public/model_types.h"

#ifndef DEDICATED
#include "game/client/enginesprite.h"
#endif // !DEDICATED
typedef int ModelFileNameHandle_t; // 4 bytes in r5, void* originally.

struct brushdata_t // !! UNCONFIRMED !!
{
	void* pShared; // worldbrushdata_t
	int				firstmodelsurface;
	int				nummodelsurfaces;

	// track the union of all lightstyles on this brush.  That way we can avoid
	// searching all faces if the lightstyle hasn't changed since the last update
	int				nLightstyleLastComputedFrame;
	unsigned short	nLightstyleIndex;	// g_ModelLoader actually holds the allocated data here
	unsigned short	nLightstyleCount;

	unsigned short	renderHandle;
	unsigned short	firstnode;
};

#ifndef DEDICATED
struct spritedata_t // !! UNCONFIRMED !!
{
	int				numframes;
	int				width;
	int				height;
	CEngineSprite* sprite;
};
#endif // !DEDICATED

struct model_t // !! CONFIRMED !!
{
	ModelFileNameHandle_t	fnHandle;
	char				szPathName[MAX_OSPATH];

	int					nLoadFlags;		// mark loaded/not loaded
	int					nServerCount;	// marked at load

	modtype_t			type;
	int					flags;			// MODELFLAG_???

	// volume occupied by the model graphics
	Vector3D			mins, maxs;
	float				radius;
	KeyValues* m_pKeyValues;
	union
	{
		brushdata_t		brush;
		MDLHandle_t		studio;
#ifndef DEDICATED
		spritedata_t	sprite;
#endif // !DEDICATED
	};
};