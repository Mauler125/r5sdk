#include "core/stdafx.h"
#include "rtech/rtech_utils.h"
#ifndef DEDICATED
#include "windows/id3dx.h"
#include "materialsystem/cshaderglue.h"
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
	state->input_byte_pos = input_byte_pos_init;
	state->m_nDecompSize = byte_init & ((1i64 << decompressed_size_bits) - 1) | (1i64 << decompressed_size_bits);
	byte_1_low = *(uint64_t*)((mask & input_byte_pos_init) + file_buf) << (64
		- ((uint8_t)decompressed_size_bits
			+ 6));
	input_byte_pos_1 = input_byte_pos_init + ((uint64_t)(uint32_t)(decompressed_size_bits + 6) >> 3);
	state->input_byte_pos = input_byte_pos_1;
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
	state->input_byte_pos = input_byte_pos_final;
	if (inv_mask_in == -1i64)
	{
		state->header_skip_bytes_bs = 0;
		stream_len_needed = fileSize;
	}
	else
	{
		brih_bytes = brih_bits >> 3;
		state->header_skip_bytes_bs = brih_bytes + 1;
		byte_tmp = *(uint64_t*)((mask & input_byte_pos_final) + file_buf);
		state->input_byte_pos = input_byte_pos_final + brih_bytes + 1;
		stream_len_needed = byte_tmp & ((1i64 << (8 * ((uint8_t)brih_bytes + 1))) - 1);
	}
	result = state->m_nDecompSize;
	inv_mask_out = state->m_nInvMaskOut;
	qw70 = offNoHeader + state->m_nInvMaskIn - 6i64;
	state->m_nLengthNeeded = stream_len_needed + offNoHeader;
	state->qword70 = qw70;
	state->byte = byte_final;
	state->byte_bit_offset = byte_bit_offset_final;
	state->dword6C = 0;
	state->m_nCompressedStreamSize = stream_len_needed + offNoHeader;
	state->m_nDecompStreamSize = result;
	if (result - 1 > inv_mask_out)
	{
		stream_compressed_size_new = stream_len_needed + offNoHeader - state->header_skip_bytes_bs;
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
	uint64_t decompressed_position;        // r15
	uint32_t byte_bit_offset;              // ebp
	uint64_t byte;                         // rsi
	uint64_t input_byte_pos;               // rdi
	uint64_t some_size;                    // r12
	uint32_t dword6C;                      // ecx MAPDST
	uint64_t v12;                          // rsi
	uint64_t i;                            // rax
	uint64_t dword6c_shl8;                 // r8
	int64_t dword6c_old;                   // r9
	int32_t LUT_200_val;                   // ecx
	uint64_t v17;                          // rax
	uint64_t byte_new;                     // rsi
	int64_t  LUT_0_VAL;                    // r14
	int32_t byte_4bits_1;                  // ecx
	uint64_t v21;                          // r11
	int32_t v22;                           // edx
	uint64_t out_mask;                     // rax
	int32_t v24;                           // er8
	uint32_t LUT_400_seek_backwards;       // er13
	uint64_t out_seek_back;                // r10
	uint64_t out_seekd_1;                  // rax
	uint64_t* out_seekd_back;              // r10
	uint64_t decompressed_size;            // r9
	uint64_t inv_mask_in;                  // r10
	uint64_t header_skip_bytes_bs;         // r8
	uint64_t v32;                          // rax
	uint64_t v33;                          // rax
	uint64_t v34;                          // rax
	uint64_t stream_decompressed_size_new; // rcx
	int64_t  v36;                          // rdx
	uint64_t len_needed_new;               // r14
	uint64_t stream_compressed_size_new;   // r11
	char v39;                                   // cl MAPDST
	uint64_t v40;                          // rsi MAPDST
	uint64_t v46;                               // rcx
	int64_t v47;                           // r9
	int64_t m;                             // r8
	uint32_t v49;                          // er9
	int64_t v50;                           // r8
	int64_t v51;                           // rdx
	int64_t k;                             // r8
	char* v53;                                  // r10
	int64_t  v54;                          // rdx
	uint32_t lut0_val_abs;                 // er14
	int64_t* in_seekd;                     // rdx
	int64_t* out_seekd;                    // r8
	int64_t  byte_3bits;                   // rax MAPDST
	uint64_t byte_new_tmp;                 // r9 MAPDST
	int32_t LUT_4D0_480;                   // er10 MAPDST
	uint8_t LUT_4D8_4C0_nBits;             // cl MAPDST
	uint64_t byte_4bits;                   // rax MAPDST
	uint32_t copy_bytes_ammount;           // er14
	uint32_t j;                            // ecx
	int64_t v67;                           // rax
	uint64_t v68;                          // rcx
	uint8_t result;                        // al

	if (inLen < state->m_nLengthNeeded)
		return 0;

	decompressed_position = state->m_nDecompPosition;
	if (outLen < state->m_nInvMaskOut + (decompressed_position & ~state->m_nInvMaskOut) + 1 && outLen < state->m_nDecompSize)
		return 0;

	byte_bit_offset = state->byte_bit_offset; // Keeping copy since we increment it down below.
	byte = state->byte; // Keeping copy since its getting overwritten down below.
	input_byte_pos = state->input_byte_pos; // Keeping copy since we increment it down below.
	some_size = state->qword70;
	if (state->m_nCompressedStreamSize < some_size)
		some_size = state->m_nCompressedStreamSize;
	dword6C = state->dword6C;

	if (!byte_bit_offset)
		goto LABEL_9;

	v12 = (*(uint64_t*)((input_byte_pos & state->m_nMask) + state->m_nInputBuf) << (64 - (uint8_t)byte_bit_offset)) | byte;
	for (i = byte_bit_offset; ; i = byte_bit_offset)
	{
		byte_bit_offset &= 7u;
		input_byte_pos += i >> 3;
		byte = (0xFFFFFFFFFFFFFFFFui64 >> byte_bit_offset) & v12;
	LABEL_9:
		dword6c_shl8 = (uint64_t)dword6C << 8;
		dword6c_old = dword6C;
		LUT_200_val = LUT_200[(uint8_t)byte + dword6c_shl8];// LUT_200 - u8 - ammount of bits
		v17 = (uint8_t)byte + dword6c_shl8;
		byte_bit_offset += LUT_200_val;
		byte_new = byte >> LUT_200_val;
		LUT_0_VAL = LUT_0[v17];// LUT_0 - i32 - signed, ammount of bytes

		if (LUT_0_VAL < 0)
		{
			lut0_val_abs = -(int32_t)LUT_0_VAL;
			in_seekd = (int64_t*)(state->m_nInputBuf + (input_byte_pos & state->m_nMask));
			dword6C = 1;
			out_seekd = (int64_t*)(state->m_nOut + (decompressed_position & state->m_nOutMask));
			if (lut0_val_abs == LUT_4E0[dword6c_old])
			{
				if ((~input_byte_pos & state->m_nInvMaskIn) < 0xF
					|| (state->m_nInvMaskOut & ~decompressed_position) < 0xF
					|| state->m_nDecompSize - decompressed_position < 0x10)
				{
					lut0_val_abs = 1;
				}

				v39 = byte_new;
				v40 = byte_new >> 3;
				byte_3bits = v39 & 7;
				byte_new_tmp = v40;

				if (byte_3bits)
				{
					LUT_4D0_480 = LUT_4D0[byte_3bits];// LUT_4D0 - u8
					LUT_4D8_4C0_nBits = LUT_4D8[byte_3bits];// LUT_4D8 - u8 - ammount of bits
				}
				else
				{
					byte_new_tmp = v40 >> 4;
					byte_4bits = v40 & 15;
					byte_bit_offset += 4;
					LUT_4D0_480 = LUT_480[byte_4bits];// LUT_480 - u32
					LUT_4D8_4C0_nBits = LUT_4C0[byte_4bits]; // LUT_4C0 - u8 - ammount of bits???
				}

				byte_bit_offset += LUT_4D8_4C0_nBits + 3;
				byte_new = byte_new_tmp >> LUT_4D8_4C0_nBits;
				copy_bytes_ammount = LUT_4D0_480 + (byte_new_tmp & ((1 << LUT_4D8_4C0_nBits) - 1)) + lut0_val_abs;

				for (j = copy_bytes_ammount >> 3; j; --j)// copy by 8 bytes
				{
					v67 = *in_seekd++;
					*out_seekd++ = v67;
				}

				if ((copy_bytes_ammount & 4) != 0)    // copy by 4
				{
					*(uint32_t*)out_seekd = *(uint32_t*)in_seekd;
					out_seekd = (int64_t*)((char*)out_seekd + 4);
					in_seekd = (int64_t*)((char*)in_seekd + 4);
				}

				if ((copy_bytes_ammount & 2) != 0)    // copy by 2
				{
					*(uint16_t*)out_seekd = *(uint16_t*)in_seekd;
					out_seekd = (int64_t*)((char*)out_seekd + 2);
					in_seekd = (int64_t*)((char*)in_seekd + 2);
				}

				if ((copy_bytes_ammount & 1) != 0)    // copy by 1
					*(uint8_t*)out_seekd = *(uint8_t*)in_seekd;

				input_byte_pos += copy_bytes_ammount;
				decompressed_position += copy_bytes_ammount;
			}
			else
			{
				*out_seekd = *in_seekd;
				out_seekd[1] = in_seekd[1];
				input_byte_pos += lut0_val_abs;
				decompressed_position += lut0_val_abs;
			}
		}
		else
		{
			byte_4bits_1 = byte_new & 0xF;
			dword6C = 0;
			v21 = ((uint64_t)(uint32_t)byte_new >> (((uint32_t)(byte_4bits_1 + 0xFFFFFFE1) >> 3) & 6)) & 0x3F;// 6 bits after shift for who knows how much???
			v22 = 1 << (byte_4bits_1 + ((byte_new >> 4) & ((24 * (((uint32_t)(byte_4bits_1 + 0xFFFFFFE1) >> 3) & 2)) >> 4)));// ammount of bits to read???
			byte_bit_offset += (((uint32_t)(byte_4bits_1 + 0xFFFFFFE1) >> 3) & 6)// shit shit gets shifted by ammount of bits it read or something
				+ LUT_440[v21]
				+ byte_4bits_1
				+ ((byte_new >> 4) & ((24 * (((uint32_t)(byte_4bits_1 + 0xFFFFFFE1) >> 3) & 2)) >> 4));
			out_mask = state->m_nOutMask;
			v24 = 16
				* (v22
					+ ((v22 - 1) & (byte_new >> ((((uint32_t)(byte_4bits_1 + 0xFFFFFFE1) >> 3) & 6)
						+ LUT_440[v21]))));
			byte_new >>= (((uint32_t)(byte_4bits_1 + 0xFFFFFFE1) >> 3) & 6)
				+ LUT_440[v21]
				+ byte_4bits_1
				+ ((byte_new >> 4) & ((24 * (((uint32_t)(byte_4bits_1 + 0xFFFFFFE1) >> 3) & 2)) >> 4));
			LUT_400_seek_backwards = v24 + LUT_400[v21] - 16;// LUT_400 - u8 - seek backwards
			out_seek_back = out_mask & (decompressed_position - LUT_400_seek_backwards);
			out_seekd_1 = state->m_nOut + (decompressed_position & out_mask);
			out_seekd_back = (uint64_t*)(state->m_nOut + out_seek_back);
			if ((int32_t)LUT_0_VAL == 17)
			{
				v39 = byte_new;
				v40 = byte_new >> 3;
				byte_3bits = v39 & 7;
				byte_new_tmp = v40;
				if (byte_3bits)
				{
					LUT_4D0_480 = LUT_4D0[byte_3bits];
					LUT_4D8_4C0_nBits = LUT_4D8[byte_3bits];
				}
				else
				{
					byte_bit_offset += 4;
					byte_4bits = v40 & 0xF;
					byte_new_tmp = v40 >> 4;
					LUT_4D0_480 = LUT_480[byte_4bits];
					LUT_4D8_4C0_nBits = LUT_4C0[byte_4bits];
					if (state->m_nInputBuf && byte_bit_offset + LUT_4D8_4C0_nBits >= 0x3D)
					{
						v46 = input_byte_pos++ & state->m_nMask;
						byte_new_tmp |= (uint64_t) * (uint8_t*)(v46 + state->m_nInputBuf) << (61
							- (uint8_t)byte_bit_offset);
						byte_bit_offset -= 8;
					}
				}
				byte_bit_offset += LUT_4D8_4C0_nBits + 3;
				byte_new = byte_new_tmp >> LUT_4D8_4C0_nBits;
				v47 = ((uint32_t)byte_new_tmp & ((1 << LUT_4D8_4C0_nBits) - 1)) + LUT_4D0_480 + 17;
				decompressed_position += v47;
				if (LUT_400_seek_backwards < 8)
				{
					v49 = v47 - 13;
					decompressed_position -= 13i64;
					if (LUT_400_seek_backwards == 1)    // 1 means copy v49 qwords?
					{
						v50 = *(uint8_t*)out_seekd_back;
						v51 = 0i64;
						for (k = 0x101010101010101i64 * v50; (uint32_t)v51 < v49; v51 = (uint32_t)(v51 + 8))
							*(uint64_t*)(v51 + out_seekd_1) = k;
					}
					else
					{
						if (v49)
						{
							v53 = (char*)out_seekd_back - out_seekd_1;
							v54 = v49;
							do
							{
								*(uint8_t*)out_seekd_1 = v53[out_seekd_1];// seekd = seek_back; increment ptrs
								++out_seekd_1;
								--v54;
							} while (v54);
						}
					}
				}
				else
				{
					for (m = 0i64; (uint32_t)m < (uint32_t)v47; m = (uint32_t)(m + 8))
						*(uint64_t*)(m + out_seekd_1) = *(uint64_t*)((char*)out_seekd_back + m);
				}
			}
			else
			{
				decompressed_position += LUT_0_VAL;
				*(uint64_t*)out_seekd_1 = *out_seekd_back;
				*(uint64_t*)(out_seekd_1 + 8) = out_seekd_back[1];
			}
		}
		if (input_byte_pos >= some_size)
			break;

	LABEL_26:
		v12 = (*(uint64_t*)((input_byte_pos & state->m_nMask) + state->m_nInputBuf) << (64 - (uint8_t)byte_bit_offset)) | byte_new;
	}

	if (decompressed_position != state->m_nDecompStreamSize)
		goto LABEL_22;

	decompressed_size = state->m_nDecompSize;
	if (decompressed_position == decompressed_size)
	{
		state->input_byte_pos = input_byte_pos;
		result = 1;
		state->m_nDecompPosition = decompressed_position;
		return result;
	}

	inv_mask_in = state->m_nInvMaskIn;
	header_skip_bytes_bs = state->header_skip_bytes_bs;
	v32 = inv_mask_in & -(int64_t)input_byte_pos;
	byte_new >>= 1;
	++byte_bit_offset;

	if (header_skip_bytes_bs > v32)
	{
		input_byte_pos += v32;
		v33 = state->qword70;
		if (input_byte_pos > v33)
			state->qword70 = inv_mask_in + v33 + 1;
	}

	v34 = input_byte_pos & state->m_nMask;
	input_byte_pos += header_skip_bytes_bs;
	stream_decompressed_size_new = decompressed_position + state->m_nInvMaskOut + 1;
	v36 = *(uint64_t*)(v34 + state->m_nInputBuf) & ((1LL << (8 * (uint8_t)header_skip_bytes_bs)) - 1);
	len_needed_new = v36 + state->m_nLengthNeeded;
	stream_compressed_size_new = v36 + state->m_nCompressedStreamSize;
	state->m_nLengthNeeded = len_needed_new;
	state->m_nCompressedStreamSize = stream_compressed_size_new;

	if (stream_decompressed_size_new >= decompressed_size)
	{
		stream_decompressed_size_new = decompressed_size;
		state->m_nCompressedStreamSize = header_skip_bytes_bs + stream_compressed_size_new;
	}

	state->m_nDecompStreamSize = stream_decompressed_size_new;

	if (inLen >= len_needed_new && outLen >= stream_decompressed_size_new)
	{
	LABEL_22:
		some_size = state->qword70;
		if (input_byte_pos >= some_size)
		{
			input_byte_pos = ~state->m_nInvMaskIn & (input_byte_pos + 7);
			some_size += state->m_nInvMaskIn + 1;
			state->qword70 = some_size;
		}
		if (state->m_nCompressedStreamSize < some_size)
			some_size = state->m_nCompressedStreamSize;
		goto LABEL_26;
	}

	v68 = state->qword70;

	if (input_byte_pos >= v68)
	{
		input_byte_pos = ~inv_mask_in & (input_byte_pos + 7);
		state->qword70 = v68 + inv_mask_in + 1;
	}

	state->dword6C = dword6C;
	result = 0;
	state->input_byte_pos = input_byte_pos;
	state->m_nDecompPosition = decompressed_position;
	state->byte = byte_new;
	state->byte_bit_offset = byte_bit_offset;

	return result;
}

#if not defined DEDICATED && defined (GAMEDLL_S3)

#pragma warning( push )
// Disable stack warning, tells us to move more data to the heap instead. Not really possible with 'initialData' here. Since its parallel processed.
// Also disable 6378, complains that there is no control path where it would use 'nullptr', if that happens 'Error' will be called though.
#pragma warning( disable : 6262 6387)
//----------------------------------------------------------------------------------
// Purpose: creates 2D texture and shader resource from textureHeader and imageData.
//----------------------------------------------------------------------------------
void RTech::CreateDXTexture(RTechTextureInfo_t* textureHeader, int64_t imageData)
{
	if (textureHeader->unk0 && !textureHeader->m_nHeight) // Return never gets hit. Maybe its some debug check?
		return;

	__int64 initialData[4096]{};
	textureHeader->m_nTextureMipLevels = textureHeader->m_nMipLevels;

	const int totalStreamedMips = textureHeader->m_nMipLevelsStreamedOpt + textureHeader->m_nMipLevelsStreamed;
	uint32_t mipLevel = textureHeader->m_nMipLevels + totalStreamedMips;
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
	
				uint8_t x = s_pRTechBytesPerPixel[textureHeader->m_nFormat].first;
				uint8_t y = s_pRTechBytesPerPixel[textureHeader->m_nFormat].second;

				uint32_t bytesPerPixelWidth = (y + mipWidth) >> (y >> 1);
				uint32_t bytesPerPixelHeight = (y + mipHeight) >> (y >> 1);
				uint32_t sliceWidth = x * (y >> (y >> 1));

				uint32_t rowPitch = sliceWidth * bytesPerPixelWidth;
				uint32_t slicePitch = x * bytesPerPixelWidth * bytesPerPixelHeight;

				uint32_t subResourceEntry = mipLevel;
				for (int i = 0; i < textureHeader->m_nArraySize; i++)
				{
					uint32_t offsetCurrentResourceData = subResourceEntry << 4u;

					*(int64_t*)((uint8_t*)initialData + offsetCurrentResourceData) = imageData;
					*(uint32_t*)((uint8_t*)&initialData[1] + offsetCurrentResourceData) = rowPitch;
					*(uint32_t*)((uint8_t*)&initialData[1] + offsetCurrentResourceData + 4) = slicePitch;

					imageData += (slicePitch + 15) & 0xFFFFFFF0;
					subResourceEntry += textureHeader->m_nMipLevels;
				}
			}
		} while (mipLevel != totalStreamedMips);
	}

	const DXGI_FORMAT dxgiFormat = rpakToDxgiFormat[textureHeader->m_nFormat]; // Get dxgi format

	D3D11_TEXTURE2D_DESC textureDesc{};
	textureDesc.Width = textureHeader->m_nWidth >> mipLevel;
	textureDesc.Height = textureHeader->m_nHeight >> mipLevel;
	textureDesc.MipLevels = textureHeader->m_nMipLevels;
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
		Error(eDLL_T::RTECH, "Couldn't create texture \"%s\": error code %08x\n", textureHeader->m_nDebugName, createTextureRes);

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
		Error(eDLL_T::RTECH, "Couldn't create shader resource view for texture \"%s\": error code %08x\n", textureHeader->m_nDebugName, createShaderResourceRes);
}
#pragma warning( pop )
#endif

#ifndef DEDICATED
//----------------------------------------------------------------------------------
// Purpose: start loading shader sets, assign vtable pointer
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
int32_t RTech::OpenFile(const char* szFilePath, void* unused, int64_t* fileSizeOut)
{
	const HANDLE hFile = CreateFileA(szFilePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, 0, OPEN_EXISTING, FILE_SUPPORTS_GHOSTING, 0);
	if (hFile == INVALID_HANDLE_VALUE)
		return -1;

	if (fileSizeOut)
	{
		LARGE_INTEGER fileSize{};
		if (GetFileSizeEx(hFile, &fileSize))
			*fileSizeOut = fileSize.QuadPart;
	}

	AcquireSRWLockExclusive(reinterpret_cast<PSRWLOCK>(&*g_pPakFileSlotLock));
	const int32_t fileIdx = RTech_FindFreeSlotInFiles(s_pFileArray);
	ReleaseSRWLockExclusive(reinterpret_cast<PSRWLOCK>(&*g_pPakFileSlotLock));

	const int32_t fileHandleIdx =  (fileIdx & 0x3FF); // Something with ArraySize.

	m_FileHandles->self[fileHandleIdx].m_nFileNumber = fileIdx;
	m_FileHandles->self[fileHandleIdx].m_hFileHandle = hFile;
	m_FileHandles->self[fileHandleIdx].m_nCurOfs = 1;

	return fileIdx;
}

//-----------------------------------------------------------------------------
// Purpose: gets information about loaded pak file via pak ID
//-----------------------------------------------------------------------------
RPakLoadedInfo_t* RTech::GetPakLoadedInfo(int nPakId)
{
	for (int16_t i = 0; i < *s_pLoadedPakCount; ++i)
	{
		RPakLoadedInfo_t* info = &g_pLoadedPakInfo[i];
		if (!info)
			continue;

		if (info->m_nPakId != nPakId)
			continue;

		return info;
	}

	Warning(eDLL_T::RTECH, "%s - Failed getting RPakLoadInfo_t for PakId '%d'\n", __FUNCTION__, nPakId);
	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: gets information about loaded pak file via pak name
//-----------------------------------------------------------------------------
RPakLoadedInfo_t* RTech::GetPakLoadedInfo(const char* szPakName)
{
	for (int16_t i = 0; i < *s_pLoadedPakCount; ++i)
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

	Warning(eDLL_T::RTECH, "%s - Failed getting RPakLoadInfo_t for Pak '%s'\n", __FUNCTION__, szPakName);
	return nullptr;
}

void RTech_Utils_Attach()
{
	//DetourAttach((LPVOID*)&RTech_OpenFile, &RTech::OpenFile); // !FIXME: Loading override rpaks doesn't work with this, disabled for now.

#if not defined DEDICATED && defined (GAMEDLL_S3)
	DetourAttach((LPVOID*)&RTech_CreateDXTexture, &RTech::CreateDXTexture);
#endif
}

void RTech_Utils_Detach()
{
	// [ PIXIE ]: Everything related to RTech::OpenFile should be compatible across seasons.
	//DetourDetach((LPVOID*)&RTech_OpenFile, &RTech::OpenFile);

#if not defined DEDICATED && defined (GAMEDLL_S3)
	DetourDetach((LPVOID*)&RTech_CreateDXTexture, &RTech::CreateDXTexture);
#endif
}

///////////////////////////////////////////////////////////////////////////////
RTech* g_pRTech = new RTech();
