#ifndef BSPFILE_H
#define BSPFILE_H

//=============================================================================

enum
{
	LUMP_ENTITIES                        = 0x0000,
	LUMP_PLANES                          = 0x0001,
	LUMP_TEXTURE_DATA                    = 0x0002,
	LUMP_VERTICES                        = 0x0003,
	LUMP_LIGHTPROBE_PARENT_INFOS         = 0x0004,
	LUMP_SHADOW_ENVIRONMENTS             = 0x0005,
	LUMP_UNUSED_6                        = 0x0006,
	LUMP_UNUSED_7                        = 0x0007,
	LUMP_UNUSED_8                        = 0x0008,
	LUMP_UNUSED_9                        = 0x0009,
	LUMP_UNUSED_10                       = 0x000A,
	LUMP_UNUSED_11                       = 0x000B,
	LUMP_UNUSED_12                       = 0x000C,
	LUMP_UNUSED_13                       = 0x000D,
	LUMP_MODELS                          = 0x000E,
	LUMP_SURFACE_NAMES                   = 0x000F,
	LUMP_CONTENTS_MASKS                  = 0x0010,
	LUMP_SURFACE_PROPERTIES              = 0x0011,
	LUMP_BVH_NODES                       = 0x0012,
	LUMP_BVH_LEAF_DATA                   = 0x0013,
	LUMP_PACKED_VERTICES                 = 0x0014,
	LUMP_UNUSED_21                       = 0x0015,
	LUMP_UNUSED_22                       = 0x0016,
	LUMP_UNUSED_23                       = 0x0017,
	LUMP_ENTITY_PARTITIONS               = 0x0018,
	LUMP_UNUSED_25                       = 0x0019,
	LUMP_UNUSED_26                       = 0x001A,
	LUMP_UNUSED_27                       = 0x001B,
	LUMP_UNUSED_28                       = 0x001C,
	LUMP_UNUSED_29                       = 0x001D,
	LUMP_VERTEX_NORMALS                  = 0x001E,
	LUMP_UNUSED_31                       = 0x001F,
	LUMP_UNUSED_32                       = 0x0020,
	LUMP_UNUSED_33                       = 0x0021,
	LUMP_UNUSED_34                       = 0x0022,

	// The game lump is a method of adding game-specific lumps.
	LUMP_GAME_LUMP                       = 0x0023,
	LUMP_UNUSED_36                       = 0x0024,
	LUMP_UNKNOWN_37                      = 0x0025, // connected to VIS lumps
	LUMP_UNKNOWN_38                      = 0x0026, // connected to CSM lumps
	LUMP_UNKNOWN_39                      = 0x0027, // connected to VIS lumps

	// A pak file can be embedded in a .bsp now, and the file system will search the pak
	//  file first for any referenced names, before deferring to the game directory 
	//  file system/pak files and finally the base directory file system/pak files.
	LUMP_PAKFILE                         = 0x0028,
	LUMP_UNUSED_41                       = 0x0029,

	// A map can have a number of cubemap entities in it which cause cubemap renders
	// to be taken after running vrad.
	LUMP_CUBEMAPS                        = 0x002A,
	LUMP_UNKNOWN_43                      = 0x002B,
	LUMP_UNKNOWN_44                      = 0x002C, // Storm Point & Habitat
	LUMP_UNKNOWN_45                      = 0x002D, // Storm Point & Habitat
	LUMP_UNKNOWN_46                      = 0x002E, // Storm Point & Habitat
	LUMP_UNKNOWN_47                      = 0x002F, // Storm Point & Habitat
	LUMP_UNKNOWN_48                      = 0x0030, // Storm Point & Habitat; sometimes unused
	LUMP_UNUSED_49                       = 0x0031,
	LUMP_UNUSED_50                       = 0x0032,
	LUMP_UNUSED_51                       = 0x0033,
	LUMP_UNUSED_52                       = 0x0034,
	LUMP_UNUSED_53                       = 0x0035,
	LUMP_WORLD_LIGHTS                    = 0x0036,
	LUMP_WORLD_LIGHT_PARENT_INFOS        = 0x0037,
	LUMP_UNUSED_56                       = 0x0038,
	LUMP_UNUSED_57                       = 0x0039,
	LUMP_UNUSED_58                       = 0x003A,
	LUMP_UNUSED_59                       = 0x003B,
	LUMP_UNUSED_60                       = 0x003C,
	LUMP_UNUSED_61                       = 0x003D,
	LUMP_UNUSED_62                       = 0x003E,
	LUMP_UNUSED_63                       = 0x003F,
	LUMP_UNUSED_64                       = 0x0040,
	LUMP_UNUSED_65                       = 0x0041,
	LUMP_UNUSED_66                       = 0x0042,
	LUMP_UNUSED_67                       = 0x0043,
	LUMP_UNUSED_68                       = 0x0044,
	LUMP_UNUSED_69                       = 0x0045,
	LUMP_UNUSED_70                       = 0x0046,
	LUMP_VERTEX_UNLIT                    = 0x0047, // VERTEX_RESERVED_0
	LUMP_VERTEX_LIT_FLAT                 = 0x0048, // VERTEX_RESERVED_1
	LUMP_VERTEX_LIT_BUMP                 = 0x0049, // VERTEX_RESERVED_2
	LUMP_VERTEX_UNLIT_TS                 = 0x004A, // VERTEX_RESERVED_3
	LUMP_VERTEX_BLINN_PHONG              = 0x004B, // VERTEX_RESERVED_4
	LUMP_VERTEX_RESERVED_5               = 0x004C,
	LUMP_VERTEX_RESERVED_6               = 0x004D,
	LUMP_VERTEX_RESERVED_7               = 0x004E,
	LUMP_MESH_INDICES                    = 0x004F,
	LUMP_MESHES                          = 0x0050,
	LUMP_MESH_BOUNDS                     = 0x0051,
	LUMP_MATERIAL_SORT                   = 0x0052,
	LUMP_LIGHTMAP_HEADERS                = 0x0053,
	LUMP_UNUSED_84                       = 0x0054,
	LUMP_TWEAK_LIGHTS                    = 0x0055,
	LUMP_UNUSED_86                       = 0x0056,
	LUMP_UNUSED_87                       = 0x0057,
	LUMP_UNUSED_88                       = 0x0058,
	LUMP_UNUSED_89                       = 0x0059,
	LUMP_UNUSED_90                       = 0x005A,
	LUMP_UNUSED_91                       = 0x005B,
	LUMP_UNUSED_92                       = 0x005C,
	LUMP_UNUSED_93                       = 0x005D,
	LUMP_UNUSED_94                       = 0x005E,
	LUMP_UNUSED_95                       = 0x005F,
	LUMP_UNUSED_96                       = 0x0060,
	LUMP_UNKNOWN_97                      = 0x0061,
	LUMP_LIGHTMAP_DATA_SKY               = 0x0062,
	LUMP_CSM_AABB_NODES                  = 0x0063,
	LUMP_CSM_OBJ_REFERENCES              = 0x0064,
	LUMP_LIGHTPROBES                     = 0x0065, // Changed in S14; 4 trailing padding bytes have been removed from the 'dlightprobe_t' struct.
	LUMP_STATIC_PROP_LIGHTPROBE_INDICES  = 0x0066,
	LUMP_LIGHTPROBE_TREE                 = 0x0067,
	LUMP_LIGHTPROBE_REFERENCES           = 0x0068,
	LUMP_LIGHTMAP_DATA_REAL_TIME_LIGHTS  = 0x0069,
	LUMP_CELL_BSP_NODES                  = 0x006A,
	LUMP_CELLS                           = 0x006B,
	LUMP_PORTALS                         = 0x006C,
	LUMP_PORTAL_VERTICES                 = 0x006D,
	LUMP_PORTAL_EDGES                    = 0x006E,
	LUMP_PORTAL_VERTEX_EDGES             = 0x006F,
	LUMP_PORTAL_VERTEX_REFERENCES        = 0x0070,
	LUMP_PORTAL_EDGE_REFERENCES          = 0x0071,
	LUMP_PORTAL_EDGE_INTERSECT_AT_EDGE   = 0x0072,
	LUMP_PORTAL_EDGE_INTERSECT_AT_VERTEX = 0x0073,
	LUMP_PORTAL_EDGE_INTERSECT_HEADER    = 0x0074,
	LUMP_OCCLUSION_MESH_VERTICES         = 0x0075,
	LUMP_OCCLUSION_MESH_INDICES          = 0x0076,
	LUMP_CELL_AABB_NODES                 = 0x0077,
	LUMP_OBJ_REFERENCES                  = 0x0078,
	LUMP_OBJ_REFERENCE_BOUNDS            = 0x0079,
	LUMP_LIGHTMAP_DATA_RTL_PAGE          = 0x007A,
	LUMP_LEVEL_INFO                      = 0x007B,
	LUMP_SHADOW_MESH_OPAQUE_VERTICES     = 0x007C,
	LUMP_SHADOW_MESH_ALPHA_VERTICES      = 0x007D,
	LUMP_SHADOW_MESH_INDICES             = 0x007E,
	LUMP_SHADOW_MESHES                   = 0x007F,
};

#define	HEADER_LUMPS	128

struct lump_t
{
	int fileofs;
	int filelen;
	int version;
	int uncompLen;
};

struct BSPHeader_t
{
	int ident;
	int version;
	int mapRevision;
	int lastLump;
	lump_t lumps[HEADER_LUMPS];
};

enum GameLumpId_t
{
	GAMELUMP_DETAIL_PROPS = 'dprp',
	GAMELUMP_DETAIL_PROP_LIGHTING = 'dplt',
	GAMELUMP_STATIC_PROPS = 'sprp',
	GAMELUMP_DETAIL_PROP_LIGHTING_HDR = 'dplh',
};

struct dgamelumpheader_t
{
	int lumpCount;
};

struct dgamelump_t
{
	GameLumpId_t id;
	unsigned short flags;
	unsigned short version;
	int fileofs;
	int filelen;
};

struct dlightprobe_t
{
	short ambientSH[12]; // Ambient spherical harmonics coefficients
	short skyDirSunVis[4];
	char staticLightWeights[4];
	short staticLightIndexes[4];
	char pad[4]; // Padding has been removed as of S14
};

#endif // BSPFILE_H
