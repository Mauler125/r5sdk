// File: polar_codes.cpp
// See Copyright Notice and license at the end of include/lzham.h
//
// Andrew Polar's prefix code algorithm: 
// http://ezcodesample.com/prefixer/prefixer_article.html
//
// Also implements Fyffe's approximate codelength generation method, which is
// very similar but operates directly on codelengths vs. symbol frequencies:
// Fyffe Codes for Fast Codelength Approximation, Graham Fyffe, 1999
// http://code.google.com/p/lzham/wiki/FyffeCodes
#include "include/lzham_core.h"
#include "include/lzham_polar_codes.h"

#define LZHAM_USE_SHANNON_FANO_CODES 0
#define LZHAM_USE_FYFFE_CODES 0

namespace lzham
{
   struct sym_freq
   {
      uint16 m_freq;
      uint16 m_sym;
   };

   static inline sym_freq* radix_sort_syms(uint num_syms, sym_freq* syms0, sym_freq* syms1)
   {  
      const uint cMaxPasses = 2;
      uint hist[256 * cMaxPasses];

      memset(hist, 0, sizeof(hist[0]) * 256 * cMaxPasses);
      
      {
         sym_freq* p = syms0;
         sym_freq* q = syms0 + (num_syms >> 1) * 2;

         for ( ; p != q; p += 2)
         {
            const uint freq0 = p[0].m_freq;
            const uint freq1 = p[1].m_freq;

            hist[        freq0         & 0xFF]++;
            hist[256 + ((freq0 >>  8) & 0xFF)]++;

            hist[        freq1        & 0xFF]++;
            hist[256 + ((freq1 >>  8) & 0xFF)]++;
         }

         if (num_syms & 1)
         {
            const uint freq = p->m_freq;

            hist[        freq        & 0xFF]++;
            hist[256 + ((freq >>  8) & 0xFF)]++;
         }
      }

      sym_freq* pCur_syms = syms0;
      sym_freq* pNew_syms = syms1;
      
      const uint total_passes = (hist[256] == num_syms) ? 1 : cMaxPasses;
               
      for (uint pass = 0; pass < total_passes; pass++)
      {
         const uint* pHist = &hist[pass << 8];

         uint offsets[256];

         uint cur_ofs = 0;
         for (uint i = 0; i < 256; i += 2)
         {
            offsets[i] = cur_ofs;
            cur_ofs += pHist[i];

            offsets[i+1] = cur_ofs;
            cur_ofs += pHist[i+1];
         }

         const uint pass_shift = pass << 3;

         sym_freq* p = pCur_syms;
         sym_freq* q = pCur_syms + (num_syms >> 1) * 2;

         for ( ; p != q; p += 2)
         {
            uint c0 = p[0].m_freq;
            uint c1 = p[1].m_freq;

            if (pass)
            {
               c0 >>= 8;
               c1 >>= 8;
            }

            c0 &= 0xFF;
            c1 &= 0xFF;

            // Cut down on LHS's on console platforms by processing two at a time.
            if (c0 == c1)
            {
               uint dst_offset0 = offsets[c0];

               offsets[c0] = dst_offset0 + 2;

               pNew_syms[dst_offset0] = p[0];
               pNew_syms[dst_offset0 + 1] = p[1];
            }
            else
            {
               uint dst_offset0 = offsets[c0]++;
               uint dst_offset1 = offsets[c1]++;

               pNew_syms[dst_offset0] = p[0];
               pNew_syms[dst_offset1] = p[1];
            }
         }

         if (num_syms & 1)
         {
            uint c = ((p->m_freq) >> pass_shift) & 0xFF;

            uint dst_offset = offsets[c];
            offsets[c] = dst_offset + 1;

            pNew_syms[dst_offset] = *p;
         }

         sym_freq* t = pCur_syms;
         pCur_syms = pNew_syms;
         pNew_syms = t;
      }            

#if LZHAM_ASSERTS_ENABLED
      uint prev_freq = 0;
      for (uint i = 0; i < num_syms; i++)
      {
         LZHAM_ASSERT(!(pCur_syms[i].m_freq < prev_freq));
         prev_freq = pCur_syms[i].m_freq;
      }
#endif

      return pCur_syms;
   }

   struct polar_work_tables
   {
      sym_freq syms0[cPolarMaxSupportedSyms];
      sym_freq syms1[cPolarMaxSupportedSyms];
   };

   uint get_generate_polar_codes_table_size()
   {
      return sizeof(polar_work_tables);
   }
   
   void generate_polar_codes(uint num_syms, sym_freq* pSF, uint8* pCodesizes, uint& max_code_size_ret)
   {
      int tmp_freq[cPolarMaxSupportedSyms];

      uint orig_total_freq = 0;
      uint cur_total = 0;
      for (uint i = 0; i < num_syms; i++) 
      {
         uint sym_freq = pSF[num_syms - 1 - i].m_freq;
         orig_total_freq += sym_freq;

         uint sym_len = math::total_bits(sym_freq);
         uint adjusted_sym_freq = 1 << (sym_len - 1);
         tmp_freq[i] = adjusted_sym_freq;
         cur_total += adjusted_sym_freq;
      }

      uint tree_total = 1 << (math::total_bits(orig_total_freq) - 1);
      if (tree_total < orig_total_freq)
         tree_total <<= 1;

      uint start_index = 0;
      while ((cur_total < tree_total) && (start_index < num_syms))
      {
         for (uint i = start_index; i < num_syms; i++) 
         {
            uint freq = tmp_freq[i];
            if ((cur_total + freq) <= tree_total) 
            {
               tmp_freq[i] += freq;
               if ((cur_total += freq) == tree_total)
                  break;
            }
            else 
            {
               start_index = i + 1;
            }
         }
      }         

      LZHAM_ASSERT(cur_total == tree_total);

      uint max_code_size = 0;
      const uint tree_total_bits = math::total_bits(tree_total);
      for (uint i = 0; i < num_syms; i++) 
      {
         uint codesize = (tree_total_bits - math::total_bits(tmp_freq[i]));
         max_code_size = LZHAM_MAX(codesize, max_code_size);
         pCodesizes[pSF[num_syms-1-i].m_sym] = static_cast<uint8>(codesize);
      }
      max_code_size_ret = max_code_size;
   }

#if LZHAM_USE_FYFFE_CODES
   void generate_fyffe_codes(uint num_syms, sym_freq* pSF, uint8* pCodesizes, uint& max_code_size_ret)
   {
      int tmp_codesizes[cPolarMaxSupportedSyms];
      
      uint cur_total = 0;
      uint orig_total = 0;
      for (uint i = 0; i < num_syms; i++) 
      {
         uint sym_freq = pSF[i].m_freq;
         orig_total += sym_freq;
                  
         // Compute the nearest power of 2 lower or equal to the symbol's frequency.
         // This is equivalent to codesize=ceil(-log2(sym_prob)).
         uint floor_sym_freq = sym_freq;
         if (!math::is_power_of_2(floor_sym_freq))
         {
            uint sym_freq_bits = math::total_bits(sym_freq);
            floor_sym_freq = 1 << (sym_freq_bits - 1);
         }
         
         // Compute preliminary codesizes. tmp_freq's will always be <= the input frequencies.
         tmp_codesizes[i] = math::total_bits(floor_sym_freq);
         cur_total += floor_sym_freq;
      }

      // Desired_total is a power of 2, and will always be >= the adjusted frequency total.
      uint desired_total = cur_total;
      if (!math::is_power_of_2(desired_total))
         desired_total = math::next_pow2(desired_total);

      LZHAM_ASSERT(cur_total <= desired_total);
      
      // Compute residual and initial symbol codesizes.
      uint desired_total_bits = math::total_bits(desired_total);
      int r = desired_total;
      for (uint i = 0; i < num_syms; i++)
      {
         uint codesize = desired_total_bits - tmp_codesizes[i];
         tmp_codesizes[i] = static_cast<uint8>(codesize);
         r -= (desired_total >> codesize);
      }
      
      LZHAM_ASSERT(r >= 0);
            
      int sym_freq_scale = (desired_total << 7) / orig_total;
            
      // Promote codesizes from most probable to lowest, as needed.
      bool force_unhappiness = false;
      while (r > 0)
      {
         for (int i = num_syms - 1; i >= 0; i--)
         {
            uint codesize = tmp_codesizes[i];
            if (codesize == 1)
               continue;
               
            int sym_freq = pSF[i].m_freq;
            int f = desired_total >> codesize;
            if (f > r)
               continue;

            // A code is "unhappy" when it is assigned more bits than -log2(sym_prob).
            // It's too expensive to compute -log2(sym_freq/total_freq), so instead this directly compares the symbol's original 
            // frequency vs. the effective/adjusted frequency. sym_freq >= f is an approximation.
            //bool unhappy = force_unhappiness || (sym_freq >= f);
            
            // Compare the symbol's original probability vs. its effective probability at its current codelength.
            //bool unhappy = force_unhappiness || ((sym_freq * ((float)desired_total / orig_total)) > f);
            bool unhappy = force_unhappiness || ((sym_freq * sym_freq_scale) > (f << 7));

            if (unhappy)
            {
               tmp_codesizes[i]--;
               
               r -= f;
               if (r <= 0)
                  break;
            }
         }
         // Occasionally, a second pass is required to reduce the residual to 0. 
         // Subsequent passes ignore unhappiness. This is not discussed in Fyffe's original article.
         force_unhappiness = true;
      }
      
      LZHAM_ASSERT(!r);
      
      uint max_code_size = 0;
      
      for (uint i = 0; i < num_syms; i++) 
      {
         uint codesize = tmp_codesizes[i];
         max_code_size = LZHAM_MAX(codesize, max_code_size);
         pCodesizes[pSF[i].m_sym] = static_cast<uint8>(codesize);
      }
      max_code_size_ret = max_code_size;
   }
#endif //LZHAM_USE_FYFFE_CODES

#if LZHAM_USE_SHANNON_FANO_CODES   
   // Straightforward recursive Shannon-Fano implementation, for comparison purposes.
   static void generate_shannon_fano_codes_internal(uint num_syms, sym_freq* pSF, uint8* pCodesizes, int l, int h, uint total_freq)
   {
      LZHAM_ASSERT((h - l) >= 2);

      uint left_total = total_freq;
      uint right_total = 0;
      int best_diff = INT_MAX;
      int best_split_index = 0;
      for (int i = h - 1; i > l; i--)
      {
         uint freq = pSF[i].m_freq;
         uint next_left_total = left_total - freq;
         uint next_right_total = right_total + freq;
         LZHAM_ASSERT((next_left_total + next_right_total) == total_freq);
         
         int diff = labs(next_left_total - next_right_total);
         if (diff >= best_diff)
            break;
               
         left_total = next_left_total;
         right_total = next_right_total;
         best_split_index = i;
         best_diff = diff;
         if (!best_diff)
            break;
      }
                  
      for (int i = l; i < h; i++)
         pCodesizes[i]++;
                  
      if ((best_split_index - l) > 1) generate_shannon_fano_codes_internal(num_syms, pSF, pCodesizes, l, best_split_index, left_total);
      if ((h - best_split_index) > 1) generate_shannon_fano_codes_internal(num_syms, pSF, pCodesizes, best_split_index, h, right_total);
   }
   
   void generate_shannon_fano_codes(uint num_syms, sym_freq* pSF, uint total_freq, uint8* pCodesizes, uint& max_code_size_ret)
   {
      LZHAM_ASSERT(num_syms >= 2);
      uint8 tmp_codesizes[cPolarMaxSupportedSyms];
      memset(tmp_codesizes, 0, num_syms);
                  
      generate_shannon_fano_codes_internal(num_syms, pSF, tmp_codesizes, 0, num_syms, total_freq);
      
      uint max_code_size = 0;
      for (uint i = 0; i < num_syms; i++) 
      {
         uint codesize = tmp_codesizes[i];
         max_code_size = LZHAM_MAX(codesize, max_code_size);
         pCodesizes[pSF[i].m_sym] = static_cast<uint8>(codesize);
      }
      max_code_size_ret = max_code_size;
   }
#endif // LZHAM_USE_SHANNON_FANO_CODES

   bool generate_polar_codes(void* pContext, uint num_syms, const uint16* pFreq, uint8* pCodesizes, uint& max_code_size, uint& total_freq_ret)
   {
      if ((!num_syms) || (num_syms > cPolarMaxSupportedSyms))
         return false;

      polar_work_tables& state = *static_cast<polar_work_tables*>(pContext);

      uint max_freq = 0;
      uint total_freq = 0;

      uint num_used_syms = 0;
      for (uint i = 0; i < num_syms; i++)
      {
         uint freq = pFreq[i];

         if (!freq)
            pCodesizes[i] = 0;
         else
         {
            total_freq += freq;
            max_freq = math::maximum(max_freq, freq);

            sym_freq& sf = state.syms0[num_used_syms];
            sf.m_sym = static_cast<uint16>(i);
            sf.m_freq = static_cast<uint16>(freq);
            num_used_syms++;
         }            
      }

      total_freq_ret = total_freq;

      if (num_used_syms == 1)
      {
         pCodesizes[state.syms0[0].m_sym] = 1;
      }
      else
      {
         sym_freq* syms = radix_sort_syms(num_used_syms, state.syms0, state.syms1);
         
#if LZHAM_USE_SHANNON_FANO_CODES
         generate_shannon_fano_codes(num_syms, syms, total_freq, pCodesizes, max_code_size);
#elif LZHAM_USE_FYFFE_CODES         
         generate_fyffe_codes(num_syms, syms, pCodesizes, max_code_size);
#else
         generate_polar_codes(num_syms, syms, pCodesizes, max_code_size);
#endif         
      }

      return true;
   }

} // namespace lzham

