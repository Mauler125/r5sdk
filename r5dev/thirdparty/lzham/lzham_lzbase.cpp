// File: lzham_lzbase.cpp
// See Copyright Notice and license at the end of include/lzham.h
#include "include/lzham_core.h"
#include "include/lzham_lzbase.h"

namespace lzham
{
   void CLZBase::init_slot_tabs()
   {
      for (uint i = 0; i < m_num_lzx_slots; i++)
      {
         //printf("%u: 0x%08X - 0x%08X, %u\n", i, m_lzx_position_base[i], m_lzx_position_base[i] + (1 << m_lzx_position_extra_bits[i]) - 1, m_lzx_position_extra_bits[i]);

         uint lo = m_lzx_position_base[i];
         uint hi = lo + m_lzx_position_extra_mask[i];

         uint8* pTab;
         uint shift;
         uint n; LZHAM_NOTE_UNUSED(n);

         if (hi < 0x1000)
         {
            pTab = m_slot_tab0;
            shift = 0;
            n = sizeof(m_slot_tab0);
         }
         else if (hi < 0x100000)
         {
            pTab = m_slot_tab1;
            shift = 11;
            n = sizeof(m_slot_tab1);
         }
         else if (hi < 0x1000000)
         {
            pTab = m_slot_tab2;
            shift = 16;
            n = sizeof(m_slot_tab2);
         }
         else
            break;

         lo >>= shift;
         hi >>= shift;

         LZHAM_ASSERT(hi < n);
         memset(pTab + lo, (uint8)i, hi - lo + 1);
      }

#ifdef LZHAM_BUILD_DEBUG      
      uint slot, ofs;
      for (uint i = 1; i < m_num_lzx_slots; i++) 
      {
         compute_lzx_position_slot(m_lzx_position_base[i], slot, ofs);
         LZHAM_ASSERT(slot == i);

         compute_lzx_position_slot(m_lzx_position_base[i] + m_lzx_position_extra_mask[i], slot, ofs);
         LZHAM_ASSERT(slot == i);
      }

      for (uint i = 1; i <= (m_dict_size-1); i += 512U*1024U)
      {
         compute_lzx_position_slot(i, slot, ofs);
         LZHAM_ASSERT(i == m_lzx_position_base[slot] + ofs);
      }

      compute_lzx_position_slot(m_dict_size - 1, slot, ofs);
      LZHAM_ASSERT((m_dict_size - 1) == m_lzx_position_base[slot] + ofs);
#endif      
   }
} //namespace lzham

