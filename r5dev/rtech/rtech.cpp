#include "core/stdafx.h"
#include "rtech/rtech.h"
#include "engine/sys_utils.h"

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

******************************************************************************/

//-----------------------------------------------------------------------------
// Purpose: calculate 'GUID' from input data
//-----------------------------------------------------------------------------
std::uint64_t __fastcall RTech::StringToGuid(const char* pData)
{
	std::uint32_t*        v1; // r8
	std::uint64_t         v2; // r10
	int                   v3; // er11
	std::uint32_t         v4; // er9
	std::uint32_t          i; // edx
	std::uint64_t         v6; // rcx
	int                   v7; // er9
	int                   v8; // edx
	int                   v9; // eax
	std::uint32_t        v10; // er8
	int                  v12; // ecx
	std::uint32_t* a1 = (std::uint32_t*)pData;

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
		v2 = ((((std::uint64_t)(0xFB8C4D96501i64 * v6) >> 24) + 0x633D5F1 * v2) >> 61) ^ (((std::uint64_t)(0xFB8C4D96501i64 * v6) >> 24)
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
	return 0x633D5F1 * v2 + ((0xFB8C4D96501i64 * (std::uint64_t)(v4 & v10)) >> 24) - 0xAE502812AA7333i64 * (std::uint32_t)(v3 + v9 / 8);
}

//-----------------------------------------------------------------------------
// Purpose: calculate 'decompressed' size and commit parameters
//-----------------------------------------------------------------------------
std::uint32_t __fastcall RTech::DecompressedSize(std::int64_t param_buf, std::uint8_t* file_buf, std::int64_t file_size, std::int64_t off_no_header, std::int64_t header_size)
{
	std::int64_t       v8;    // r9
	std::uint64_t      v9;    // r11
	char              v10;    // r8
	int               v11;    // er8
	std::int64_t      v12;    // rbx
	unsigned int      v13;    // ebp
	std::uint64_t     v14;    // rbx
	std::int64_t      v15;    // rax
	unsigned int      v16;    // er9
	std::uint64_t     v17;    // r12
	std::uint64_t     v18;    // r11
	std::uint64_t     v19;    // r10
	std::uint64_t     v20;    // rax
	int               v21;    // ebp
	std::uint64_t     v22;    // r10
	unsigned int      v23;    // er9
	std::int64_t      v24;    // rax
	std::int64_t      v25;    // rsi
	std::int64_t      v26;    // rdx
	std::int64_t      v28;    // rdx
	std::int64_t      v29;    // [rsp+48h] [rbp+18h]
	std::int64_t   result;    // rax

	v29 = 0xFFFFFFi64;
	*(std::uint64_t*)param_buf = (std::uint64_t)file_buf;
	*(std::uint64_t*)(param_buf + 32) = off_no_header + file_size;
	*(std::uint64_t*)(param_buf + 8) = 0i64;
	*(std::uint64_t*)(param_buf + 24) = 0i64;
	*(std::uint32_t*)(param_buf + 68) = 0;
	*(std::uint64_t*)(param_buf + 16) = -1i64;
	v8 = off_no_header + header_size + 8;
	v9 = *(std::uint64_t*)((0xFFFFFFi64 & (off_no_header + header_size)) + file_buf);
	*(std::uint64_t*)(param_buf + 80) = header_size;
	*(std::uint64_t*)(param_buf + 72) = v8;
	v10 = v9;
	v9 >>= 6;
	v11 = v10 & 0x3F;
	*(std::uint64_t*)(param_buf + 40) = (1i64 << v11) | v9 & ((1i64 << v11) - 1);
	v12 = *(std::uint64_t*)((0xFFFFFFi64 & v8) + file_buf) << (64 - ((std::uint8_t)v11 + 6));
	*(std::uint64_t*)(param_buf + 72) = v8 + ((std::uint8_t)(unsigned int)(v11 + 6) >> 3);
	v13 = ((v11 + 6) & 7) + 13;
	v14 = (0xFFFFFFFFFFFFFFFFui64 >> ((v11 + 6) & 7)) & ((v9 >> v11) | v12);
	v15 = v29 & *(std::uint64_t*)(param_buf + 72);
	v16 = (((std::uint8_t)v14 - 1) & 0x3F) + 1;
	v17 = 0xFFFFFFFFFFFFFFFFui64 >> (64 - (std::uint8_t)v16);
	*(uint64_t*)(param_buf + 48) = v17;
	v18 = 0xFFFFFFFFFFFFFFFFui64 >> (64 - ((((v14 >> 6) - 1) & 0x3F) + 1));
	*(uint64_t*)(param_buf + 56) = v18;
	v19 = (v14 >> 13) | (*(std::uint64_t*)(v15 + file_buf) << (64 - (std::uint8_t)v13));
	v20 = v13;
	v21 = v13 & 7;
	*(std::uint64_t*)(param_buf + 72) += v20 >> 3;
	v22 = (0xFFFFFFFFFFFFFFFFui64 >> v21) & v19;
	if (v17 == -1i64)
	{
		*(std::uint32_t*)(param_buf + 64) = 0;
		*(std::uint64_t*)(param_buf + 88) = file_size;
	}
	else
	{
		v23 = v16 >> 3;
	 	v24 = v29 & *(std::uint64_t*)(param_buf + 72);
		*(std::uint32_t*)(param_buf + 64) = v23 + 1;
	 	v25 = *(std::uint64_t*)(v24 + file_buf) & ((1i64 << (8 * ((std::uint8_t)v23 + 1))) - 1);
		*(std::uint64_t*)(param_buf + 72) += v23 + 1;
		*(std::uint64_t*)(param_buf + 88) = v25;
	}
	*(std::uint64_t*)(param_buf + 88) += off_no_header;
	v26 = *(std::uint64_t*)(param_buf + 88);
	*(std::uint64_t*)(param_buf + 96) = v22;
	*(std::uint32_t*)(param_buf + 104) = v21;
	*(std::uint64_t*)(param_buf + 112) = v17 + off_no_header - 6;
	result = *(std::uint64_t*)(param_buf + 40);
	*(std::uint32_t*)(param_buf + 108) = 0;
	*(std::uint64_t*)(param_buf + 120) = v26;
	*(std::uint64_t*)(param_buf + 128) = result;
	if ((((std::uint8_t)(v14 >> 6) - 1) & 0x3F) != -1i64 && result - 1 > v18)
	{
		v28 = v26 - *(unsigned int*)(param_buf + 64);
		*(std::uint64_t*)(param_buf + 128) = v18 + 1;
		*(std::uint64_t*)(param_buf + 120) = v28;
	}
	return result;
}

//-----------------------------------------------------------------------------
// Purpose: decompress input data
//-----------------------------------------------------------------------------
std::uint8_t __fastcall RTech::Decompress(std::int64_t* param_buffer, std::uint64_t file_size, std::uint64_t buf_size)
{
	char        result; // al
	std::int64_t    v5; // r15
	std::int64_t    v6; // r11
	std::uint32_t   v7; // ebp
	std::uint64_t   v8; // rsi
	std::uint64_t   v9; // rdi
	std::uint64_t  v10; // r12
	std::int64_t   v11; // r13
	std::uint32_t  v12; // ecx
	std::uint64_t  v13; // rsi
	std::uint64_t    i; // rax
	std::uint64_t  v15; // r8
	std::int64_t   v16; // r9
	int            v17; // ecx
	std::uint64_t  v18; // rax
	std::uint64_t  v19; // rsi
	std::int64_t   v20; // r14
	int            v21; // ecx
	std::uint64_t  v22; // r11
	int            v23; // edx
	std::int64_t   v24; // rax
	int            v25; // er8
	std::uint32_t  v26; // er13
	std::int64_t   v27; // r10
	std::int64_t   v28; // rax
	std::uint64_t* v29; // r10
	std::uint64_t  v30; // r9
	std::uint64_t  v31; // r10
	std::uint64_t  v32; // r8
	std::uint64_t  v33; // rax
	std::uint64_t  v34; // rax
	std::uint64_t  v35; // rax
	std::uint64_t  v36; // rcx
	std::uint64_t  v37; // rdx
	std::uint64_t  v38; // r14
	std::uint64_t  v39; // r11
	char           v40; // cl
	std::uint64_t  v41; // rsi
	std::uint64_t  v42; // rcx
	std::uint64_t  v43; // r8
	int            v44; // er11
	std::uint8_t   v45; // r9
	std::uint64_t  v46; // rcx
	std::uint64_t  v47; // rcx
	std::uint64_t  v48; // r9
	std::uint64_t    l; // r8
	std::uint32_t  v50; // er9
	std::uint64_t  v51; // r8
	std::uint64_t  v52; // rdx
	std::uint64_t    k; // r8
	char*          v54; // r10
	std::uint64_t  v55; // rdx
	std::uint32_t  v56; // er14
	std::uint64_t* v57; // rdx
	std::uint64_t* v58; // r8
	char           v59; // al
	std::uint64_t  v60; // rsi
	std::uint64_t  v61; // rax
	std::uint64_t  v62; // r9
	int            v63; // er10
	std::uint8_t   v64; // cl
	std::uint64_t  v65; // rax
	std::uint32_t  v66; // er14
	std::uint32_t    j; // ecx
	std::uint64_t  v68; // rax
	std::uint64_t  v69; // rcx
	std::uint64_t  v70; // [rsp+0h] [rbp-58h]
	int            v71; // [rsp+60h] [rbp+8h]
	std::uint64_t  v74; // [rsp+78h] [rbp+20h]
	
	if (file_size < param_buffer[11])
	{
		return 0;
	}
	v5 = param_buffer[10];
	if (buf_size < param_buffer[7] + (v5 & (std::uint64_t)~param_buffer[7]) + 1 && buf_size < param_buffer[5])
	{
		return 0;
	}
	v6 = param_buffer[1];
	v7 = *((std::uint32_t*)param_buffer + 26);
	v8 = param_buffer[12];
	v9 = param_buffer[9];
	v10 = param_buffer[14];
	v11 = *param_buffer;
	if (param_buffer[15] < v10)
	{
		v10 = param_buffer[15];
	}
	v12 = *((std::uint32_t*)param_buffer + 27);
	v74 = v11;
	v70 = v6;
	v71 = v12;
	if (!v7)
	{
		goto LABEL_11;
	}
	v13 = (*(std::uint64_t*)((v9 & param_buffer[2]) + v11) << (64 - (std::uint8_t)v7)) | v8;
	for (i = v7; ; i = v7)
	{
		v7 &= 7u;
		v9 += i >> 3;
		v12 = v71;
		v8 = (0xFFFFFFFFFFFFFFFFui64 >> v7) & v13;
	LABEL_11:
		v15 = (std::uint64_t)v12 << 8;
		v16 = v12;
		v17 = *((std::uint8_t*)&LUT_0 + (std::uint8_t)v8 + v15 + 512);
		v18 = (std::uint8_t)v8 + v15;
		v7 += v17;
		v19 = v8 >> v17;
		v20 = (std::uint32_t)*((char*)&LUT_0 + v18);
		if (*((char*)&LUT_0 + v18) < 0)
		{
			v56 = -(int)v20;
			v57 = (std::uint64_t*)(v11 + (v9 & param_buffer[2]));
			v71 = 1;
			v58 = (std::uint64_t*)(v6 + (v5 & param_buffer[3]));
			if (v56 == *((std::uint8_t*)&LUT_0 + v16 + 1248))
			{
				if ((~v9 & param_buffer[6]) < 0xF || (param_buffer[7] & (std::uint64_t)~v5) < 0xF || (std::uint64_t)(param_buffer[5] - v5) < 0x10)
				{
					v56 = 1;
				}
				v59 = v19;
				v60 = v19 >> 3;
				v61 = v59 & 7;
				v62 = v60;
				if (v61)
				{
					v63 = *((std::uint8_t*)&LUT_0 + v61 + 1232);
					v64 = *((std::uint8_t*)&LUT_0 + v61 + 1240);
				}
				else
				{
					v62 = v60 >> 4;
					v65 = v60 & 0xF;
					v7 += 4;
					v63 = *((std::uint32_t*)&LUT_0 + v65 + 288);
					v64 = *((std::uint8_t*)&LUT_0 + v65 + 1216);
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
					*(std::uint32_t*)v58 = *(std::uint32_t*)v57;
					v58 = (std::uint64_t*)((char*)v58 + 4);
					v57 = (std::uint64_t*)((char*)v57 + 4);
				}
				if ((v66 & 2) != 0)
				{
					*(std::uint16_t*)v58 = *(std::uint16_t*)v57;
					v58 = (std::uint64_t*)((char*)v58 + 2);
					v57 = (std::uint64_t*)((char*)v57 + 2);
				}
				if ((v66 & 1) != 0)
				{
					*(std::uint8_t*)v58 = *(std::uint8_t*)v57;
				}
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
			v22 = ((std::uint64_t)(std::uint32_t)v19 >> (((std::uint32_t)(v21 - 31) >> 3) & 6)) & 0x3F;
			v23 = 1 << (v21 + ((v19 >> 4) & ((24 * (((std::uint32_t)(v21 - 31) >> 3) & 2)) >> 4)));
			v7 += (((std::uint32_t)(v21 - 31) >> 3) & 6) + *((std::uint8_t*)&LUT_0 + v22 + 1088) + v21 + ((v19 >> 4) & ((24 * (((std::uint32_t)(v21 - 31) >> 3) & 2)) >> 4));
			v24 = param_buffer[3];
			v25 = 16 * (v23 + ((v23 - 1) & (v19 >> ((((std::uint32_t)(v21 - 31) >> 3) & 6) + *((std::uint8_t*)&LUT_0 + v22 + 1088)))));
			v19 >>= (((std::uint32_t)(v21 - 31) >> 3) & 6) + *((std::uint8_t*)&LUT_0 + v22 + 1088) + v21 + ((v19 >> 4) & ((24 * (((std::uint32_t)(v21 - 31) >> 3) & 2)) >> 4));
			v26 = v25 + *((std::uint8_t*)&LUT_0 + v22 + 1024) - 16;
			v27 = v24 & (v5 - v26);
			v28 = v70 + (v5 & v24);
			v29 = (std::uint64_t*)(v70 + v27);
			if ((std::uint32_t)v20 == 17)
			{
				v40 = v19;
				v41 = v19 >> 3;
				v42 = v40 & 7;
				v43 = v41;
				if (v42)
				{
					v44 = *((std::uint8_t*)&LUT_0 + v42 + 1232);
					v45 = *((std::uint8_t*)&LUT_0 + v42 + 1240);
				}
				else
				{
					v7 += 4;
					v46 = v41 & 0xF;
					v43 = v41 >> 4;
					v44 = *((std::uint32_t*)&LUT_0 + v46 + 288);
					v45 = *((std::uint8_t*)&LUT_0 + v46 + 1216);
					if (v74 && v7 + v45 >= 0x3D)
					{
						v47 = v9++ & param_buffer[2];
						v43 |= (std::uint64_t)*(std::uint8_t*)(v47 + v74) << (61 - (std::uint8_t)v7);
						v7 -= 8;
					}
				}
				v7 += v45 + 3;
				v19 = v43 >> v45;
				v48 = ((std::uint32_t)v43 & ((1 << v45) - 1)) + v44 + 17;
				v5 += v48;
				if (v26 < 8)
				{
					v50 = v48 - 13;
					v5 -= 13i64;
					if (v26 == 1)
					{
						v51 = *(std::uint8_t*)v29;
						v52 = 0i64;
						for (k = 0x101010101010101i64 * v51; (std::uint32_t)v52 < v50; v52 = (std::uint32_t)(v52 + 8))
						{
							*(std::uint64_t*)(v52 + v28) = k;
						}
					}
					else
					{
						if (v50)
						{
							v54 = (char*)v29 - v28;
							v55 = v50;
							do
							{
								*(std::uint8_t*)v28 = v54[v28];
								++v28;
								--v55;
							} while (v55);
						}
					}
				}
				else
				{
					for (l = 0i64; (std::uint32_t)l < (std::uint32_t)v48; l = (std::uint32_t)(l + 8))
					{
						*(std::uint64_t*)(l + v28) = *(std::uint64_t*)((char*)v29 + l);
					}
				}
			}
			else
			{
				v5 += v20;
				*(std::uint64_t*)v28 = *v29;
				*(std::uint64_t*)(v28 + 8) = v29[1];
			}
			v11 = v74;
		}
		if (v9 >= v10)
		{
			break;
		}
	LABEL_29:
		v6 = v70;
		v13 = (*(std::uint64_t*)((v9 & param_buffer[2]) + v11) << (64 - (std::uint8_t)v7)) | v19;
	}
	if (v5 != param_buffer[16])
	{
		goto LABEL_25;
	}
	v30 = param_buffer[5];
	if (v5 == v30)
	{
		result = 1;
		goto LABEL_69;
	}
	v31 = param_buffer[6];
	v32 = *((std::uint32_t*)param_buffer + 16);
	v33 = v31 & -(std::int64_t)v9;
	v19 >>= 1;
	++v7;
	if (v32 > v33)
	{
		v9 += v33;
		v34 = param_buffer[14];
		if (v9 > v34)
		{
			param_buffer[14] = v31 + v34 + 1;
		}
	}
	v35 = v9 & param_buffer[2];
	v9 += v32;
	v36 = v5 + param_buffer[7] + 1;
	v37 = *(std::uint32_t*)(v35 + v11) & ((1i64 << (8 * (std::uint8_t)v32)) - 1);
	v38 = v37 + param_buffer[11];
	v39 = v37 + param_buffer[15];
	param_buffer[11] = v38;
	param_buffer[15] = v39;
	if (v36 >= v30)
	{
		v36 = v30;
		param_buffer[15] = v32 + v39;
	}
	param_buffer[16] = v36;
	if (file_size >= v38 && buf_size >= v36)
	{
	LABEL_25:
		v10 = param_buffer[14];
		if (v9 >= v10)
		{
			v9 = ~param_buffer[6] & (v9 + 7);
			v10 += param_buffer[6] + 1;
			param_buffer[14] = v10;
		}
		if (param_buffer[15] < v10)
		{
			v10 = param_buffer[15];
		}
		goto LABEL_29;
	}
	v69 = param_buffer[14];
	if (v9 >= v69)
	{
		v9 = ~v31 & (v9 + 7);
		param_buffer[14] = v69 + v31 + 1;
	}
	*((std::uint32_t*)param_buffer + 27) = v71;
	result = 0;
	param_buffer[12] = v19;
	*((std::uint32_t*)param_buffer + 26) = v7;
LABEL_69:
	param_buffer[10] = v5;
	param_buffer[9] = v9;
	return result;
}
///////////////////////////////////////////////////////////////////////////////
RTech* g_pRtech = new RTech();
