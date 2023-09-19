// File: lzham_polar_codes.h
// See Copyright Notice and license at the end of include/lzham.h
#pragma once

namespace lzham
{
   //const uint cPolarMaxSupportedSyms = 600;
   const uint cPolarMaxSupportedSyms = 1024;

   uint get_generate_polar_codes_table_size();

   bool generate_polar_codes(void* pContext, uint num_syms, const uint16* pFreq, uint8* pCodesizes, uint& max_code_size, uint& total_freq_ret);

} // namespace lzham
