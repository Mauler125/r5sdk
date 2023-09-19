#ifndef IDATACACHE_H
#define IDATACACHE_H

//---------------------------------------------------------
// Cache-defined handle for a cache item
//---------------------------------------------------------
FORWARD_DECLARE_HANDLE(memhandle_t);
typedef memhandle_t DataCacheHandle_t;
#define DC_INVALID_HANDLE ((DataCacheHandle_t)0xDEADFEEDDEADFEED)

#endif // IDATACACHE_H
