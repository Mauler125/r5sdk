#pragma once

// Usually an enum, need to re-check if it's still standard.
typedef int64_t fieldtype_t;

struct datamap_t;
struct typedescription_t;

// [ PIXIE ] TODO: Verify this again, been a long time since I reversed this.
struct datamap_t
{
	typedescription_t* dataDesc;
	int dataNumFields;
	int unk1_;
	const char* dataClassName;
	uint64_t packed_size;
	int64_t unk2;
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