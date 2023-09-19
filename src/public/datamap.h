#pragma once

typedef enum _fieldtypes
{
	FIELD_VOID = 0,			// No type or value
	FIELD_FLOAT,			// Any floating point value
	FIELD_STRING,			// A string ID (return from ALLOC_STRING)
	FIELD_VECTOR,			// Any vector, QAngle, or AngularImpulse
	FIELD_QUATERNION,		// A quaternion
	FIELD_INTEGER,			// Any integer or enum
	FIELD_BOOLEAN,			// boolean, implemented as an int, I may use this as a hint for compression
	FIELD_SHORT,			// 2 byte integer
	FIELD_CHARACTER,		// a byte
	FIELD_COLOR32,			// 8-bit per channel r,g,b,a (32bit color)
	FIELD_EMBEDDED,			// an embedded object with a datadesc, recursively traverse and embedded class/structure based on an additional typedescription
	FIELD_CUSTOM,			// special type that contains function pointers to it's read/write/parse functions

	FIELD_CLASSPTR,			// CBaseEntity*
	FIELD_EHANDLE,			// Entity handle
	FIELD_EDICT,			// edict_t

	FIELD_POSITION_VECTOR,	// A world coordinate (these are fixed up across level transitions automagically)
	FIELD_TIME,				// a floating point time (these are fixed up automatically too!)
	FIELD_TICK,				// an integer tick count( fixed up similarly to time)
	FIELD_MODELNAME,		// Engine string that is a model name (needs precache)
	FIELD_SOUNDNAME,		// Engine string that is a sound name (needs precache)

	FIELD_INPUT,			// a list of inputed data fields (all derived from CMultiInputVar)
	FIELD_FUNCTION,			// A class function pointer (Think, Use, etc)

	FIELD_VMATRIX,			// a vmatrix (output coords are NOT worldspace)

	// NOTE: Use float arrays for local transformations that don't need to be fixed up.
	FIELD_VMATRIX_WORLDSPACE,// A VMatrix that maps some local space to world space (translation is fixed up on level transitions)
	FIELD_MATRIX3X4_WORLDSPACE,	// matrix3x4_t that maps some local space to world space (translation is fixed up on level transitions)

	FIELD_INTERVAL,			// a start and range floating point interval ( e.g., 3.2->3.6 == 3.2 and 0.4 )
	FIELD_MODELINDEX,		// a model index
	FIELD_MATERIALINDEX,	// a material index (using the material precache string table)

	FIELD_VECTOR2D,			// 2 floats
	FIELD_INTEGER64,		// 64bit integer

	FIELD_VECTOR4D,			// 4 floats

	FIELD_TYPECOUNT,		// MUST BE LAST
} fieldtype_t;

struct datamap_t;
struct typedescription_t;

// [ PIXIE ] TODO: Verify this again, been a long time since I reversed this.
struct datamap_t
{
	typedescription_t* dataDesc;
	int unk1;
	int unk2;
	const char* dataClassName;
	uint64_t packed_size;
	int64_t dataNumFields;
	datamap_t* baseMap;
	// Verify size.
};

// [ PIXIE ] TODO: Verify this again, been a long time since I reversed this.
struct typedescription_t
{
	fieldtype_t fieldType;
	const char* fieldName;
	int fieldOffset;
	unsigned short fieldSize;
	short flags;
	uint64_t unk2[5];
	datamap_t* td;
	// Not full size yet.
};

namespace DataMapHandler
{
	int FindOffsetForField(datamap_t* map, const string& fieldName);
	typedescription_t* FindFieldInDataMap(datamap_t* map, const string& fieldName);
}

#define DATAMAP_VAR(type, name, datamap, varname) \
	type& name() { \
		static int _##name = NetVarManager::FindOffsetForField(datamap, varname); \
		return *(type*)((std::uintptr_t)this + _##name); \
	}