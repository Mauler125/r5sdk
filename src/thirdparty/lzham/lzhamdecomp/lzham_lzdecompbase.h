// File: lzham_lzdecompbase.h
// See Copyright Notice and license at the end of include/lzham.h
#pragma once

//#define LZHAM_LZDEBUG

#define LZHAM_IS_MATCH_MODEL_INDEX(prev_char, cur_state) ((prev_char) >> (8 - CLZDecompBase::cNumIsMatchContextBits)) + ((cur_state) << CLZDecompBase::cNumIsMatchContextBits)

#define LZHAM_USE_ALL_ARITHMETIC_CODING 0

#define LZHAM_RND_CONG(jcong)  (69069U * jcong + 1234567U)

namespace lzham
{
   struct CLZDecompBase
   {
      enum 
      {
         cMinMatchLen = 2U,
         cMaxMatchLen = 257U,
         
         cMaxHugeMatchLen = 65536,
                           
         cMinDictSizeLog2 = 15,
         cMaxDictSizeLog2 = 29,
                  
         cMatchHistSize = 4,
         cMaxLen2MatchDist = 2047
      };
         
      enum 
      {
         cLZXNumSecondaryLengths = 249,
         
         cNumHugeMatchCodes = 1,
         cMaxHugeMatchCodeBits = 16,
                                    
         cLZXNumSpecialLengths = 2,
         
         cLZXLowestUsableMatchSlot = 1,
         cLZXMaxPositionSlots = 128
      };
      
      enum
      {
         cLZXSpecialCodeEndOfBlockCode = 0,
         cLZXSpecialCodePartialStateReset = 1
      };
      
      enum
      {  
         cLZHAMDebugSyncMarkerValue = 666,
         cLZHAMDebugSyncMarkerBits = 12
      };

      enum
      {
         cBlockHeaderBits = 2,
         cBlockCheckBits = 4,
         cBlockFlushTypeBits = 2,
         
         cSyncBlock = 0,
         cCompBlock = 1,
         cRawBlock = 2,
         cEOFBlock = 3
      };
      
      enum
      {
         cNumStates = 12,
         cNumLitStates = 7,

         cNumLitPredBits = 6,          // must be even
         cNumDeltaLitPredBits = 6,     // must be even

         cNumIsMatchContextBits = 6
      };
      
      uint m_dict_size_log2;
      uint m_dict_size;
      
      uint m_num_lzx_slots;
      uint m_lzx_position_base[cLZXMaxPositionSlots];
      uint m_lzx_position_extra_mask[cLZXMaxPositionSlots];
      uint8 m_lzx_position_extra_bits[cLZXMaxPositionSlots];
      
      void init_position_slots(uint dict_size_log2);
   };
   
} // namespace lzham
