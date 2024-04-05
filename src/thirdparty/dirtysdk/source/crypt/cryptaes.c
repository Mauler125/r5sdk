/*H********************************************************************************/
/*!
       \File cryptaes.c
          
       \Description
           An implementation of the AES-128 and AES-256 cipher, based on the FIPS
           standard, intended for use with the TLS AES cipher suites.
   
           This implementation deliberately uses the naming conventions from the
           standard where possible in order to aid comprehension.
   
       \Notes
           References:
               FIPS197 standard: http://csrc.nist.gov/publications/fips/fips197/fips-197.pdf

    \Copyright
        Copyright (c) 2011 Electronic Arts Inc.

    \Version 01/20/2011 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/crypt/cryptaes.h"

/*** Defines **********************************************************************/

/*
    32-bit word rotation
*/
#define CRYPTAES_ROT(_x, _a)        (((_x) << (_a)) | ((_x) >> (32-(_a))))

#define CRYPTAES_ROT1(_x)           CRYPTAES_ROT((_x), 24)
#define CRYPTAES_ROT2(_x)           CRYPTAES_ROT((_x), 16)
#define CRYPTAES_ROT3(_x)           CRYPTAES_ROT((_x), 8)

/*
    Read/write big-endian words
*/
#define CRYPTAES_RDWORD(_ptr)       ((((uint32_t)(_ptr)[0]) << 24) | ((uint32_t)((_ptr)[1]) << 16) | (((uint32_t)(_ptr)[2]) << 8) | ((uint32_t)(_ptr)[3]))
#define CRYPTAES_WRWORD(_x, _ptr)   ((_ptr)[3] = (uint8_t)(_x),\
                                    (_ptr)[2] = (uint8_t)((_x)>>8),\
                                    (_ptr)[1] = (uint8_t)((_x)>>16),\
                                    (_ptr)[0] = (uint8_t)((_x)>>24))

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

// AES s-box table
static const uint8_t _CryptAes_Sbox[256] =
{
    0x63,0x7C,0x77,0x7B, 0xF2,0x6B,0x6F,0xC5, 0x30,0x01,0x67,0x2B, 0xFE,0xD7,0xAB,0x76,
    0xCA,0x82,0xC9,0x7D, 0xFA,0x59,0x47,0xF0, 0xAD,0xD4,0xA2,0xAF, 0x9C,0xA4,0x72,0xC0,
    0xB7,0xFD,0x93,0x26, 0x36,0x3F,0xF7,0xCC, 0x34,0xA5,0xE5,0xF1, 0x71,0xD8,0x31,0x15,
    0x04,0xC7,0x23,0xC3, 0x18,0x96,0x05,0x9A, 0x07,0x12,0x80,0xE2, 0xEB,0x27,0xB2,0x75,
    0x09,0x83,0x2C,0x1A, 0x1B,0x6E,0x5A,0xA0, 0x52,0x3B,0xD6,0xB3, 0x29,0xE3,0x2F,0x84,
    0x53,0xD1,0x00,0xED, 0x20,0xFC,0xB1,0x5B, 0x6A,0xCB,0xBE,0x39, 0x4A,0x4C,0x58,0xCF,
    0xD0,0xEF,0xAA,0xFB, 0x43,0x4D,0x33,0x85, 0x45,0xF9,0x02,0x7F, 0x50,0x3C,0x9F,0xA8,
    0x51,0xA3,0x40,0x8F, 0x92,0x9D,0x38,0xF5, 0xBC,0xB6,0xDA,0x21, 0x10,0xFF,0xF3,0xD2,
    0xCD,0x0C,0x13,0xEC, 0x5F,0x97,0x44,0x17, 0xC4,0xA7,0x7E,0x3D, 0x64,0x5D,0x19,0x73,
    0x60,0x81,0x4F,0xDC, 0x22,0x2A,0x90,0x88, 0x46,0xEE,0xB8,0x14, 0xDE,0x5E,0x0B,0xDB,
    0xE0,0x32,0x3A,0x0A, 0x49,0x06,0x24,0x5C, 0xC2,0xD3,0xAC,0x62, 0x91,0x95,0xE4,0x79,
    0xE7,0xC8,0x37,0x6D, 0x8D,0xD5,0x4E,0xA9, 0x6C,0x56,0xF4,0xEA, 0x65,0x7A,0xAE,0x08,
    0xBA,0x78,0x25,0x2E, 0x1C,0xA6,0xB4,0xC6, 0xE8,0xDD,0x74,0x1F, 0x4B,0xBD,0x8B,0x8A,
    0x70,0x3E,0xB5,0x66, 0x48,0x03,0xF6,0x0E, 0x61,0x35,0x57,0xB9, 0x86,0xC1,0x1D,0x9E,
    0xE1,0xF8,0x98,0x11, 0x69,0xD9,0x8E,0x94, 0x9B,0x1E,0x87,0xE9, 0xCE,0x55,0x28,0xDF,
    0x8C,0xA1,0x89,0x0D, 0xBF,0xE6,0x42,0x68, 0x41,0x99,0x2D,0x0F, 0xB0,0x54,0xBB,0x16
};

// AES is-box table
static const uint8_t _CryptAes_Isbox[256] = 
{
    0x52,0x09,0x6a,0xd5, 0x30,0x36,0xa5,0x38, 0xbf,0x40,0xa3,0x9e, 0x81,0xf3,0xd7,0xfb,
    0x7c,0xe3,0x39,0x82, 0x9b,0x2f,0xff,0x87, 0x34,0x8e,0x43,0x44, 0xc4,0xde,0xe9,0xcb,
    0x54,0x7b,0x94,0x32, 0xa6,0xc2,0x23,0x3d, 0xee,0x4c,0x95,0x0b, 0x42,0xfa,0xc3,0x4e,
    0x08,0x2e,0xa1,0x66, 0x28,0xd9,0x24,0xb2, 0x76,0x5b,0xa2,0x49, 0x6d,0x8b,0xd1,0x25,
    0x72,0xf8,0xf6,0x64, 0x86,0x68,0x98,0x16, 0xd4,0xa4,0x5c,0xcc, 0x5d,0x65,0xb6,0x92,
    0x6c,0x70,0x48,0x50, 0xfd,0xed,0xb9,0xda, 0x5e,0x15,0x46,0x57, 0xa7,0x8d,0x9d,0x84,
    0x90,0xd8,0xab,0x00, 0x8c,0xbc,0xd3,0x0a, 0xf7,0xe4,0x58,0x05, 0xb8,0xb3,0x45,0x06,
    0xd0,0x2c,0x1e,0x8f, 0xca,0x3f,0x0f,0x02, 0xc1,0xaf,0xbd,0x03, 0x01,0x13,0x8a,0x6b,
    0x3a,0x91,0x11,0x41, 0x4f,0x67,0xdc,0xea, 0x97,0xf2,0xcf,0xce, 0xf0,0xb4,0xe6,0x73,
    0x96,0xac,0x74,0x22, 0xe7,0xad,0x35,0x85, 0xe2,0xf9,0x37,0xe8, 0x1c,0x75,0xdf,0x6e,
    0x47,0xf1,0x1a,0x71, 0x1d,0x29,0xc5,0x89, 0x6f,0xb7,0x62,0x0e, 0xaa,0x18,0xbe,0x1b,
    0xfc,0x56,0x3e,0x4b, 0xc6,0xd2,0x79,0x20, 0x9a,0xdb,0xc0,0xfe, 0x78,0xcd,0x5a,0xf4,
    0x1f,0xdd,0xa8,0x33, 0x88,0x07,0xc7,0x31, 0xb1,0x12,0x10,0x59, 0x27,0x80,0xec,0x5f,
    0x60,0x51,0x7f,0xa9, 0x19,0xb5,0x4a,0x0d, 0x2d,0xe5,0x7a,0x9f, 0x93,0xc9,0x9c,0xef,
    0xa0,0xe0,0x3b,0x4d, 0xae,0x2a,0xf5,0xb0, 0xc8,0xeb,0xbb,0x3c, 0x83,0x53,0x99,0x61,
    0x17,0x2b,0x04,0x7e, 0xba,0x77,0xd6,0x26, 0xe1,0x69,0x14,0x63, 0x55,0x21,0x0c,0x7d
};

// AES Rcon (round constant word) table
static const uint8_t _CryptAes_Rcon[30] =
{
    0x01,0x02,0x04,0x08, 0x10,0x20,0x40,0x80, 0x1b,0x36,0x6c,0xd8, 0xab,0x4d,0x9a,0x2f,
    0x5e,0xbc,0x63,0xc6, 0x97,0x35,0x6a,0xd4, 0xb3,0x7d,0xfa,0xef, 0xc5,0x91
};


/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _CryptAesMul2

    \Description
        Optimized implementation of AES mul-by-two operation x4 using shifts and adds.

    \Input uValue           - four-byte composite value to mul-by-two

    \Output
        uint32_t            - mul-by-two result
    
    \Version 01/20/2011 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _CryptAesMul2(uint32_t uValue)
{
    static const uint32_t _mt = 0x80808080;
    static const uint32_t _mh = 0xfefefefe;
    static const uint32_t _mm = 0x1b1b1b1b;

    uint32_t uTemp = uValue & _mt;
    uTemp = ((uValue + uValue) & _mh) ^ ((uTemp - (uTemp >> 7)) & _mm);
    return(uTemp);
}

/*F********************************************************************************/
/*!
    \Function _CryptAesXtime

    \Description
        AES xtime operation

    \Input uValue           - input state value

    \Output
        uint8_t             - xtime result
    
    \Version 01/20/2011 (jbrookes)
*/
/********************************************************************************F*/
static uint8_t _CryptAesXtime(uint32_t uValue)
{
    uValue = (uValue & 0x80) ? (uValue << 1) ^ 0x1b : uValue << 1;
    return((uint8_t)uValue);
}

/*F********************************************************************************/
/*!
    \Function _CryptAesInit

    \Description
        Initialize crypt state based in input key and initialization vector

    \Input *pAes            - cipher state
    \Input *pKeyBuf         - input key
    \Input iKeyLen          - length of key in bytes (may be 16 for 128 bit AES, or 32 for 256 bit AES)
    \Input *pInitVec        - initialization vector for CBC mode
    \Input uKeyType         - key type (CRYPTAES_KEYTYPE_*)

    \Version 01/20/2011 (jbrookes)
*/
/********************************************************************************F*/
static void _CryptAesInit(CryptAesT *pAes, const uint8_t *pKeyBuf, int32_t iKeyLen, const uint8_t *pInitVec, uint32_t uKeyType)
{
    // init AES key schedule
    CryptAesInitKeySchedule(&pAes->KeySchedule, pKeyBuf, iKeyLen, uKeyType);
    
    // copy the initialization vector
    ds_memcpy(pAes->aInitVec, pInitVec, sizeof(pAes->aInitVec));
}

/*F********************************************************************************/
/*!
    \Function _CryptAesInvMixCol

    \Description
        Perform inverse mix column operation on a key schedule word

    \Input uValue   - key schedule word to perform operation on

    \Output
        uint32_t    - operation result
    
    \Version 01/20/2011 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _CryptAesInvMixCol(uint32_t uValue)
{
    uint32_t a0, a1, a2, a3;

    a0 = _CryptAesMul2(uValue);
    a1 = _CryptAesMul2(a0);
    a2 = _CryptAesMul2(a1);
    a3 = uValue ^ a2;
    a2 = a0 ^ a1 ^ a2;
    a0 ^= a3;
    a1 ^= a3;
    a2 ^= CRYPTAES_ROT3(a0);
    a2 ^= CRYPTAES_ROT2(a1);
    a2 ^= CRYPTAES_ROT1(a3);
    return(a2);
}

/*F********************************************************************************/
/*!
    \Function _CryptAesInvertKey

    \Description
        Invert key schedule, used for decryption

    \Input *pKeySchedule    - key schedule
    
    \Version 01/20/2011 (jbrookes)
*/
/********************************************************************************F*/
static void _CryptAesInvertKey(CryptAesKeyScheduleT *pKeySchedule)
{
    uint32_t uCount, *pState;
    for (uCount = 4, pState = pKeySchedule->aKeySchedule; uCount < (unsigned)pKeySchedule->uNumRounds*4; uCount += 1)
    {
        pState[uCount] = _CryptAesInvMixCol(pState[uCount]);
    }
}

/*F********************************************************************************/
/*!
    \Function _CryptAesEncrypt

    \Description
        Encrypt a single block

    \Input *pKeySchedule - cipher key schedule
    \Input *pData        - [inp/out] data to encrypt, storage for encrypted data
    
    \Version 01/20/2011 (jbrookes)
*/
/********************************************************************************F*/
static void _CryptAesEncrypt(const CryptAesKeyScheduleT *pKeySchedule, uint32_t *pData)
{
    const uint32_t *pState = pKeySchedule->aKeySchedule;
    const uint8_t *pSbox = _CryptAes_Sbox;
    uint32_t uRound, uNumRounds;
    uint32_t uRow, aRowData[4];
    uint32_t a0, a1, a2, a3, t0, t1;
    
    // handle pre-round key addition
    for (uRow = 0; uRow < 4; uRow += 1, pState += 1)
    {
        pData[uRow] ^= *pState;
    }
    
    // encrypt a single block of data
    for (uRound = 0, uNumRounds = pKeySchedule->uNumRounds; uRound < uNumRounds; uRound += 1)
    {
        // ByteSub+ShiftRow
        for (uRow = 0; uRow < 4; uRow += 1)
        {
            a0 = (uint32_t)pSbox[(pData[uRow%4] >> 24) & 0xff];
            a1 = (uint32_t)pSbox[(pData[(uRow+1)%4] >> 16) & 0xff];
            a2 = (uint32_t)pSbox[(pData[(uRow+2)%4] >> 8) & 0xff];
            a3 = (uint32_t)pSbox[(pData[(uRow+3)%4]) & 0xff];
            
            // MixColumn (unless last round)
            if (uRound != (uNumRounds - 1))
            {
                t0 = a0 ^ a1 ^ a2 ^ a3;
                t1 = a0;
                
                a0 ^= t0 ^ _CryptAesXtime(a0 ^ a1);
                a1 ^= t0 ^ _CryptAesXtime(a1 ^ a2);
                a2 ^= t0 ^ _CryptAesXtime(a2 ^ a3);
                a3 ^= t0 ^ _CryptAesXtime(a3 ^ t1);
            }
            
            aRowData[uRow] = ((a0 << 24) | (a1 << 16) | (a2 << 8) | a3);
        }
        
        // KeyAddition
        for (uRow = 0; uRow < 4; uRow += 1, pState += 1)
        {
            pData[uRow] = aRowData[uRow] ^ *pState;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _CryptAesEncryptCBC

    \Description
        CBC encrypt of encrypted data blocks (must be block-sized).

    \Input *pAes        - cipher state
    \Input *pBuffer     - [inp/out] data to encrypt, storage for encrypted data
    \Input iLength      - data length in bytes
    
    \Version 01/20/2011 (jbrookes)
*/
/********************************************************************************F*/
static void _CryptAesEncryptCBC(CryptAesT *pAes, uint8_t *pBuffer, int32_t iLength)
{
    uint32_t s0, s1, s2, s3;
    uint32_t a0, a1, a2, a3;
    uint32_t aBlock[4];

    // read the input vector from bytes into words
    s0 = CRYPTAES_RDWORD(pAes->aInitVec+0);
    s1 = CRYPTAES_RDWORD(pAes->aInitVec+4);
    s2 = CRYPTAES_RDWORD(pAes->aInitVec+8);
    s3 = CRYPTAES_RDWORD(pAes->aInitVec+12);

    // encrypt the data    
    for (iLength -= 16; iLength >= 0; iLength -= 16, pBuffer += 16)
    {
        // read a block of data from bytes into words
        a0 = CRYPTAES_RDWORD(pBuffer+0);
        a1 = CRYPTAES_RDWORD(pBuffer+4);
        a2 = CRYPTAES_RDWORD(pBuffer+8);
        a3 = CRYPTAES_RDWORD(pBuffer+12);
        
        // xor CBC state against data prior to encryption
        aBlock[0] = a0 ^ s0;
        aBlock[1] = a1 ^ s1;
        aBlock[2] = a2 ^ s2;
        aBlock[3] = a3 ^ s3;
        
        // encrypt the block
        _CryptAesEncrypt(&pAes->KeySchedule, aBlock);
        
        // update state
        s0 = aBlock[0];
        s1 = aBlock[1];
        s2 = aBlock[2];
        s3 = aBlock[3];
        
        // write encrypted data to buffer
        CRYPTAES_WRWORD(s0, pBuffer+0);
        CRYPTAES_WRWORD(s1, pBuffer+4);
        CRYPTAES_WRWORD(s2, pBuffer+8);
        CRYPTAES_WRWORD(s3, pBuffer+12);
    }

    // write back updated state
    CRYPTAES_WRWORD(s0, pAes->aInitVec+0);
    CRYPTAES_WRWORD(s1, pAes->aInitVec+4);
    CRYPTAES_WRWORD(s2, pAes->aInitVec+8);
    CRYPTAES_WRWORD(s3, pAes->aInitVec+12);
}

/*F********************************************************************************/
/*!
    \Function _CryptAesDecrypt

    \Description
        Decrypt a single block

    \Input *pKeySchedule - cipher key schedule
    \Input *pData        - [inp/out] data to decrypt, storage for decrypted data
    
    \Version 01/20/2011 (jbrookes)
*/
/********************************************************************************F*/
static void _CryptAesDecrypt(const CryptAesKeyScheduleT *pKeySchedule, uint32_t *pData)
{
    uint32_t uRound, uNumRounds = pKeySchedule->uNumRounds;
    const uint32_t *pState = pKeySchedule->aKeySchedule + ((uNumRounds + 1) * 4);
    const uint8_t *pIsbox = _CryptAes_Isbox;
    uint32_t uRow, aRowData[4];
    uint32_t a0, a1, a2, a3;
    uint32_t xt0, xt1, xt2, xt3, xt4, xt5, xt6;
    
    // handle pre-round key addition
    for (uRow = 4; uRow > 0; uRow -= 1)
    {
        pData[uRow-1] ^= *(--pState);
    }
    
    // decrypt a single block of data
    for (uRound = 0; uRound < uNumRounds; uRound += 1)
    {
        // ByteSub+ShiftRow
        for (uRow = 4; uRow > 0; uRow -= 1)
        {
            a0 = pIsbox[(pData[(uRow+3)%4] >> 24) & 0xff];
            a1 = pIsbox[(pData[(uRow+2)%4] >> 16) & 0xff];
            a2 = pIsbox[(pData[(uRow+1)%4] >> 8) & 0xff];
            a3 = pIsbox[(pData[uRow%4]) & 0xff];
            
            // MixColumn (unless last row)
            if (uRound != (uNumRounds - 1))
            {
                xt0 = _CryptAesXtime(a0 ^ a1);
                xt1 = _CryptAesXtime(a1 ^ a2);
                xt2 = _CryptAesXtime(a2 ^ a3);
                xt3 = _CryptAesXtime(a3 ^ a0);
                xt4 = _CryptAesXtime(xt0 ^ xt1);
                xt5 = _CryptAesXtime(xt1 ^ xt2);
                xt6 = _CryptAesXtime(xt4 ^ xt5);
                
                xt0 ^= a1 ^ a2 ^ a3 ^ xt4 ^ xt6;
                xt1 ^= a0 ^ a2 ^ a3 ^ xt5 ^ xt6;
                xt2 ^= a0 ^ a1 ^ a3 ^ xt4 ^ xt6;
                xt3 ^= a0 ^ a1 ^ a2 ^ xt5 ^ xt6;
                
                aRowData[uRow-1] = (xt0 << 24) | (xt1 << 16) | (xt2 << 8) | xt3;
            }
            else
            {
                aRowData[uRow-1] = (a0 << 24) | (a1 << 16) | (a2 << 8) | a3;
            }
        }
        
        // KeyAddition
        for (uRow = 4; uRow > 0; uRow -= 1)
        {
            pData[uRow-1] = aRowData[uRow-1] ^ *(--pState);
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _CryptAesDecryptCBC

    \Description
        CBC decrypt of encrypted data blocks (must be block-sized).

    \Input *pAes        - cipher state
    \Input *pBuffer     - [inp/out] data to decrypt, storage for decrypted data
    \Input iLength      - data length in bytes
    
    \Version 01/20/2011 (jbrookes)
*/
/********************************************************************************F*/
static void _CryptAesDecryptCBC(CryptAesT *pAes, uint8_t *pBuffer, int32_t iLength)
{
    uint32_t s0, s1, s2, s3;
    uint32_t a0, a1, a2, a3;
    uint32_t t0, t1, t2, t3;
    uint32_t aBlock[4];

    // read the input vector from bytes into words
    s0 = CRYPTAES_RDWORD(pAes->aInitVec+0);
    s1 = CRYPTAES_RDWORD(pAes->aInitVec+4);
    s2 = CRYPTAES_RDWORD(pAes->aInitVec+8);
    s3 = CRYPTAES_RDWORD(pAes->aInitVec+12);

    // encrypt the data    
    for (iLength -= 16; iLength >= 0; iLength -= 16, pBuffer += 16)
    {
        // read a block of data from bytes into words
        a0 = CRYPTAES_RDWORD(pBuffer+0);
        a1 = CRYPTAES_RDWORD(pBuffer+4);
        a2 = CRYPTAES_RDWORD(pBuffer+8);
        a3 = CRYPTAES_RDWORD(pBuffer+12);
        
        // set up for decrypt
        aBlock[0] = a0;
        aBlock[1] = a1;
        aBlock[2] = a2;
        aBlock[3] = a3;
        
        // encrypt the block
        _CryptAesDecrypt(&pAes->KeySchedule, aBlock);
        
        // undo xor against state
        t0 = aBlock[0] ^ s0;
        t1 = aBlock[1] ^ s1;
        t2 = aBlock[2] ^ s2;
        t3 = aBlock[3] ^ s3;
        
        // update state
        s0 = a0;
        s1 = a1;
        s2 = a2;
        s3 = a3;
        
        // write encrypted data to buffer
        CRYPTAES_WRWORD(t0, pBuffer+0);
        CRYPTAES_WRWORD(t1, pBuffer+4);
        CRYPTAES_WRWORD(t2, pBuffer+8);
        CRYPTAES_WRWORD(t3, pBuffer+12);
    }

    // write back updated state
    CRYPTAES_WRWORD(s0, pAes->aInitVec+0);
    CRYPTAES_WRWORD(s1, pAes->aInitVec+4);
    CRYPTAES_WRWORD(s2, pAes->aInitVec+8);
    CRYPTAES_WRWORD(s3, pAes->aInitVec+12);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CryptAesInit

    \Description
        Init the AES cipher with the specified key

    \Input *pAes        - cipher state to initialize
    \Input *pKeyBuf     - pointer to key
    \Input iKeyLen      - key length
    \Input uKeyType     - type of crypt operation (CRYPTAES_KEYTYPE_*)
    \Input *pInitVec    - initialization vector
    
    \Version 01/20/2011 (jbrookes)
*/
/********************************************************************************F*/
void CryptAesInit(CryptAesT *pAes, const uint8_t *pKeyBuf, int32_t iKeyLen, uint32_t uKeyType, const uint8_t *pInitVec)
{
    // clear state
    ds_memclr(pAes, sizeof(*pAes));
    // init state
    _CryptAesInit(pAes, pKeyBuf, iKeyLen, pInitVec, uKeyType);
}

/*F********************************************************************************/
/*!
    \Function CryptAesEncrypt
    
    \Description
        Encrypt data with the AES cipher in CBC mode

    \Input *pAes    - cipher state
    \Input *pBuffer - [inp/out] data to encrypt
    \Input iLength  - length of data to encrypt (must be a multiple of 16)
    
    \Version 01/20/2011 (jbrookes)
*/
/********************************************************************************F*/
void CryptAesEncrypt(CryptAesT *pAes, uint8_t *pBuffer, int32_t iLength)
{
    _CryptAesEncryptCBC(pAes, pBuffer, iLength);
}

/*F********************************************************************************/
/*!
    \Function CryptAesEncryptBlock
    
    \Description
        Encrypt 16 byte block with the AES cipher

    \Input *pKeySchedule    - cipher key schedule
    \Input *pInput          - [inp] data to encrypt
    \Input *pOutput         - [out] encrypted data

    \Notes
        pInput and pOutput may overlap

    \Version 07/15/2014 (jbrookes)
*/
/********************************************************************************F*/
void CryptAesEncryptBlock(CryptAesKeyScheduleT *pKeySchedule, const uint8_t *pInput, uint8_t *pOutput)
{
    uint32_t aBlock[4];

    // read a block of data from bytes into words
    aBlock[0] = CRYPTAES_RDWORD(pInput+0);
    aBlock[1] = CRYPTAES_RDWORD(pInput+4);
    aBlock[2] = CRYPTAES_RDWORD(pInput+8);
    aBlock[3] = CRYPTAES_RDWORD(pInput+12);

    _CryptAesEncrypt(pKeySchedule, aBlock);

    // write encrypted data to buffer
    CRYPTAES_WRWORD(aBlock[0], pOutput+0);
    CRYPTAES_WRWORD(aBlock[1], pOutput+4);
    CRYPTAES_WRWORD(aBlock[2], pOutput+8);
    CRYPTAES_WRWORD(aBlock[3], pOutput+12);
}

/*F********************************************************************************/
/*!
    \Function CryptAesDecrypt
    
    \Description
        Decrypt data with the AES cipher in CBC mode
        
    \Input *pAes    - cipher state
    \Input *pBuffer - [inp/out] data to decrypt
    \Input iLength  - length of data to decrypt (must be a multiple of 16)
    
    \Version 01/20/2011 (jbrookes)
*/
/********************************************************************************F*/
void CryptAesDecrypt(CryptAesT *pAes, uint8_t *pBuffer, int32_t iLength)
{
    _CryptAesDecryptCBC(pAes, pBuffer, iLength);
}

/*F********************************************************************************/
/*!
    \Function CryptAesDecryptBlock
    
    \Description
        Decrypt 16 byte block with the AES cipher

    \Input *pKeySchedule    - cipher key schedule
    \Input *pInput          - [inp] data to encrypt
    \Input *pOutput         - [out] encrypted data

    \Notes
        pInput and pOutput may overlap

    \Version 07/15/2014 (jbrookes)
*/
/********************************************************************************F*/
void CryptAesDecryptBlock(CryptAesKeyScheduleT *pKeySchedule, const uint8_t *pInput, uint8_t *pOutput)
{
    uint32_t aBlock[4];

    // read a block of data from bytes into words
    aBlock[0] = CRYPTAES_RDWORD(pInput+0);
    aBlock[1] = CRYPTAES_RDWORD(pInput+4);
    aBlock[2] = CRYPTAES_RDWORD(pInput+8);
    aBlock[3] = CRYPTAES_RDWORD(pInput+12);

    _CryptAesDecrypt(pKeySchedule, aBlock);

    // write decrypted data to buffer
    CRYPTAES_WRWORD(aBlock[0], pOutput+0);
    CRYPTAES_WRWORD(aBlock[1], pOutput+4);
    CRYPTAES_WRWORD(aBlock[2], pOutput+8);
    CRYPTAES_WRWORD(aBlock[3], pOutput+12);
}

/*F********************************************************************************/
/*!
    \Function CryptAesInitKeySchedule
    
    \Description
        Init AES key schedule

    \Input *pKeySchedule    - cipher key schedule
    \Input *pKeyBuf         - cipher key
    \Input iKeyLen          - cipher key length
    \Input uKeyType         - cipher key type (CRYPTAES_KEYTYPE_*)

    \Version 07/15/2014 (jbrookes)
*/
/********************************************************************************F*/
void CryptAesInitKeySchedule(CryptAesKeyScheduleT *pKeySchedule, const uint8_t *pKeyBuf, int32_t iKeyLen, uint32_t uKeyType) 
{
    uint32_t uNumRounds, uKeyWords, uWord;
    uint32_t uCount, uTemp, uTemp2;
    const uint8_t *pRcon = _CryptAes_Rcon;
    const uint8_t *pSbox = _CryptAes_Sbox;
    uint32_t *pState;
    
    if (iKeyLen == 16)
    {
        uNumRounds = 10;
        uKeyWords = 4;
    }
    else if (iKeyLen == 32)
    {
        uNumRounds = 14;
        uKeyWords = 8;
    }
    else
    {
        NetPrintf(("cryptaes: key length of %d is not supported\n", iKeyLen));
        return;
    }
    
    // save in state
    pKeySchedule->uNumRounds = uNumRounds;
    pKeySchedule->uKeyWords = uKeyWords;
    
    // copy key to key state
    for (uWord = 0, pState = pKeySchedule->aKeySchedule; uWord < uKeyWords; uWord += 1, pKeyBuf += 4)
    {
        pState[uWord] = ((uint32_t)pKeyBuf[0]<<24) | ((uint32_t)pKeyBuf[1]<<16) | ((uint32_t)pKeyBuf[2]<<8) | ((uint32_t)pKeyBuf[3]);
    }
    
    // key expansion
    for (uWord = uKeyWords, uCount = (pKeySchedule->uNumRounds+1) * 4; uWord < uCount; uWord += 1)
    {
        // read key state value
        uTemp = pState[uWord-1];
        
        // key expansion
        if ((uWord % uKeyWords) == 0)
        {
            uTemp2  = (uint32_t)pSbox[uTemp&0xff]<<8;
            uTemp2 |= (uint32_t)pSbox[(uTemp>>8)&0xff]<<16;
            uTemp2 |= (uint32_t)pSbox[(uTemp>>16)&0xff]<<24;
            uTemp2 |= (uint32_t)pSbox[uTemp>>24];
            uTemp = uTemp2 ^ (((uint32_t)*pRcon)<<24);
            pRcon += 1;
        }
        if ((uKeyWords == 8) && ((uWord % uKeyWords) == 4))
        {
            uTemp2  = (uint32_t)pSbox[uTemp&0xff];
            uTemp2 |= (uint32_t)pSbox[(uTemp>>8)&0xff]<<8;
            uTemp2 |= (uint32_t)pSbox[(uTemp>>16)&0xff]<<16;
            uTemp2 |= (uint32_t)pSbox[uTemp>>24]<<24;
            uTemp = uTemp2;
        }
        
        // write back to key schedule
        pState[uWord] = pState[uWord-uKeyWords] ^ uTemp;
    }

    // if decrypt, invert key schedule
    if (uKeyType == CRYPTAES_KEYTYPE_DECRYPT)
    {
        _CryptAesInvertKey(pKeySchedule);
    }
}

