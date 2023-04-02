#include "core/stdafx.h"
#include "tier1/cvar.h"
#include "rtech/rtech_utils.h"
#ifndef DEDICATED
#include "windows/id3dx.h"
#include "materialsystem/cshaderglue.h"
#include "public/rendersystem/schema/texture.g.h"
#endif // !DEDICATED

/******************************************************************************
-------------------------------------------------------------------------------
File   : rtech.cpp
Date   : 18:07:2021
Author : Kawe Mazidjatari
Purpose: Implements the 'rtech_game' core utilities
-------------------------------------------------------------------------------
History:
- 18:07:2021 | 13:02 : Created by Kawe Mazidjatari
- 10:09:2021 | 18:22 : Implement 'StringToGuid' method
- 12:11:2021 | 14:41 : Add decompression method to ConCommand callback
- 25:12:2021 | 23:20 : Made everything more readable thanks to bezdna5-rs
- 28:03:2022 | 18:00 : Added getting pak info by PakID.

******************************************************************************/

//-----------------------------------------------------------------------------
// Purpose: calculate 'GUID' from input data
//-----------------------------------------------------------------------------
uint64_t __fastcall RTech::StringToGuid(const char* pData)
{
	uint32_t*        v1; // r8
	uint64_t         v2; // r10
	int32_t          v3; // er11
	uint32_t         v4; // er9
	uint32_t          i; // edx
	uint64_t         v6; // rcx
	int32_t          v7; // er9
	int32_t          v8; // edx
	int32_t          v9; // eax
	uint32_t        v10; // er8
	int32_t         v12; // ecx
	uint32_t* a1 = (uint32_t*)pData;

	v1 = a1;
	v2 = 0i64;
	v3 = 0;
	v4 = (*a1 - 45 * ((~(*a1 ^ 0x5C5C5C5Cu) >> 7) & (((*a1 ^ 0x5C5C5C5Cu) - 0x1010101) >> 7) & 0x1010101)) & 0xDFDFDFDF;
	for (i = ~*a1 & (*a1 - 0x1010101) & 0x80808080; !i; i = v8 & 0x80808080)
	{
		v6 = v4;
		v7 = v1[1];
		++v1;
		v3 += 4;
		v2 = ((((uint64_t)(0xFB8C4D96501i64 * v6) >> 24) + 0x633D5F1 * v2) >> 61) ^ (((uint64_t)(0xFB8C4D96501i64 * v6) >> 24)
			+ 0x633D5F1 * v2);
		v8 = ~v7 & (v7 - 0x1010101);
		v4 = (v7 - 45 * ((~(v7 ^ 0x5C5C5C5Cu) >> 7) & (((v7 ^ 0x5C5C5C5Cu) - 0x1010101) >> 7) & 0x1010101)) & 0xDFDFDFDF;
	}
	v9 = -1;
	v10 = (i & -(signed)i) - 1;
	if (_BitScanReverse((unsigned long*)&v12, v10))
	{
		v9 = v12;
	}
	return 0x633D5F1 * v2 + ((0xFB8C4D96501i64 * (uint64_t)(v4 & v10)) >> 24) - 0xAE502812AA7333i64 * (uint32_t)(v3 + v9 / 8);
}

//-----------------------------------------------------------------------------
// Purpose: calculate 'decompressed' size and commit parameters
//-----------------------------------------------------------------------------
uint64_t __fastcall RTech::DecompressPakFileInit(RPakDecompState_t* state, uint8_t* fileBuffer, uint64_t fileSize, uint64_t offNoHeader, uint64_t headerSize)
{
	int64_t input_byte_pos_init;         // r9
	uint64_t byte_init;                  // r11
	int32_t decompressed_size_bits;      // ecx
	int64_t byte_1_low;                  // rdi
	uint64_t input_byte_pos_1;           // r10
	uint32_t bit_pos_final;              // ebp
	uint64_t byte_1;                     // rdi
	uint32_t brih_bits;                  // er11
	uint64_t inv_mask_in;                // r8
	uint64_t byte_final_full;            // rbx
	uint64_t bit_pos_final_1;            // rax
	int32_t byte_bit_offset_final;       // ebp
	uint64_t input_byte_pos_final;       // r10
	uint64_t byte_final;                 // rbx
	uint32_t brih_bytes;                 // er11
	uint64_t byte_tmp;                   // rdx
	uint64_t stream_len_needed;          // r14
	uint64_t result;                     // rax
	uint64_t inv_mask_out;               // r8
	uint64_t qw70;                       // rcx
	uint64_t stream_compressed_size_new; // rdx

	const uintptr_t mask = UINT64_MAX;
	const uintptr_t file_buf = uintptr_t(fileBuffer);

	state->m_nInputBuf = file_buf;
	state->m_nOut = 0i64;
	state->m_nOutMask = 0i64;
	state->dword44 = 0;
	state->m_nTotalFileLen = fileSize + offNoHeader;
	state->m_nMask = mask;
	input_byte_pos_init = offNoHeader + headerSize + 8;
	byte_init = *(uint64_t*)((mask & (offNoHeader + headerSize)) + file_buf);
	state->m_nDecompPosition = headerSize;
	decompressed_size_bits = byte_init & 0x3F;
	byte_init >>= 6;
	state->m_nInputBytePos = input_byte_pos_init;
	state->m_nDecompSize = byte_init & ((1i64 << decompressed_size_bits) - 1) | (1i64 << decompressed_size_bits);
	byte_1_low = *(uint64_t*)((mask & input_byte_pos_init) + file_buf) << (64
		- ((uint8_t)decompressed_size_bits
			+ 6));
	input_byte_pos_1 = input_byte_pos_init + ((uint64_t)(uint32_t)(decompressed_size_bits + 6) >> 3);
	state->m_nInputBytePos = input_byte_pos_1;
	bit_pos_final = ((decompressed_size_bits + 6) & 7) + 13;
	byte_1 = (0xFFFFFFFFFFFFFFFFui64 >> ((decompressed_size_bits + 6) & 7)) & ((byte_init >> decompressed_size_bits) | byte_1_low);
	brih_bits = (((uint8_t)byte_1 - 1) & 0x3F) + 1;
	inv_mask_in = 0xFFFFFFFFFFFFFFFFui64 >> (64 - (uint8_t)brih_bits);
	state->m_nInvMaskIn = inv_mask_in;
	state->m_nInvMaskOut = 0xFFFFFFFFFFFFFFFFui64 >> (63 - (((byte_1 >> 6) - 1) & 0x3F));
	byte_final_full = (byte_1 >> 13) | (*(uint64_t*)((mask & input_byte_pos_1) + file_buf) << (64
		- (uint8_t)bit_pos_final));
	bit_pos_final_1 = bit_pos_final;
	byte_bit_offset_final = bit_pos_final & 7;
	input_byte_pos_final = (bit_pos_final_1 >> 3) + input_byte_pos_1;
	byte_final = (0xFFFFFFFFFFFFFFFFui64 >> byte_bit_offset_final) & byte_final_full;
	state->m_nInputBytePos = input_byte_pos_final;
	if (inv_mask_in == -1i64)
	{
		state->m_nHeaderOffset = 0;
		stream_len_needed = fileSize;
	}
	else
	{
		brih_bytes = brih_bits >> 3;
		state->m_nHeaderOffset = brih_bytes + 1;
		byte_tmp = *(uint64_t*)((mask & input_byte_pos_final) + file_buf);
		state->m_nInputBytePos = input_byte_pos_final + brih_bytes + 1;
		stream_len_needed = byte_tmp & ((1i64 << (8 * ((uint8_t)brih_bytes + 1))) - 1);
	}
	result = state->m_nDecompSize;
	inv_mask_out = state->m_nInvMaskOut;
	qw70 = offNoHeader + state->m_nInvMaskIn - 6i64;
	state->m_nLengthNeeded = stream_len_needed + offNoHeader;
	state->qword70 = qw70;
	state->byte = byte_final;
	state->m_nByteBitOffset = byte_bit_offset_final;
	state->dword6C = 0;
	state->m_nCompressedStreamSize = stream_len_needed + offNoHeader;
	state->m_nDecompStreamSize = result;
	if (result - 1 > inv_mask_out)
	{
		stream_compressed_size_new = stream_len_needed + offNoHeader - state->m_nHeaderOffset;
		state->m_nDecompStreamSize = inv_mask_out + 1;
		state->m_nCompressedStreamSize = stream_compressed_size_new;
	}

	return result;
}

//-----------------------------------------------------------------------------
// Purpose: decompress input data
//-----------------------------------------------------------------------------
uint8_t __fastcall RTech::DecompressPakFile(RPakDecompState_t* state, uint64_t inLen, uint64_t outLen)
{
	char result;                          // al
	uint64_t v5;                          // r15
	uint64_t v6;                          // r11
	uint32_t v7;                          // ebp
	uint64_t v8;                          // rsi
	uint64_t v9;                          // rdi
	uint64_t v10;                         // r12
	uint64_t v11;                         // r13
	uint32_t v12;                         // ecx
	uint64_t v13;                         // rsi
	uint64_t i;                           // rax
	uint64_t v15;                         // r8
	int64_t v16;                          // r9
	int v17;                              // ecx
	uint64_t v18;                         // rax
	uint64_t v19;                         // rsi
	int64_t v20;                          // r14
	int v21;                              // ecx
	uint64_t v22;                         // r11
	int v23;                              // edx
	uint64_t v24;                         // rax
	int v25;                              // er8
	uint32_t v26;                         // er13
	uint64_t v27;                         // r10
	uint64_t v28;                         // rax
	_QWORD* v29;                          // r10
	uint64_t v30;                         // r9
	uint64_t v31;                         // r10
	uint64_t v32;                         // r8
	uint64_t v33;                         // rax
	uint64_t v34;                         // rax
	uint64_t v35;                         // rax
	uint64_t v36;                         // rcx
	int64_t v37;                          // rdx
	uint64_t v38;                         // r14
	uint64_t v39;                         // r11
	char v40;                             // cl
	uint64_t v41;                         // rsi
	int64_t v42;                          // rcx
	uint64_t v43;                         // r8
	int v44;                              // er11
	uint8_t v45;                          // r9
	uint64_t v46;                         // rcx
	uint64_t v47;                         // rcx
	int64_t v48;                          // r9
	int64_t l;                            // r8
	uint32_t v50;                         // er9
	int64_t v51;                          // r8
	int64_t v52;                          // rdx
	int64_t k;                            // r8
	char* v54;                            // r10
	int64_t v55;                          // rdx
	uint32_t v56;                         // er14
	int64_t* v57;                         // rdx
	int64_t* v58;                         // r8
	char v59;                             // al
	uint64_t v60;                         // rsi
	int64_t v61;                          // rax
	uint64_t v62;                         // r9
	int v63;                              // er10
	uint8_t v64;                          // cl
	uint64_t v65;                         // rax
	uint32_t v66;                         // er14
	uint32_t j;                           // ecx
	int64_t v68;                          // rax
	uint64_t v69;                         // rcx
	uint64_t v70;                         // [rsp+0h] [rbp-58h]
	uint32_t v71;                         // [rsp+60h] [rbp+8h]
	uint64_t v74;                         // [rsp+78h] [rbp+20h]

	if (inLen < state->m_nLengthNeeded)
		return 0;
	v5 = state->m_nDecompPosition;
	if (outLen < state->m_nInvMaskOut + (v5 & ~state->m_nInvMaskOut) + 1 && outLen < state->m_nDecompSize)
		return 0;
	v6 = state->m_nOut;
	v7 = state->m_nByteBitOffset;
	v8 = state->byte;
	v9 = state->m_nInputBytePos;
	v10 = state->qword70;
	v11 = state->m_nInputBuf;
	if (state->m_nCompressedStreamSize < v10)
		v10 = state->m_nCompressedStreamSize;
	v12 = state->dword6C;
	v74 = v11;
	v70 = v6;
	v71 = v12;
	if (!v7)
		goto LABEL_11;
	v13 = (*(_QWORD*)((v9 & state->m_nMask) + v11) << (64 - (unsigned __int8)v7)) | v8;
	for (i = v7; ; i = v7)
	{
		v7 &= 7u;
		v9 += i >> 3;
		v12 = v71;
		v8 = (0xFFFFFFFFFFFFFFFFui64 >> v7) & v13;
	LABEL_11:
		v15 = (unsigned __int64)v12 << 8;
		v16 = v12;
		v17 = *((unsigned __int8*)&s_PakFileCompressionLUT + (unsigned __int8)v8 + v15 + 512);
		v18 = (unsigned __int8)v8 + v15;
		v7 += v17;
		v19 = v8 >> v17;
		v20 = (unsigned int)*((char*)&s_PakFileCompressionLUT + v18);
		if (*((char*)&s_PakFileCompressionLUT + v18) < 0)
		{
			v56 = -(int)v20;
			v57 = (__int64*)(v11 + (v9 & state->m_nMask));
			v71 = 1;
			v58 = (__int64*)(v6 + (v5 & state->m_nOutMask));
			if (v56 == *((unsigned __int8*)&s_PakFileCompressionLUT + v16 + 1248))
			{
				if ((~v9 & state->m_nInvMaskIn) < 0xF || (state->m_nInvMaskOut & ~v5) < 15 || state->m_nDecompSize - v5 < 0x10)
					v56 = 1;
				v59 = char(v19);
				v60 = v19 >> 3;
				v61 = v59 & 7;
				v62 = v60;
				if (v61)
				{
					v63 = *((unsigned __int8*)&s_PakFileCompressionLUT + v61 + 1232);
					v64 = *((_BYTE*)&s_PakFileCompressionLUT + v61 + 1240);
				}
				else
				{
					v62 = v60 >> 4;
					v65 = v60 & 0xF;
					v7 += 4;
					v63 = *((_DWORD*)&s_PakFileCompressionLUT + v65 + 288);
					v64 = *((_BYTE*)&s_PakFileCompressionLUT + v65 + 1216);
				}
				v7 += v64 + 3;
				v19 = v62 >> v64;
				v66 = v63 + (v62 & ((1 << v64) - 1)) + v56;
				for (j = v66 >> 3; j; --j)
				{
					v68 = *v57++;
					*v58++ = v68;
				}
				if ((v66 & 4) != 0)
				{
					*(_DWORD*)v58 = *(_DWORD*)v57;
					v58 = (__int64*)((char*)v58 + 4);
					v57 = (__int64*)((char*)v57 + 4);
				}
				if ((v66 & 2) != 0)
				{
					*(_WORD*)v58 = *(_WORD*)v57;
					v58 = (__int64*)((char*)v58 + 2);
					v57 = (__int64*)((char*)v57 + 2);
				}
				if ((v66 & 1) != 0)
					*(_BYTE*)v58 = *(_BYTE*)v57;
				v9 += v66;
				v5 += v66;
			}
			else
			{
				*v58 = *v57;
				v58[1] = v57[1];
				v9 += v56;
				v5 += v56;
			}
		}
		else
		{
			v21 = v19 & 0xF;
			v71 = 0;
			v22 = ((unsigned __int64)(unsigned int)v19 >> (((unsigned int)(v21 - 31) >> 3) & 6)) & 0x3F;
			v23 = 1 << (v21 + ((v19 >> 4) & ((24 * (((unsigned int)(v21 - 31) >> 3) & 2)) >> 4)));
			v7 += (((unsigned int)(v21 - 31) >> 3) & 6) + *((unsigned __int8*)&s_PakFileCompressionLUT + v22 + 1088) + v21 + ((v19 >> 4) & ((24 * (((unsigned int)(v21 - 31) >> 3) & 2)) >> 4));
			v24 = state->m_nOutMask;
			v25 = 16 * (v23 + ((v23 - 1) & (v19 >> ((((unsigned int)(v21 - 31) >> 3) & 6) + *((_BYTE*)&s_PakFileCompressionLUT + v22 + 1088)))));
			v19 >>= (((unsigned int)(v21 - 31) >> 3) & 6) + *((_BYTE*)&s_PakFileCompressionLUT + v22 + 1088) + v21 + ((v19 >> 4) & ((24 * (((unsigned int)(v21 - 31) >> 3) & 2)) >> 4));
			v26 = v25 + *((unsigned __int8*)&s_PakFileCompressionLUT + v22 + 1024) - 16;
			v27 = v24 & (v5 - v26);
			v28 = v70 + (v5 & v24);
			v29 = (_QWORD*)(v70 + v27);
			if ((_DWORD)v20 == 17)
			{
				v40 = char(v19);
				v41 = v19 >> 3;
				v42 = v40 & 7;
				v43 = v41;
				if (v42)
				{
					v44 = *((unsigned __int8*)&s_PakFileCompressionLUT + v42 + 1232);
					v45 = *((_BYTE*)&s_PakFileCompressionLUT + v42 + 1240);
				}
				else
				{
					v7 += 4;
					v46 = v41 & 0xF;
					v43 = v41 >> 4;
					v44 = *((_DWORD*)&s_PakFileCompressionLUT + v46 + 288);
					v45 = *((_BYTE*)&s_PakFileCompressionLUT + v46 + 1216);
					if (v74 && v7 + v45 >= 61)
					{
						v47 = v9++ & state->m_nMask;
						v43 |= (unsigned __int64)*(unsigned __int8*)(v47 + v74) << (61 - (unsigned __int8)v7);
						v7 -= 8;
					}
				}
				v7 += v45 + 3;
				v19 = v43 >> v45;
				v48 = ((unsigned int)v43 & ((1 << v45) - 1)) + v44 + 17;
				v5 += v48;
				if (v26 < 8)
				{
					v50 = uint32_t(v48 - 13);
					v5 -= 13i64;
					if (v26 == 1)
					{
						v51 = *(unsigned __int8*)v29;
						//++dword_14D40B2BC;
						v52 = 0i64;
						for (k = 0x101010101010101i64 * v51; (unsigned int)v52 < v50; v52 = (unsigned int)(v52 + 8))
							*(_QWORD*)(v52 + v28) = k;
					}
					else
					{
						//++dword_14D40B2B8;
						if (v50)
						{
							v54 = (char*)v29 - v28;
							v55 = v50;
							do
							{
								*(_BYTE*)v28 = v54[v28];
								++v28;
								--v55;
							} while (v55);
						}
					}
				}
				else
				{
					//++dword_14D40B2AC;
					for (l = 0i64; (unsigned int)l < (unsigned int)v48; l = (unsigned int)(l + 8))
						*(_QWORD*)(l + v28) = *(_QWORD*)((char*)v29 + l);
				}
			}
			else
			{
				v5 += v20;
				*(_QWORD*)v28 = *v29;
				*(_QWORD*)(v28 + 8) = v29[1];
			}
			v11 = v74;
		}
		if (v9 >= v10)
			break;
	LABEL_29:
		v6 = v70;
		v13 = (*(_QWORD*)((v9 & state->m_nMask) + v11) << (64 - (unsigned __int8)v7)) | v19;
	}
	if (v5 != state->m_nDecompStreamSize)
		goto LABEL_25;
	v30 = state->m_nDecompSize;
	if (v5 == v30)
	{
		result = 1;
		goto LABEL_69;
	}
	v31 = state->m_nInvMaskIn;
	v32 = state->m_nHeaderOffset;
	v33 = v31 & -(__int64)v9;
	v19 >>= 1;
	++v7;
	if (v32 > v33)
	{
		v9 += v33;
		v34 = state->qword70;
		if (v9 > v34)
			state->qword70 = v31 + v34 + 1;
	}
	v35 = v9 & state->m_nMask;
	v9 += v32;
	v36 = v5 + state->m_nInvMaskOut + 1;
	v37 = *(_QWORD*)(v35 + v11) & ((1i64 << (8 * (unsigned __int8)v32)) - 1);
	v38 = v37 + state->m_nLengthNeeded;
	v39 = v37 + state->m_nCompressedStreamSize;
	state->m_nLengthNeeded = v38;
	state->m_nCompressedStreamSize = v39;
	if (v36 >= v30)
	{
		v36 = v30;
		state->m_nCompressedStreamSize = v32 + v39;
	}
	state->m_nDecompStreamSize = v36;
	if (inLen >= v38 && outLen >= v36)
	{
	LABEL_25:
		v10 = state->qword70;
		if (v9 >= v10)
		{
			v9 = ~state->m_nInvMaskIn & (v9 + 7);
			v10 += state->m_nInvMaskIn + 1;
			state->qword70 = v10;
		}
		if (state->m_nCompressedStreamSize < v10)
			v10 = state->m_nCompressedStreamSize;
		goto LABEL_29;
	}
	v69 = state->qword70;
	if (v9 >= v69)
	{
		v9 = ~v31 & (v9 + 7);
		state->qword70 = v69 + v31 + 1;
	}
	state->dword6C = v71;
	result = 0;
	state->byte = v19;
	state->m_nByteBitOffset = v7;
LABEL_69:
	state->m_nDecompPosition = v5;
	state->m_nInputBytePos = v9;
	return result;
}

#if !defined(DEDICATED)

#pragma warning( push )
// Disable stack warning, tells us to move more data to the heap instead. Not really possible with 'initialData' here. Since its parallel processed.
// Also disable 6378, complains that there is no control path where it would use 'nullptr', if that happens 'Error' will be called though.
#pragma warning( disable : 6262 6387)
constexpr uint32_t ALIGNMENT_SIZE = 15; // Used by the game in CreateDXTexture.
//----------------------------------------------------------------------------------
// Purpose: creates 2D texture and shader resource from textureHeader and imageData.
//----------------------------------------------------------------------------------
void RTech::CreateDXTexture(TextureHeader_t* textureHeader, int64_t imageData)
{
	if (textureHeader->m_nDepth && !textureHeader->m_nHeight) // Return never gets hit. Maybe its some debug check?
		return;

	__int64 initialData[4096]{};
	textureHeader->m_nTextureMipLevels = textureHeader->m_nPermanentMipCount;

	const int totalStreamedMips = textureHeader->m_nOptStreamedMipCount + textureHeader->m_nStreamedMipCount;
	int mipLevel = textureHeader->m_nPermanentMipCount + totalStreamedMips;
	if (mipLevel != totalStreamedMips)
	{
		do
		{
			--mipLevel;
			if (textureHeader->m_nArraySize)
			{
				int mipWidth = 0;
				if (textureHeader->m_nWidth >> mipLevel > 1)
					mipWidth = (textureHeader->m_nWidth >> mipLevel) - 1;

				int mipHeight = 0;
				if (textureHeader->m_nHeight >> mipLevel > 1)
					mipHeight = (textureHeader->m_nHeight >> mipLevel) - 1;
	
				uint8_t x = s_pBytesPerPixel[textureHeader->m_nImageFormat].first;
				uint8_t y = s_pBytesPerPixel[textureHeader->m_nImageFormat].second;

				uint32_t bppWidth = (y + mipWidth) >> (y >> 1);
				uint32_t bppHeight = (y + mipHeight) >> (y >> 1);
				uint32_t sliceWidth = x * (y >> (y >> 1));

				uint32_t rowPitch = sliceWidth * bppWidth;
				uint32_t slicePitch = x * bppWidth * bppHeight;

				uint32_t subResourceEntry = mipLevel;
				for (int i = 0; i < textureHeader->m_nArraySize; i++)
				{
					uint32_t offsetCurrentResourceData = subResourceEntry << 4u;

					*(int64_t*)((uint8_t*)initialData + offsetCurrentResourceData) = imageData;
					*(uint32_t*)((uint8_t*)&initialData[1] + offsetCurrentResourceData) = rowPitch;
					*(uint32_t*)((uint8_t*)&initialData[1] + offsetCurrentResourceData + 4) = slicePitch;

					imageData += (slicePitch + ALIGNMENT_SIZE) & ~ALIGNMENT_SIZE;
					subResourceEntry += textureHeader->m_nPermanentMipCount;
				}
			}
		} while (mipLevel != totalStreamedMips);
	}

	const DXGI_FORMAT dxgiFormat = g_TxtrAssetToDxgiFormat[textureHeader->m_nImageFormat]; // Get dxgi format

	D3D11_TEXTURE2D_DESC textureDesc{};
	textureDesc.Width = textureHeader->m_nWidth >> mipLevel;
	textureDesc.Height = textureHeader->m_nHeight >> mipLevel;
	textureDesc.MipLevels = textureHeader->m_nPermanentMipCount;
	textureDesc.ArraySize = textureHeader->m_nArraySize;
	textureDesc.Format = dxgiFormat;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = textureHeader->m_nCPUAccessFlag != 2 ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	textureDesc.MiscFlags = 0;

	const uint32_t offsetStartResourceData = mipLevel << 4u;
	const D3D11_SUBRESOURCE_DATA* subResData = (D3D11_SUBRESOURCE_DATA*)((uint8_t*)initialData + offsetStartResourceData);
	const HRESULT createTextureRes = (*g_ppGameDevice)->CreateTexture2D(&textureDesc, subResData, &textureHeader->m_ppTexture);
	if (createTextureRes < S_OK)
		Error(eDLL_T::RTECH, EXIT_FAILURE, "Couldn't create texture \"%s\": error code = %08x\n", textureHeader->m_pDebugName, createTextureRes);

	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResource{};
	shaderResource.Format = dxgiFormat;
	shaderResource.Texture2D.MipLevels = textureHeader->m_nTextureMipLevels;
	if (textureHeader->m_nArraySize > 1) // Do we have a texture array?
	{
		shaderResource.Texture2DArray.FirstArraySlice = 0;
		shaderResource.Texture2DArray.ArraySize = textureHeader->m_nArraySize;
		shaderResource.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2DARRAY;
	}
	else
	{
		shaderResource.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
	}

	const HRESULT createShaderResourceRes = (*g_ppGameDevice)->CreateShaderResourceView(textureHeader->m_ppTexture, &shaderResource, &textureHeader->m_ppShaderResourceView);
	if (createShaderResourceRes < S_OK)
		Error(eDLL_T::RTECH, EXIT_FAILURE, "Couldn't create shader resource view for texture \"%s\": error code = %08x\n", textureHeader->m_pDebugName, createShaderResourceRes);
}
#pragma warning( pop )
#endif

#ifndef DEDICATED
//----------------------------------------------------------------------------------
// Purpose: start loading shader sets, assign vftable pointer
//----------------------------------------------------------------------------------
void** RTech::LoadShaderSet(void** VTablePtr)
{
	*VTablePtr = &g_pShaderGlueVFTable;
	return &g_pShaderGlueVFTable;
}
#endif

//----------------------------------------------------------------------------------
// Purpose: open a file and add it to m_FileHandles.
//----------------------------------------------------------------------------------
int32_t RTech::OpenFile(const CHAR* szFilePath, void* unused, LONGLONG* fileSizeOut)
{
	string svModFile = szFilePath;
	string svBaseFile = szFilePath;
	const string svModDir = "paks\\Win32\\";
	const string svBaseDir = "paks\\Win64\\";

	if (strstr(ConvertToWinPath(szFilePath).c_str(), svBaseDir.c_str()))
	{
		svBaseFile.erase(0, 11); // Erase 'base_dir'.
		svModFile = svModDir + svBaseFile; // Prepend 'mod_dir'.

		if (!FileExists(svModFile))
		{
			svModFile = szFilePath;
		}
	}

	const HANDLE hFile = CreateFileA(svModFile.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, 0, OPEN_EXISTING, FILE_SUPPORTS_GHOSTING, 0);
	if (hFile == INVALID_HANDLE_VALUE)
		return -1;

	if (rtech_debug->GetBool())
		DevMsg(eDLL_T::RTECH, "Opened file: '%s'\n", svModFile.c_str());

	if (fileSizeOut)
	{
		LARGE_INTEGER fileSize{};
		if (GetFileSizeEx(hFile, &fileSize))
			*fileSizeOut = fileSize.QuadPart;
	}

	AcquireSRWLockExclusive(reinterpret_cast<PSRWLOCK>(&*s_pFileArrayMutex));
	const int32_t fileIdx = RTech_FindFreeSlotInFiles(s_pFileArray);
	ReleaseSRWLockExclusive(reinterpret_cast<PSRWLOCK>(&*s_pFileArrayMutex));

	const int32_t fileHandleIdx =  (fileIdx & 0x3FF); // Something with ArraySize.

	s_pFileHandles->self[fileHandleIdx].m_nFileNumber = fileIdx;
	s_pFileHandles->self[fileHandleIdx].m_hFileHandle = hFile;
	s_pFileHandles->self[fileHandleIdx].m_nCurOfs = 1;

	return fileIdx;
}

//-----------------------------------------------------------------------------
// Purpose: gets information about loaded pak file via pak ID
//-----------------------------------------------------------------------------
RPakLoadedInfo_t* RTech::GetPakLoadedInfo(RPakHandle_t nHandle)
{
	for (int16_t i = 0; i < *g_pLoadedPakCount; ++i)
	{
		RPakLoadedInfo_t* info = &g_pLoadedPakInfo[i];
		if (!info)
			continue;

		if (info->m_nHandle != nHandle)
			continue;

		return info;
	}

	Warning(eDLL_T::RTECH, "%s - Failed to retrieve pak info for handle '%d'\n", __FUNCTION__, nHandle);
	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: gets information about loaded pak file via pak name
//-----------------------------------------------------------------------------
RPakLoadedInfo_t* RTech::GetPakLoadedInfo(const char* szPakName)
{
	for (int16_t i = 0; i < *g_pLoadedPakCount; ++i)
	{
		RPakLoadedInfo_t* info = &g_pLoadedPakInfo[i];
		if (!info)
			continue;

		if (!info->m_pszFileName || !*info->m_pszFileName)
			continue;

		if (strcmp(szPakName, info->m_pszFileName) != 0)
			continue;

		return info;
	}

	Warning(eDLL_T::RTECH, "%s - Failed to retrieve pak info for name '%s'\n", __FUNCTION__, szPakName);
	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: returns pak status as string
//-----------------------------------------------------------------------------
const char* RTech::PakStatusToString(RPakStatus_t status)
{
	switch (status)
	{
		case RPakStatus_t::PAK_STATUS_FREED:                  return "PAK_STATUS_FREED";
		case RPakStatus_t::PAK_STATUS_LOAD_PENDING:           return "PAK_STATUS_LOAD_PENDING";
		case RPakStatus_t::PAK_STATUS_REPAK_RUNNING:          return "PAK_STATUS_REPAK_RUNNING";
		case RPakStatus_t::PAK_STATUS_REPAK_DONE:             return "PAK_STATUS_REPAK_DONE";
		case RPakStatus_t::PAK_STATUS_LOAD_STARTING:          return "PAK_STATUS_LOAD_STARTING";
		case RPakStatus_t::PAK_STATUS_LOAD_PAKHDR:            return "PAK_STATUS_LOAD_PAKHDR";
		case RPakStatus_t::PAK_STATUS_LOAD_PATCH_INIT:        return "PAK_STATUS_LOAD_PATCH_INIT";
		case RPakStatus_t::PAK_STATUS_LOAD_PATCH_EDIT_STREAM: return "PAK_STATUS_LOAD_PATCH_EDIT_STREAM";
		case RPakStatus_t::PAK_STATUS_LOAD_ASSETS:            return "PAK_STATUS_LOAD_ASSETS";
		case RPakStatus_t::PAK_STATUS_LOADED:                 return "PAK_STATUS_LOADED";
		case RPakStatus_t::PAK_STATUS_UNLOAD_PENDING:         return "PAK_STATUS_UNLOAD_PENDING";
		case RPakStatus_t::PAK_STATUS_FREE_PENDING:           return "PAK_STATUS_FREE_PENDING";
		case RPakStatus_t::PAK_STATUS_CANCELING:              return "PAK_STATUS_CANCELING";
		case RPakStatus_t::PAK_STATUS_ERROR:                  return "PAK_STATUS_ERROR";
		case RPakStatus_t::PAK_STATUS_INVALID_PAKHANDLE:      return "PAK_STATUS_INVALID_PAKHANDLE";
		case RPakStatus_t::PAK_STATUS_BUSY:                   return "PAK_STATUS_BUSY";
		default:                                              return "PAK_STATUS_UNKNOWN";
	}
}
#ifdef GAMEDLL_S3
//-----------------------------------------------------------------------------
// Purpose: process guid relations for asset
//-----------------------------------------------------------------------------
void RTech::PakProcessGuidRelationsForAsset(PakFile_t* pPak, RPakAssetEntry_t* pAsset)
{
#if defined (GAMEDLL_S0) && defined (GAMEDLL_S1) && defined (GAMEDLL_S2)
	static const int GLOBAL_MUL = 0x1D;
#else
	static const int GLOBAL_MUL = 0x17;
#endif

	RPakDescriptor_t* pGuidDescriptors = &pPak->m_pGuidDescriptors[pAsset->m_nUsesStartIdx];
	volatile uint32_t* v5 = reinterpret_cast<volatile uint32_t*>(*(reinterpret_cast<uint64_t*>(g_pPakGlobals) + GLOBAL_MUL * (pPak->qword578 & 0x1FF) + 0x160212));
	const bool bDebug = rtech_debug->GetBool();

	if (bDebug)
		DevMsg(eDLL_T::RTECH, "Processing GUID relations for asset '0x%-16llX' in pak '%-32s'. Uses: %-4i\n", pAsset->m_Guid, pPak->m_pszFileName, pAsset->m_nUsesCount);

	for (uint32_t i = 0; i < pAsset->m_nUsesCount; i++)
	{
		void** pCurrentGuid = reinterpret_cast<void**>(pPak->m_ppPagePointers[pGuidDescriptors[i].m_Index] + pGuidDescriptors[i].m_Offset);

		// Get current guid.
		const uint64_t currentGuid = reinterpret_cast<uint64_t>(*pCurrentGuid);

		// Get asset index.
		int assetIdx = currentGuid & 0x3FFFF;
		uint64_t assetIdxEntryGuid = g_pPakGlobals->m_Assets[assetIdx].m_Guid;

		const int64_t v9 = 2i64 * InterlockedExchangeAdd(v5, 1u);
		*reinterpret_cast<uint64_t*>(const_cast<uint32_t*>(&v5[2 * v9 + 2])) = currentGuid;
		*reinterpret_cast<uint64_t*>(const_cast<uint32_t*>(&v5[2 * v9 + 4])) = pAsset->m_Guid;

		std::function<bool(bool)> fnCheckAsset = [&](bool shouldCheckTwo)
		{
			while (true)
			{
				if (shouldCheckTwo && assetIdxEntryGuid == 2)
				{
					if (pPak->m_PakHdr.m_nAssetEntryCount)
						return false;
				}

				assetIdx++;

				// Check if we have a deadlock and report it if we have rtech_debug enabled.
				if (bDebug && assetIdx >= 0x40000)
				{
					Warning(eDLL_T::RTECH, "Possible deadlock detected while processing asset '0x%-16llX' in pak '%-32s'. Uses: %-4i | assetIdxEntryGuid: '0x%-16llX' | currentGuid: '0x%-16llX'\n", pAsset->m_Guid, pPak->m_pszFileName, pAsset->m_nUsesCount, assetIdxEntryGuid, currentGuid);
					if (IsDebuggerPresent())
						DebugBreak();
				}

				assetIdx &= 0x3FFFF;
				assetIdxEntryGuid = g_pPakGlobals->m_Assets[assetIdx].m_Guid;

				if (assetIdxEntryGuid == currentGuid)
					return true;
			}
		};

		if (assetIdxEntryGuid != currentGuid)
		{
			// Are we some special asset with the guid 2?
			if (!fnCheckAsset(true))
			{
				RPakAssetEntry_t* assetEntries = pPak->m_pAssetEntries;
				uint64_t a = 0;

				for (; assetEntries->m_Guid != currentGuid; a++, assetEntries++)
				{
					if (a >= pPak->m_PakHdr.m_nAssetEntryCount)
					{
						fnCheckAsset(false);
						break;
					}
				}

				assetIdx = pPak->qword580[a];
			}
		}

		// Finally write the pointer to the guid entry.
		*pCurrentGuid = g_pPakGlobals->m_Assets[assetIdx].m_pHead;
	}
}
#endif // GAMEDLL_S3
void V_RTechUtils::Attach() const
{
	DetourAttach(&RTech_OpenFile, &RTech::OpenFile);

#ifdef GAMEDLL_S3
	DetourAttach(&RTech_Pak_ProcessGuidRelationsForAsset, &RTech::PakProcessGuidRelationsForAsset);
#endif

#if !defined (DEDICATED) && defined (GAMEDLL_S3)
	DetourAttach(&RTech_CreateDXTexture, &RTech::CreateDXTexture);
#endif // !DEDICATED
}

void V_RTechUtils::Detach() const
{
	DetourDetach(&RTech_OpenFile, &RTech::OpenFile);

#ifdef GAMEDLL_S3
	DetourDetach(&RTech_Pak_ProcessGuidRelationsForAsset, &RTech::PakProcessGuidRelationsForAsset);
#endif

#if !defined (DEDICATED) && defined (GAMEDLL_S3)
	DetourDetach(&RTech_CreateDXTexture, &RTech::CreateDXTexture);
#endif // !DEDICATED
}

///////////////////////////////////////////////////////////////////////////////
RTech* g_pRTech = new RTech();
