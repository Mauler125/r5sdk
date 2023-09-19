#ifndef IMDLCACHE_H
#define IMDLCACHE_H

#define ERROR_MODEL		"mdl/error.rmdl"
#define EMPTY_MODEL		"mdl/dev/empty_model.rmdl"
#define IDSTUDIOHEADER	(('T'<<24)+('S'<<16)+('D'<<8)+'I')

//-----------------------------------------------------------------------------
// Reference to a loaded studiomdl 
//-----------------------------------------------------------------------------
typedef unsigned short MDLHandle_t;

inline MDLHandle_t VoidPtrToMDLHandle(void* ptr)
{
	return (MDLHandle_t)(int)(intptr_t)ptr;
}

inline void* MDLHandleToVirtual(MDLHandle_t hndl)
{
	return (void*)(uintptr_t)hndl;
}

enum
{
	MDLHANDLE_INVALID = (MDLHandle_t)~0
};

//-----------------------------------------------------------------------------
// Cache data types
//-----------------------------------------------------------------------------
enum MDLCacheDataType_t
{
	MDLCACHE_NONE = -1,

	// Callbacks to get called when data is loaded or unloaded for these:
	MDLCACHE_STUDIOHDR = 0,
	MDLCACHE_STUDIOHWDATA,
	MDLCACHE_VCOLLIDE,

	// Callbacks NOT called when data is loaded or unloaded for these:
	MDLCACHE_ANIMBLOCK,
	MDLCACHE_VIRTUALMODEL,
	MDLCACHE_VERTEXES,
	MDLCACHE_DECODEDANIMBLOCK
};

abstract_class IMDLCache : public IAppSystem
{
	// !TODO: map this out.
};


#endif // !IMDLCACHE_H
