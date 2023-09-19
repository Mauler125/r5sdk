// File: lzham_lzdecompbase.cpp
// See Copyright Notice and license at the end of include/lzham.h
#include "../include/lzham_core.h"
#include "lzham_lzdecompbase.h"

namespace lzham
{
   void CLZDecompBase::init_position_slots(uint dict_size_log2)
   {
      m_dict_size_log2 = dict_size_log2;
      m_dict_size = 1U << dict_size_log2;
      
      int i, j;
      for (i = 0, j = 0; i < cLZXMaxPositionSlots; i += 2) 
      {
         m_lzx_position_extra_bits[i] = (uint8)j;
         m_lzx_position_extra_bits[i + 1] = (uint8)j; 

         if ((i != 0) && (j < 25))  
            j++; 
      }

      for (i = 0, j = 0; i < cLZXMaxPositionSlots; i++) 
      {
         m_lzx_position_base[i] = j;
         m_lzx_position_extra_mask[i] = (1 << m_lzx_position_extra_bits[i]) - 1;
         j += (1 << m_lzx_position_extra_bits[i]);
      }

      m_num_lzx_slots = 0;         
      
      const uint largest_dist = m_dict_size - 1;
      for (i = 0; i < cLZXMaxPositionSlots; i++)
      {
         if ( (largest_dist >= m_lzx_position_base[i]) &&
              (largest_dist < (m_lzx_position_base[i] + (1 << m_lzx_position_extra_bits[i])) ) )
         {
            m_num_lzx_slots = i + 1;
            break;
         }              
      }
      
      LZHAM_VERIFY(m_num_lzx_slots);
   }

} //namespace lzham
