//=============================================================================//
//
// Purpose: pak page allocation and alignment
//
//=============================================================================//
#include "rtech/ipakfile.h"
#include "pakstate.h"
#include "pakalloc.h"

//-----------------------------------------------------------------------------
// aligns the segment headers for each asset type
//-----------------------------------------------------------------------------
void Pak_AlignSegmentHeaders(PakFile_s* const pak, PakSegmentDescriptor_s* const desc)
{
    uint64_t headersSize = 0;
    uint8_t headerSegmentAlignment = static_cast<int8_t>(desc->segmentAlignmentForType[SF_HEAD]);

    for (uint8_t i = 0; i < PAK_MAX_TRACKED_TYPES; ++i)
    {
        const PakAssetBinding_s& binding = g_pakGlobals->assetBindings[i];

        if (desc->assetTypeCount[i])
        {
            // asset header alignment really shouldn't be above 255
            // if this needs raising, headerSegmentAlignment should be made wider
            assert(binding.headerAlignment <= UINT8_MAX);

            const size_t alignedSize = ALIGN_VALUE(headersSize, static_cast<size_t>(binding.headerAlignment));

            pak->memoryData.unkAssetTypeBindingSizes[i] = alignedSize;
            headersSize = alignedSize + (desc->assetTypeCount[i] * binding.nativeClassSize);

            desc->segmentSizeForType[SF_HEAD] = headersSize;

            headerSegmentAlignment = Max(headerSegmentAlignment, static_cast<uint8_t>(binding.headerAlignment));
            desc->segmentAlignmentForType[SF_HEAD] = headerSegmentAlignment;
        }
    }
}

//-----------------------------------------------------------------------------
// aligns each individual non-header segment
//-----------------------------------------------------------------------------
void Pak_AlignSegments(PakFile_s* const pak, PakSegmentDescriptor_s* const desc)
{
    for (uint16_t i = 0; i < pak->GetSegmentCount(); ++i)
    {
        const PakSegmentHeader_s* const segHeader = pak->GetSegmentHeader(i);

        const uint8_t segmentType = segHeader->typeFlags & (SF_TEMP | SF_CPU);

        if (segmentType != SF_HEAD) // if not a header segment
        {
            // should this be a hard error on release?
            // segment alignment must not be 0 and must be a power of two
            assert(segHeader->dataAlignment > 0 && IsPowerOfTwo(segHeader->dataAlignment));

            const size_t alignedSegmentSize = ALIGN_VALUE(desc->segmentSizeForType[segmentType], static_cast<size_t>(segHeader->dataAlignment));
            //const size_t sizeAligned = ~(m_align - 1) & (m_align - 1 + segmentSizeForType[segmentType]);

            desc->segmentSizes[i] = alignedSegmentSize;
            desc->segmentSizeForType[segmentType] = alignedSegmentSize + segHeader->dataSize;

            // check if this segment's alignment is higher than the previous highest for this type
            // if so, increase the alignment to accommodate this segment
            desc->segmentAlignmentForType[segmentType] = Max(desc->segmentAlignmentForType[segmentType], segHeader->dataAlignment);
        }
    }
}

//-----------------------------------------------------------------------------
// copy's pages into pre-allocated and aligned segments
//-----------------------------------------------------------------------------
void Pak_CopyPagesToSegments(PakFile_s* const pak, PakLoadedInfo_s* const loadedInfo, PakSegmentDescriptor_s* const desc)
{
    for (uint32_t i = 0; i < pak->GetPageCount(); ++i)
    {
        const PakPageHeader_s* const pageHeader = pak->GetPageHeader(i);
        const uint32_t segmentIndex = pageHeader->segmentIdx;

        const PakSegmentHeader_s* const segHeader = pak->GetSegmentHeader(segmentIndex);
        const int typeFlags = segHeader->typeFlags;

        // check if header page
        if ((typeFlags & (SF_TEMP | SF_CPU)) != 0)
        {
            // align the segment's current size to the alignment of the new page to get copied in
            // this ensures that the location holding the page is aligned as required
            // 
            // since the segment will always have alignment equal to or greater than the page, and that alignment will always be a power of 2
            // the page does not have to be aligned to the same alignment as the segment, as aligning it to its own alignment is sufficient as long as
            // every subsequent page does the same thing
            const size_t alignedSegmentSize = ALIGN_VALUE(desc->segmentSizes[segmentIndex], static_cast<size_t>(pageHeader->pageAlignment));

            // get a pointer to the newly aligned location within the segment for this page
            pak->memoryData.memPageBuffers[i] = reinterpret_cast<uint8_t*>(loadedInfo->segmentBuffers[typeFlags & (SF_TEMP | SF_CPU)]) + alignedSegmentSize;

            // update the segment size to reflect the new alignment and page size
            desc->segmentSizes[segmentIndex] = alignedSegmentSize + pak->memoryData.pageHeaders[i].dataSize;
        }
        else
        {
            // all headers go into one segment and are dealt with separately in Pak_ProcessPakFile
            // since headers must be copied individually into a buffer that is big enough for the "native class" version of the header
            // instead of just the file version
            pak->memoryData.memPageBuffers[i] = reinterpret_cast<uint8_t*>(loadedInfo->segmentBuffers[SF_HEAD]);
        }
    }
}
