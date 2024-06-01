/*H********************************************************************************/
/*!
    \File dirtyjpg.c

    \Description
        Implements JFIF/EXIF image decoding to an output 32bit buffer.  This decoder
        does not support progressive, lossless, or arithmetic options.

    \Notes
        References:
            [1] http://www.w3.org/Graphics/JPEG/jfif3.pdf
            [2] http://www.w3.org/Graphics/JPEG/itu-t81.pdf
            [3] http://class.ece.iastate.edu/ee528/Reading%20material/JPEG_File_Format.pdf
            [4] http://www.bsdg.org/SWAG/GRAPHICS/0143.PAS.html
            [5] http://www.exif.org/Exif2-2.PDF

    \Copyright
        Copyright (c) 2006 Electronic Arts Inc.

    \Version 02/23/2006 (jbrookes) First version, based on gshaefer code
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"

#include "DirtySDK/graph/dirtyjpg.h"

/*** Defines **********************************************************************/

#define DIRTYJPG_DEBUG         (TRUE)
#define DIRTYJPG_VERBOSE       (DIRTYJPG_DEBUG && FALSE)
#define DIRTYJPG_DEBUG_HUFF    (DIRTYJPG_DEBUG && FALSE)
#define DIRTYJPG_DEBUG_IDCT    (DIRTYJPG_DEBUG && FALSE)

#if !DIRTYJPG_DEBUG_IDCT
 #define _PrintMatrix16(_pMatrix, _pTitle) {;}
#endif

#define MCU_SIZE    (8*8*4)         // 8x8 32-bit
#define MCU_MAXSIZE (MCU_SIZE*4*4)  // max size is 4x4 blocks

// LLM constants
#define DCTSIZE 8
#define CONST_BITS 13
#define PASS1_BITS 2
#define FIX_0_298631336  ((int32_t)  2446)    /* FIX(0.298631336) */
#define FIX_0_390180644  ((int32_t)  3196)    /* FIX(0.390180644) */
#define FIX_0_541196100  ((int32_t)  4433)    /* FIX(0.541196100) */
#define FIX_0_765366865  ((int32_t)  6270)    /* FIX(0.765366865) */
#define FIX_0_899976223  ((int32_t)  7373)    /* FIX(0.899976223) */
#define FIX_1_175875602  ((int32_t)  9633)    /* FIX(1.175875602) */
#define FIX_1_501321110  ((int32_t)  12299)    /* FIX(1.501321110) */
#define FIX_1_847759065  ((int32_t)  15137)    /* FIX(1.847759065) */
#define FIX_1_961570560  ((int32_t)  16069)    /* FIX(1.961570560) */
#define FIX_2_053119869  ((int32_t)  16819)    /* FIX(2.053119869) */
#define FIX_2_562915447  ((int32_t)  20995)    /* FIX(2.562915447) */
#define FIX_3_072711026  ((int32_t)  25172)    /* FIX(3.072711026) */

/*** Type Definitions *************************************************************/

//! quantization/matrix
typedef uint16_t QuantTableT[64];

//! huffman decode table head
typedef struct HuffHeadT
{
    uint16_t uShift;        //!< number of shifts to normalize
    uint16_t uMinCode;      //!< min code in this table
    uint16_t uMaxCode;      //!< max code in this table
    uint16_t uOffset;       //!< offset to value/shift data
} HuffHeadT;

//! huffman decode table
typedef struct HuffTableT
{
    HuffHeadT Head[10];     //!< table code range
    uint8_t Code[512];      //!< code lookup
    uint8_t Step[512];      //!< code length lookup
} HuffTableT;

//! component table
typedef struct CompTableT
{
    uint16_t uMcuHorz;      //!< number of 8x8 blocks horizontally in an mcu
    uint16_t uMcuVert;      //!< number of 8x8 blocks vertically in an mcu
    uint16_t uCompHorz;     //!< horizontal sampling factor
    uint16_t uCompVert;     //!< vertical sampling factor
    uint16_t uExpHorz;      //!< horizontal expansion factor
    uint16_t uExpVert;      //!< vertical expansion factor

    uint32_t uStepHorz0;    //!< horizontal step
    uint32_t uStepVert0;    //!< vertical step
    uint32_t uStepHorz1;    //!< horizontal scaled step  
    uint32_t uStepVert1;    //!< vertical scaled stemp
    uint32_t uStepHorz8;    //!< horizontal step*8
    uint32_t uStepVert8;    //!< horizontal step*8
    uint32_t uStepHorzM;    //!< horizontal mcu step
    uint32_t uStepVertM;    //!< vertical mcu step

    uint16_t uQuantNum;     //!< quant table to use

    HuffTableT *pACHuff;    //!< pointer to current AC huffman decode table
    HuffTableT *pDCHuff;    //!< pointer to current DC huffman decode table

    void (*pExpand)(DirtyJpgStateT *pState, struct CompTableT *pCompTable, uint32_t uOutOffset);
    void (*pInvert)(DirtyJpgStateT *pState, struct CompTableT *pCompTable, uint32_t uOutOffset);

    QuantTableT Matrix;     //!< working matrix for decoding this component
    QuantTableT Quant;      //!< current quant table

} CompTableT;

//! module state
struct DirtyJpgStateT
{
    // module memory group
    int32_t iMemGroup;          //!< module mem group id
    void    *pMemGroupUserData; //!< user data associated with mem group
    
    int32_t iLastError;         //!< previous error
    uint32_t uLastOffset;       //!< offset where decoding previously stopped

    uint16_t uVersion;          //!< JFIF file version
    uint16_t uAspectRatio;      //!< image aspect ratio
    uint16_t uAspectHorz;       //!< horizontal aspect ratio
    uint16_t uAspectVert;       //!< vertical aspect ratio

    uint16_t uImageWidth;       //!< width of encoded image
    uint16_t uImageHeight;      //!< height of encoded image

    uint16_t uMcuHorz;          //!< maxumum horizontal MCU, out of all components
    uint16_t uMcuHorzM;         //!< number of MCUs to cover image horizontall
    uint16_t uMcuVert;          //!< maximum vertical MCU, out of all components
    uint16_t uMcuVertM;         //!< number of MCUs to cover image vertically

    uint32_t uBitfield;         //!< current bit buffer
    uint32_t uBitsAvail;        //!< number of bits in bit buffer
    uint32_t uBitsConsumed;     //!< number of bits consumed from bitstream
    uint32_t uBytesRead;        //!< number of bytes read from bitstream

    CompTableT  CompTable[4];   //!< component tables
    QuantTableT QuantTable[16]; //!< quantization tables
    HuffTableT  HuffTable[8];   //!< huffman decode tables
    uint8_t     aMCU[MCU_MAXSIZE];  //!< mcu decode buffer

    const uint8_t *pCompData;   //!< pointer to compressed component data
    
    uint8_t *pImageBuf;         //!< pointer to output image buffer (allocated by caller)
    const uint8_t *pSrcFinal;

    uint32_t uBufWidth;         //!< width of buffer to decode into
    uint32_t uBufHeight;        //!< height of buffer to decode into

    uint8_t bRestart;           //!< if TRUE, a restart marker has been processed
    uint8_t _pad[3];
};

/*** Variables ********************************************************************/

//! helper table to negate a value based on its bitsize
static const uint16_t _ZagAdj_adjust[] =
{
    0x0000, 0x0001, 0x0003, 0x0007,
    0x000f, 0x001f, 0x003f, 0x007f,
    0x00ff, 0x01ff, 0x03ff, 0x07ff,
    0x0fff, 0x1fff, 0x3fff, 0x7fff
};

//! jpeg zig-zag sequence table
static const uint16_t _ZagAdj_zag[] =
{
      0,  1,  8, 16,  9,  2,  3, 10,
     17, 24, 32, 25, 18, 11,  4,  5,
     12, 19, 26, 33, 40, 48, 41, 34,
     27, 20, 13,  6,  7, 14, 21, 28,
     35, 42, 49, 56, 57, 50, 43, 36,
     29, 22, 15, 23, 30, 37, 44, 51,
     58, 59, 52, 45, 38, 31, 39, 46,
     53, 60, 61, 54, 47, 55, 62, 63
};

static const int16_t _Mult07_14h[] =
{
 1462, 1451, 1439, 1428, 1416, 1405, 1394, 1382, 1371, 1359, 1348, 1336, 1325, 1314, 1302, 1291,
 1279, 1268, 1256, 1245, 1234, 1222, 1211, 1199, 1188, 1176, 1165, 1154, 1142, 1131, 1119, 1108,
 1096, 1085, 1074, 1062, 1051, 1039, 1028, 1016, 1005,  994,  982,  971,  959,  948,  936,  925,
  914,  902,  891,  879,  868,  856,  845,  834,  822,  811,  799,  788,  776,  765,  754,  742,
  731,  719,  708,  697,  685,  674,  662,  651,  639,  628,  617,  605,  594,  582,  571,  559,
  548,  537,  525,  514,  502,  491,  479,  468,  457,  445,  434,  422,  411,  399,  388,  377,
  365,  354,  342,  331,  319,  308,  297,  285,  274,  262,  251,  239,  228,  217,  205,  194,
  182,  171,  159,  148,  137,  125,  114,  102,   91,   79,   68,   57,   45,   34,   22,   11,
    0,  -11,  -22,  -34,  -45,  -57,  -68,  -79,  -91, -102, -114, -125, -137, -148, -159, -171,
 -182, -194, -205, -217, -228, -239, -251, -262, -274, -285, -297, -308, -319, -331, -342, -354,
 -365, -377, -388, -399, -411, -422, -434, -445, -457, -468, -479, -491, -502, -514, -525, -537,
 -548, -559, -571, -582, -594, -605, -617, -628, -639, -651, -662, -674, -685, -697, -708, -719,
 -731, -742, -754, -765, -776, -788, -799, -811, -822, -834, -845, -856, -868, -879, -891, -902,
 -914, -925, -936, -948, -959, -971, -982, -994,-1005,-1016,-1028,-1039,-1051,-1062,-1074,-1085,
-1096,-1108,-1119,-1131,-1142,-1154,-1165,-1176,-1188,-1199,-1211,-1222,-1234,-1245,-1256,-1268,
-1279,-1291,-1302,-1314,-1325,-1336,-1348,-1359,-1371,-1382,-1394,-1405,-1416,-1428,-1439,-1451
};

static const int16_t _Mult07_14l[] =
{
 -179, -178, -176, -175, -173, -172, -171, -169, -168, -166, -165, -164, -162, -161, -159, -158,
 -157, -155, -154, -152, -151, -150, -148, -147, -145, -144, -143, -141, -140, -138, -137, -135,
 -134, -133, -131, -130, -128, -127, -126, -124, -123, -121, -120, -119, -117, -116, -114, -113,
 -112, -110, -109, -107, -106, -105, -103, -102, -100,  -99,  -98,  -96,  -95,  -93,  -92,  -91,
  -89,  -88,  -86,  -85,  -84,  -82,  -81,  -79,  -78,  -77,  -75,  -74,  -72,  -71,  -70,  -68,
  -67,  -65,  -64,  -63,  -61,  -60,  -58,  -57,  -56,  -54,  -53,  -51,  -50,  -49,  -47,  -46,
  -44,  -43,  -42,  -40,  -39,  -37,  -36,  -35,  -33,  -32,  -30,  -29,  -28,  -26,  -25,  -23,
  -22,  -21,  -19,  -18,  -16,  -15,  -14,  -12,  -11,   -9,   -8,   -7,   -5,   -4,   -2,   -1,
    0,    1,    2,    4,    5,    7,    8,    9,   11,   12,   14,   15,   16,   18,   19,   21,
   22,   23,   25,   26,   28,   29,   30,   32,   33,   35,   36,   37,   39,   40,   42,   43,
   44,   46,   47,   49,   50,   51,   53,   54,   56,   57,   58,   60,   61,   63,   64,   65,
   67,   68,   70,   71,   72,   74,   75,   77,   78,   79,   81,   82,   84,   85,   86,   88,
   89,   91,   92,   93,   95,   96,   98,   99,  100,  102,  103,  105,  106,  107,  109,  110,
  112,  113,  114,  116,  117,  119,  120,  121,  123,  124,  126,  127,  128,  130,  131,  133,
  134,  135,  137,  138,  140,  141,  143,  144,  145,  147,  148,  150,  151,  152,  154,  155,
  157,  158,  159,  161,  162,  164,  165,  166,  168,  169,  171,  172,  173,  175,  176,  178
};

static const int16_t _Mult17_03h[] =
{
 -226, -225, -223, -221, -219, -217, -216, -214, -212, -210, -209, -207, -205, -203, -202, -200,
 -198, -196, -194, -193, -191, -189, -187, -186, -184, -182, -180, -178, -177, -175, -173, -171,
 -170, -168, -166, -164, -163, -161, -159, -157, -155, -154, -152, -150, -148, -147, -145, -143,
 -141, -139, -138, -136, -134, -132, -131, -129, -127, -125, -124, -122, -120, -118, -116, -115,
 -113, -111, -109, -108, -106, -104, -102, -101,  -99,  -97,  -95,  -93,  -92,  -90,  -88,  -86,
  -85,  -83,  -81,  -79,  -77,  -76,  -74,  -72,  -70,  -69,  -67,  -65,  -63,  -62,  -60,  -58,
  -56,  -54,  -53,  -51,  -49,  -47,  -46,  -44,  -42,  -40,  -38,  -37,  -35,  -33,  -31,  -30,
  -28,  -26,  -24,  -23,  -21,  -19,  -17,  -15,  -14,  -12,  -10,   -8,   -7,   -5,   -3,   -1,
    0,    1,    3,    5,    7,    8,   10,   12,   14,   15,   17,   19,   21,   23,   24,   26,
   28,   30,   31,   33,   35,   37,   38,   40,   42,   44,   46,   47,   49,   51,   53,   54,
   56,   58,   60,   62,   63,   65,   67,   69,   70,   72,   74,   76,   77,   79,   81,   83,
   85,   86,   88,   90,   92,   93,   95,   97,   99,  101,  102,  104,  106,  108,  109,  111,
  113,  115,  116,  118,  120,  122,  124,  125,  127,  129,  131,  132,  134,  136,  138,  139,
  141,  143,  145,  147,  148,  150,  152,  154,  155,  157,  159,  161,  163,  164,  166,  168,
  170,  171,  173,  175,  177,  178,  180,  182,  184,  186,  187,  189,  191,  193,  194,  196,
  198,  200,  202,  203,  205,  207,  209,  210,  212,  214,  216,  217,  219,  221,  223,  225
};

static const int16_t _Mult17_03l[] =
{
  704,  699,  693,  688,  682,  677,  671,  666,  660,  655,  649,  644,  638,  633,  627,  622,
  616,  611,  605,  600,  594,  589,  583,  578,  572,  567,  561,  556,  550,  545,  539,  534,
  528,  523,  517,  512,  506,  501,  495,  490,  484,  479,  473,  468,  462,  457,  451,  446,
  440,  434,  429,  423,  418,  412,  407,  401,  396,  390,  385,  379,  374,  368,  363,  357,
  352,  346,  341,  335,  330,  324,  319,  313,  308,  302,  297,  291,  286,  280,  275,  269,
  264,  258,  253,  247,  242,  236,  231,  225,  220,  214,  209,  203,  198,  192,  187,  181,
  176,  170,  165,  159,  154,  148,  143,  137,  132,  126,  121,  115,  110,  104,   99,   93,
   88,   82,   77,   71,   66,   60,   55,   49,   44,   38,   33,   27,   22,   16,   11,    5,
    0,   -5,  -11,  -16,  -22,  -27,  -33,  -38,  -44,  -49,  -55,  -60,  -66,  -71,  -77,  -82,
  -88,  -93,  -99, -104, -110, -115, -121, -126, -132, -137, -143, -148, -154, -159, -165, -170,
 -176, -181, -187, -192, -198, -203, -209, -214, -220, -225, -231, -236, -242, -247, -253, -258,
 -264, -269, -275, -280, -286, -291, -297, -302, -308, -313, -319, -324, -330, -335, -341, -346,
 -352, -357, -363, -368, -374, -379, -385, -390, -396, -401, -407, -412, -418, -423, -429, -434,
 -440, -446, -451, -457, -462, -468, -473, -479, -484, -490, -495, -501, -506, -512, -517, -523,
 -528, -534, -539, -545, -550, -556, -561, -567, -572, -578, -583, -589, -594, -600, -605, -611,
 -616, -622, -627, -633, -638, -644, -649, -655, -660, -666, -671, -677, -682, -688, -693, -699
};

//! 8-bit table for clamping range [-256...511]->[0...255]
static const uint8_t _Ranger8[] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
    0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
    0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,

    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

//! offset clamp table to zero
static const uint8_t *_pRanger8 = _Ranger8+256;


/*** Private Functions ************************************************************/

#if DIRTYJPG_DEBUG_IDCT
/*F********************************************************************************/
/*!
    \Function _PrintMatrix16

    \Description
        Print 16bit 8x8 matrix to debug output.

    \Input *pMatrix - pointer to matrix to print
    \Input *pTitle  - output title

    \Version 03/09/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _PrintMatrix16(int16_t *pMatrix, const char *pTitle)
{
    int32_t ctr;

    NetPrintf(("%s\n", pTitle));
    for (ctr = 0; ctr < 64; ctr += 8)
    {
        NetPrintf(("%+10d %+10d %+10d %+10d %+10d %+10d %+10d %+10d\n",
            pMatrix[ctr+0], pMatrix[ctr+1], pMatrix[ctr+2], pMatrix[ctr+3],
            pMatrix[ctr+4], pMatrix[ctr+5], pMatrix[ctr+6], pMatrix[ctr+7]));
    }
}
#endif

/*

    Huffman decoding routines

*/

/*F********************************************************************************/
/*!
    \Function _ReadByte
    
    \Description
        Read a byte into the bitbuffer, handling any dle bytes.

    \Input *pState  - state ref
    \Input *pData   - compressed data base pointer
    \Input *pValue  - byte read 

    \Output
        uint32_t    - number of bytes consumed

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _ReadByte(DirtyJpgStateT *pState, const uint8_t *pData, uint8_t *pValue)
{
    uint32_t uValue, uOffset=0;

    // protect against reading past the end of the buffer, in which case we just return a null byte
    if ((pData+uOffset) == pState->pSrcFinal)
    {
        *pValue = 0;
        return(1);
    }

    // read a byte
    *pValue = uValue = pData[uOffset++];

    // eat marker data
    while (uValue == 0xff)
    {
        uValue = pData[uOffset++];
        pState->uBitsConsumed += 8;

        // restart marker?
        if ((uValue >= 0xd0) && (uValue <= 0xd7))
        {
            #if DIRTYJPG_DEBUG_HUFF
            NetPrintf(("dirtyjpg: restart marker %d\n", uValue&0xf));
            #endif
            // remember that there was a restart marker
            pState->bRestart = TRUE;
        }
    }

    return(uOffset);
}

/*F********************************************************************************/
/*!
    \Function _ExtractBits
    
    \Description
        Read bits from bitstream.

    \Input *pState  - state ref
    \Input *pData   - compressed data base pointer
    \Input uBitSize - number of bits to read
    \Input *pSign   - positive if leading bit of extracted data is not set, else negative
    \Input bAdvance - if TRUE, advance the bit offset

    \Output
        uint32_t    - extracted value

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _ExtractBits(DirtyJpgStateT *pState, const uint8_t *pData, uint32_t uBitSize, int32_t *pSign, uint8_t bAdvance)
{
    uint32_t uValue, uBytesRead;
    uint8_t uByte;

    // load data into bit buffer
    while ((pState->uBitsAvail < 16) && (pState->bRestart != TRUE))
    {
        uBytesRead = _ReadByte(pState, pData+pState->uBytesRead, &uByte);
        pState->uBytesRead += uBytesRead;
        pState->uBitfield |= uByte << (24-pState->uBitsAvail);
        pState->uBitsAvail += 8;
    }

    // determine sign of extracted value
    if (pSign)
    {
        *pSign = (pState->uBitfield & 0x80000000) ? -1 : 1;
    }
    
    // take top bits
    uValue = pState->uBitfield >> (32-uBitSize);
    if (bAdvance)
    {
        pState->uBitfield <<= uBitSize;
        pState->uBitsAvail -= uBitSize;
        pState->uBitsConsumed += uBitSize;
    }

    // return extracted value to caller
    return(uValue);    
}

/*F********************************************************************************/
/*!
    \Function _HuffDecode
    
    \Description
        Decode a huffman-encoded value

    \Input *pState      - state ref
    \Input *pHuffTable  - pointer to huffman table to use for decode
    \Input *pData       - compressed data base pointer

    \Output
        uint32_t        - decoded value

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _HuffDecode(DirtyJpgStateT *pState, HuffTableT *pHuffTable, const uint8_t *pData)
{
    HuffHeadT *pHuffHead;
    uint32_t uValue;
    
    // extract 16 bits, but don't advance bit offset
    uValue = _ExtractBits(pState, pData, 16, NULL, FALSE);

    // find table range for this value
    for (pHuffHead = pHuffTable->Head; uValue >= pHuffHead->uMaxCode; pHuffHead += 1)
    {
        if (pHuffHead->uShift == 0)
        {
             NetPrintf(("dirtyjpg: decode error\n"));
            return(0xffffffff);
        }
    }
    #if DIRTYJPG_DECODE_HUFF
    NetPrintf(("dirtyjpg: using range %d\n", pHuffHead-pHuffTable->Head));
    #endif
    
    // subtract mincode to get offset into table
    uValue -= pHuffHead->uMinCode;
    // shift down to discard extra bits
    uValue >>= pHuffHead->uShift;
    // offset by the start of this table into code/step buffer
    uValue += pHuffHead->uOffset;

    // lookup skip value and consume bits
    _ExtractBits(pState, pData, pHuffTable->Step[uValue], NULL, TRUE);

    #if DIRTYJPG_DEBUG_HUFF
    NetPrintf(("_decode: 0x%02x->s%d,c%02x\n", uValue, pHuffTable->Step[uValue], pHuffTable->Code[uValue]));
    #endif

    // lookup code value
    uValue = pHuffTable->Code[uValue];

    // return decoded value to caller
    return(uValue);
}

/*F********************************************************************************/
/*!
    \Function _UnpackComp
    
    \Description
        Unnpack the next sample into a component record's matrix

    \Input *pState      - state ref
    \Input *pCompTable  - pointer to component table
    \Input *pData       - compressed data base pointer

    \Output
        int32_t         - negative=error, else success

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _UnpackComp(DirtyJpgStateT *pState, CompTableT *pCompTable, const uint8_t *pData)
{
    uint16_t *pMatrix = (uint16_t *)&pCompTable->Matrix;
    uint16_t *pQuant = (uint16_t *)&pCompTable->Quant;
    uint32_t uCodeLen, uElemIdx, uValue=0;
    int32_t iSign = 0;

    // decode a token (get code length)
    if ((uCodeLen = _HuffDecode(pState, pCompTable->pDCHuff, pData)) == 0xffffffff)
    {
        return(uCodeLen);
    }
    else if (uCodeLen != 0)
    {
        // get the raw value
        uValue = _ExtractBits(pState, pData, uCodeLen, &iSign, TRUE);
        // if non-negative, adjust to negative value
        if (iSign > 0)
        {
            uValue -= _ZagAdj_adjust[uCodeLen];
        }
        #if DIRTYJPG_DEBUG_HUFF
        NetPrintf(("dc=0x%02x\n", uValue));
        #endif
        // quantify the difference
        uValue *= pQuant[0];
    }

    // accumulate and save the dc coefficient
    uValue += pMatrix[0];
    pMatrix[0] = uValue;

    // clear the rest of the matrix
    ds_memclr(&pMatrix[1], sizeof(pCompTable->Matrix)-sizeof(pMatrix[0]));

    // decode ac coefficients
    for (uElemIdx = 1; uElemIdx < 64; )
    {
        // decode ac code
        if ((uValue = _HuffDecode(pState, pCompTable->pACHuff, pData)) == 0xffffffff)
        {
            return(uValue);
        }
    
        // get vli length
        if ((uCodeLen = uValue & 0xf) == 0)
        {
            // end of block?
            if (uValue != 0xf0)
            {
                return(0);
            }
            // skip 16 (zero skip)
            uElemIdx += 16;
        }
        else
        {
            // skip count
            uValue >>= 4;
            // advance matrix offset (zero skip)
            uElemIdx = (uElemIdx+uValue)&63;
            // get the raw value and advance the bit offset
            uValue = _ExtractBits(pState, pData, uCodeLen, &iSign, TRUE);
            // if non-negative, adjust to negative value
            if (iSign > 0)
            {
                uValue -= _ZagAdj_adjust[uCodeLen];
            }
            #if DIRTYJPG_DEBUG_HUFF
            NetPrintf(("ac=0x%02x\n", uValue));
            #endif
            // quantify the value
            uValue *= pQuant[uElemIdx];
            // store the quantized value, with zag
            pMatrix[_ZagAdj_zag[uElemIdx]] = uValue;
            // increment index
            uElemIdx += 1;
        }
    }

    // return success
    return(0);
}


/*

    Image coefficient expansion routines

*/

/*F********************************************************************************/
/*!
    \Function _Expand0

    \Description
        Expand buffered component scan line data (no expansion).

    \Input *pState      - state ref
    \Input *pCompTable  - pointer to component table
    \Input uOutOffset   - offset in mcu buffer

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _Expand0(DirtyJpgStateT *pState, CompTableT *pCompTable, uint32_t uOutOffset)
{
}

/*F********************************************************************************/
/*!
    \Function _Expand3hv

    \Description
        Expand buffered component scan line data (2x2 special case).

    \Input *pState      - state ref
    \Input *pCompTable  - pointer to component table
    \Input uOutOffset   - offset in mcu buffer

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _Expand3hv(DirtyJpgStateT *pState, CompTableT *pCompTable, uint32_t uOutOffset)
{
    uint32_t uStepV0 = pCompTable->uStepVert0;
    uint32_t uStepVM = pCompTable->uStepVertM;
    uint32_t uStepH0 = pCompTable->uStepHorz0;
    uint8_t *pData = pState->aMCU + uOutOffset;
    uint8_t *pEndRow, *pEndCol;
    uint32_t uData;

    for (pEndRow = pData + uStepVM; pData != pEndRow;)
    {    
        for (pEndCol = pData + uStepV0; pData != pEndCol;)
        {
            uData = *pData;             // get upper left
            pData[uStepV0] = uData;     // save lower-left
            pData += uStepH0;
            pData[0] = uData;           // save upper-right
            pData[uStepV0] = uData;     // save lower-right
            pData += uStepH0;           
        }
        
        // skip extra col
        pData += uStepV0;
    }
}

/*F********************************************************************************/
/*!
    \Function _ExpandAny

    \Description
        Expand buffered component scan line data (general case).

    \Input *pState      - state ref
    \Input *pCompTable  - pointer to component table
    \Input uOutOffset   - offset in mcu buffer

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _ExpandAny(DirtyJpgStateT *pState, CompTableT *pCompTable, uint32_t uOutOffset)
{
    uint32_t uData, uExpHorz, uExpVert, uRptCnt;
    uint32_t uStepV0 = pCompTable->uStepVert0;
    uint32_t uStepVM = pCompTable->uStepVertM;
    uint32_t uStepH0 = pCompTable->uStepHorz0;
    uint8_t *pData, *pEndRow, *pEndCol;

    // get zero-relative vertical expansion factor    
    if ((uExpVert = pCompTable->uExpVert - 1) != 0)
    {
        pData = pState->aMCU + uOutOffset;
        for (pEndCol = pData + uStepV0; pData < pEndCol; pData += pCompTable->uStepHorz1)
        {    
            uint8_t *pTemp = pData;
            for (pEndRow = pData + uStepVM; pData < pEndRow; )
            {
                // get source data and index past it
                uData = *pData;
                pData += uStepV0;
                
                // expand the data
                for (uRptCnt = 0; uRptCnt < uExpVert; uRptCnt += 1, pData += uStepV0)
                {
                    *pData = uData;
                }
            }
            pData = pTemp;
        }
    }
    
    // get zero-relative horizontal expansion factor    
    if ((uExpHorz = pCompTable->uExpHorz - 1) != 0)
    {
        pData = pState->aMCU + uOutOffset;
        for (pEndRow = pData + uStepVM; pData < pEndRow; )
        {
            // get source data and index past it
            uData = *pData;
            pData += uStepH0;
        
            // expand the data
            for (uRptCnt = 0; uRptCnt < uExpHorz; uRptCnt += 1, pData += uStepH0)
            {
                *pData = uData;
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _TransLLM

    \Description
        Perform iDCT and quantify (component record matrix -> mcu buffer).

    \Input *pState      - state ref
    \Input *pCompTable  - pointer to component table
    \Input uOutOffset   - offset in mcu buffer

    \Version 03/03/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _TransLLM(DirtyJpgStateT *pState, CompTableT *pCompTable, uint32_t uOutOffset)
{
    uint32_t uStepH1, uStepV1, uRow;
    uint8_t *pDataDest;
    QuantTableT WorkingMatrix;
    #if DIRTYJPG_DEBUG_IDCT
    QuantTableT WorkingMatrix2;
    #endif
    int16_t *pWorking, *pMatrix;
    const uint8_t *pRanger8 = _pRanger8 + 128;
    int32_t tmp0, tmp1, tmp2, tmp3;
    int32_t tmp10, tmp11, tmp12, tmp13;
    int32_t z1, z2, z3, z4, z5;

    // point to dst data
    if ((pDataDest = pState->aMCU) == NULL)
    {
        return;
    }
    
    // increment to offset
    pDataDest += uOutOffset;
    
    // ref step values
    uStepH1 = pCompTable->uStepHorz1;
    uStepV1 = pCompTable->uStepVert1;

    // debug input
    _PrintMatrix16((int16_t *)&pCompTable->Matrix, "iDCT input matrix");

    // do eight columns
    for (uRow = 0, pWorking=(int16_t*)&WorkingMatrix, pMatrix=(int16_t*)&pCompTable->Matrix; uRow < 8; uRow++)
    {
        /* Due to quantization, we will usually find that many of the input
           coefficients are zero, especially the AC terms.  We can exploit this
           by short-circuiting the IDCT calculation for any column in which all
           the AC terms are zero.  In that case each output is equal to the
           DC coefficient (with scale factor as needed).
           With typical images and quantization tables, half or more of the
           column DCT calculations can be simplified this way. */
        if ((pMatrix[DCTSIZE*1] | pMatrix[DCTSIZE*2] | pMatrix[DCTSIZE*3] |
            pMatrix[DCTSIZE*4] | pMatrix[DCTSIZE*5] | pMatrix[DCTSIZE*6] |
            pMatrix[DCTSIZE*7]) == 0)
        {
            /* AC terms all zero */
            int16_t dcval = (int16_t) (pMatrix[DCTSIZE*0] << PASS1_BITS);
    
            pWorking[DCTSIZE*0] = dcval;
            pWorking[DCTSIZE*1] = dcval;
            pWorking[DCTSIZE*2] = dcval;
            pWorking[DCTSIZE*3] = dcval;
            pWorking[DCTSIZE*4] = dcval;
            pWorking[DCTSIZE*5] = dcval;
            pWorking[DCTSIZE*6] = dcval;
            pWorking[DCTSIZE*7] = dcval;
    
            pMatrix++;            // advance pointers to next column
            pWorking++;
            continue;
        }

        // Even part: reverse the even part of the forward DCT. The rotator is sqrt(2)*c(-6).
        z2 = pMatrix[DCTSIZE*2];
        z3 = pMatrix[DCTSIZE*6];

        z1 = (z2 + z3) * FIX_0_541196100;
        tmp2 = z1 + (z3 * -FIX_1_847759065);
        tmp3 = z1 + (z2 * FIX_0_765366865);

        z2 = pMatrix[DCTSIZE*0];
        z3 = pMatrix[DCTSIZE*4];

        tmp0 = (z2 + z3) << CONST_BITS;
        tmp1 = (z2 - z3) << CONST_BITS;

        tmp10 = tmp0 + tmp3;
        tmp13 = tmp0 - tmp3;
        tmp11 = tmp1 + tmp2;
        tmp12 = tmp1 - tmp2;

        /* Odd part per figure 8; the matrix is unitary and hence its
           transpose is its inverse.  i0..i3 are y7,y5,y3,y1 respectively. */
        tmp0 = pMatrix[DCTSIZE*7];
        tmp1 = pMatrix[DCTSIZE*5];
        tmp2 = pMatrix[DCTSIZE*3];
        tmp3 = pMatrix[DCTSIZE*1];

        z1 = tmp0 + tmp3;
        z2 = tmp1 + tmp2;
        z3 = tmp0 + tmp2;
        z4 = tmp1 + tmp3;
        z5 = (z3 + z4) * FIX_1_175875602; // sqrt(2) * c3

        tmp0 = tmp0 * FIX_0_298631336; // sqrt(2) * (-c1+c3+c5-c7)
        tmp1 = tmp1 * FIX_2_053119869; // sqrt(2) * ( c1+c3-c5+c7)
        tmp2 = tmp2 * FIX_3_072711026; // sqrt(2) * ( c1+c3+c5-c7)
        tmp3 = tmp3 * FIX_1_501321110; // sqrt(2) * ( c1+c3-c5-c7)
        z1 = z1 * -FIX_0_899976223; // sqrt(2) * (c7-c3) 
        z2 = z2 * -FIX_2_562915447; // sqrt(2) * (-c1-c3)
        z3 = z3 * -FIX_1_961570560; // sqrt(2) * (-c3-c5)
        z4 = z4 * -FIX_0_390180644; // sqrt(2) * (c5-c3) 

        z3 += z5;
        z4 += z5;

        tmp0 += z1 + z3;
        tmp1 += z2 + z4;
        tmp2 += z2 + z3;
        tmp3 += z1 + z4;

        // Final output stage: inputs are tmp10..tmp13, tmp0..tmp3
        pWorking[DCTSIZE*0] = (int16_t) ((tmp10 + tmp3) >> (CONST_BITS-PASS1_BITS));
        pWorking[DCTSIZE*7] = (int16_t) ((tmp10 - tmp3) >> (CONST_BITS-PASS1_BITS));
        pWorking[DCTSIZE*1] = (int16_t) ((tmp11 + tmp2) >> (CONST_BITS-PASS1_BITS));
        pWorking[DCTSIZE*6] = (int16_t) ((tmp11 - tmp2) >> (CONST_BITS-PASS1_BITS));
        pWorking[DCTSIZE*2] = (int16_t) ((tmp12 + tmp1) >> (CONST_BITS-PASS1_BITS));
        pWorking[DCTSIZE*5] = (int16_t) ((tmp12 - tmp1) >> (CONST_BITS-PASS1_BITS));
        pWorking[DCTSIZE*3] = (int16_t) ((tmp13 + tmp0) >> (CONST_BITS-PASS1_BITS));
        pWorking[DCTSIZE*4] = (int16_t) ((tmp13 - tmp0) >> (CONST_BITS-PASS1_BITS));

        pMatrix++;      // advance pointers to next column
        pWorking++;
    }

    // debug first pass
    _PrintMatrix16((int16_t *)&WorkingMatrix, "iDCT pass one");

    /* Pass 2: process rows from work array, store into output array.
       Note that we must descale the results by a factor of 8 == 2**3,
       and also undo the PASS1_BITS scaling. */
    for (uRow = 0, pWorking=(int16_t*)&WorkingMatrix; uRow < DCTSIZE; uRow++, pDataDest += uStepV1)
    {
        // Even part: reverse the even part of the forward DCT. The rotator is sqrt(2)*c(-6).
        z2 = (int32_t) pWorking[2];
        z3 = (int32_t) pWorking[6];

        z1 = (z2 + z3) * FIX_0_541196100;
        tmp2 = z1 + (z3 * -FIX_1_847759065);
        tmp3 = z1 + (z2 * FIX_0_765366865);

        tmp0 = ((int32_t) pWorking[0] + (int32_t) pWorking[4]) << CONST_BITS;
        tmp1 = ((int32_t) pWorking[0] - (int32_t) pWorking[4]) << CONST_BITS;

        tmp10 = tmp0 + tmp3;
        tmp13 = tmp0 - tmp3;
        tmp11 = tmp1 + tmp2;
        tmp12 = tmp1 - tmp2;

        /* Odd part per figure 8; the matrix is unitary and hence its
           transpose is its inverse.  i0..i3 are y7,y5,y3,y1 respectively. */
        tmp0 = (int32_t) pWorking[7];
        tmp1 = (int32_t) pWorking[5];
        tmp2 = (int32_t) pWorking[3];
        tmp3 = (int32_t) pWorking[1];

        z1 = tmp0 + tmp3;
        z2 = tmp1 + tmp2;
        z3 = tmp0 + tmp2;
        z4 = tmp1 + tmp3;
        z5 = (z3 + z4) * FIX_1_175875602; // sqrt(2) * c3

        tmp0 = tmp0 * FIX_0_298631336; // sqrt(2) * (-c1+c3+c5-c7)
        tmp1 = tmp1 * FIX_2_053119869; // sqrt(2) * ( c1+c3-c5+c7)
        tmp2 = tmp2 * FIX_3_072711026; // sqrt(2) * ( c1+c3+c5-c7)
        tmp3 = tmp3 * FIX_1_501321110; // sqrt(2) * ( c1+c3-c5-c7)
        z1 = z1 * -FIX_0_899976223; // sqrt(2) * (c7-c3)
        z2 = z2 * -FIX_2_562915447; // sqrt(2) * (-c1-c3)
        z3 = z3 * -FIX_1_961570560; // sqrt(2) * (-c3-c5)
        z4 = z4 * -FIX_0_390180644; // sqrt(2) * (c5-c3)

        z3 += z5;
        z4 += z5;

        tmp0 += z1 + z3;
        tmp1 += z2 + z4;
        tmp2 += z2 + z3;
        tmp3 += z1 + z4;

        #if DIRTYJPG_DEBUG_IDCT
        WorkingMatrix2[0+(uRow*8)] = (((tmp10 + tmp3) >> (CONST_BITS+PASS1_BITS+3)));
        WorkingMatrix2[7+(uRow*8)] = (((tmp10 - tmp3) >> (CONST_BITS+PASS1_BITS+3)));
        WorkingMatrix2[1+(uRow*8)] = (((tmp11 + tmp2) >> (CONST_BITS+PASS1_BITS+3)));
        WorkingMatrix2[6+(uRow*8)] = (((tmp11 - tmp2) >> (CONST_BITS+PASS1_BITS+3)));
        WorkingMatrix2[2+(uRow*8)] = (((tmp12 + tmp1) >> (CONST_BITS+PASS1_BITS+3)));
        WorkingMatrix2[5+(uRow*8)] = (((tmp12 - tmp1) >> (CONST_BITS+PASS1_BITS+3)));
        WorkingMatrix2[3+(uRow*8)] = (((tmp13 + tmp0) >> (CONST_BITS+PASS1_BITS+3)));
        WorkingMatrix2[4+(uRow*8)] = (((tmp13 - tmp0) >> (CONST_BITS+PASS1_BITS+3)));
        #endif

        pDataDest[0*uStepH1] = pRanger8[(((tmp10 + tmp3) >> (CONST_BITS+PASS1_BITS+3)))];
        pDataDest[7*uStepH1] = pRanger8[(((tmp10 - tmp3) >> (CONST_BITS+PASS1_BITS+3)))];
        pDataDest[1*uStepH1] = pRanger8[(((tmp11 + tmp2) >> (CONST_BITS+PASS1_BITS+3)))];
        pDataDest[6*uStepH1] = pRanger8[(((tmp11 - tmp2) >> (CONST_BITS+PASS1_BITS+3)))];
        pDataDest[2*uStepH1] = pRanger8[(((tmp12 + tmp1) >> (CONST_BITS+PASS1_BITS+3)))];
        pDataDest[5*uStepH1] = pRanger8[(((tmp12 - tmp1) >> (CONST_BITS+PASS1_BITS+3)))];
        pDataDest[3*uStepH1] = pRanger8[(((tmp13 + tmp0) >> (CONST_BITS+PASS1_BITS+3)))];
        pDataDest[4*uStepH1] = pRanger8[(((tmp13 - tmp0) >> (CONST_BITS+PASS1_BITS+3)))];

        pWorking += DCTSIZE;        // advance pointer to next row
    }

    // debug second pass
    _PrintMatrix16((int16_t *)&WorkingMatrix2, "iDCT pass two");
}

/*F********************************************************************************/
/*!
    \Function _InitComp

    \Description
        Init component table.

    \Input *pState      - state ref
    \Input *pFrame      - pointer to SOS (start of scan) frame

    \Version 03/01/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _InitComp(DirtyJpgStateT *pState, const uint8_t *pFrame)
{
    CompTableT *pCompTable;
    QuantTableT *pQuantTable;
    uint32_t uExpFunc;
    
    // ref comp table
    pCompTable = &pState->CompTable[pFrame[0]];
    
    // ref huffman tables
    pCompTable->pDCHuff = &pState->HuffTable[0+(pFrame[1]>>4)];
    pCompTable->pACHuff = &pState->HuffTable[4+(pFrame[1]&3)];
    #if DIRTYJPG_DEBUG_HUFF
    NetPrintf(("dirtyjpg: using dc=%d ac=%d\n", pFrame[1]>>4, pFrame[1]&3));
    #endif
    
    // reset differential encoding
    pCompTable->Matrix[0] = 0;
    
    // determine expansion function
    uExpFunc = ((pCompTable->uExpVert - 1) << 2) + (pCompTable->uExpHorz - 1);

    // ref expansion table
    if (uExpFunc == 0)
    {
        pCompTable->pExpand = _Expand0;
    }
    else if (uExpFunc == 0x0101)
    {
        pCompTable->pExpand = _Expand3hv;
    }
    else
    {
        pCompTable->pExpand = _ExpandAny;
    }
    
    // get quantify table pointer
    pQuantTable = &pState->QuantTable[pCompTable->uQuantNum];
    
    // copy the quantify table
    ds_memcpy_s(pCompTable->Quant, sizeof(pCompTable->Quant), pQuantTable, sizeof(*pQuantTable));

    // ref inverse function    
    pCompTable->pInvert = _TransLLM;
}

/*F********************************************************************************/
/*!
    \Function _PutColor

    \Description
        Convert an MCU from YCbCr to ARGB ($$tmp assume color)

    \Input *pState      - state ref
    \Input *pCompTable  - pointer to component table
    \Input uDstHOff     - horizontal mcu offset
    \Input uDstVOff     - vertical mcu offset

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _PutColor(DirtyJpgStateT *pState, CompTableT *pCompTable, uint32_t uDstHOff, uint32_t uDstVOff)
{
    uint32_t uSrcH, uSrcV, uStepSrcH, uStepSrcV, uMaxSrcH, uMaxSrcV;
    uint32_t uDstH, uDstV, uStepDstH, uStepDstV, uMaxDstH, uMaxDstV;
    const uint8_t *pDataSrc;
    uint8_t *pDataDst;
    int16_t Y, Cb_hi, Cb_lo, Cr_hi, Cr_lo;
    uint32_t R, G, B;
    
    // ref source step values
    uStepSrcH = pCompTable->uStepHorz0;
    uMaxSrcH = pCompTable->uStepHorzM;
    uStepSrcV = pCompTable->uStepVert0;
    uMaxSrcV = pCompTable->uStepVertM;

    // ref dest step values
    uStepDstH = 4; // ARGB
    uMaxDstH = uStepDstH * pState->uBufWidth;
    uStepDstV = uStepDstH * pState->uBufWidth;
    uMaxDstV = uStepDstV * pState->uBufHeight;

    // scale mcu step values to get pixel offsets
    uDstHOff *= 8*4*pState->uMcuHorz;
    uDstVOff *= uMaxDstH * 8 * pState->uMcuVert;

    // transform the data
    for (uSrcV = 0, uDstV = uDstVOff; (uSrcV < uMaxSrcV) && (uDstV < uMaxDstV); uSrcV += uStepSrcV, uDstV += uStepDstV)
    {
        for (uSrcH = 0, uDstH = uDstHOff; (uSrcH < uMaxSrcH) && (uDstH < uMaxDstH); uSrcH += uStepSrcH, uDstH += uStepDstH)
        {
            pDataSrc = pState->aMCU + uSrcH + uSrcV;
            pDataDst = pState->pImageBuf + uDstH + uDstV;

            // get YCbCr components
            Y = pDataSrc[0];
            Cb_hi = _Mult17_03h[pDataSrc[1]];   // get 1.772(Cb-128)
            Cb_lo = _Mult17_03l[pDataSrc[1]];   // get -0.34414(Cb-128) 
            Cr_hi = _Mult07_14h[pDataSrc[2]];   // get -0.71414(Cr-128)
            Cr_lo = _Mult07_14l[pDataSrc[2]];   // get 1.402(Cr-128)

            // convert to RGB    
#ifdef __SNC__
            R = *((uint8_t *)(_pRanger8 + Y + Cr_lo));                  // Y + 1.402(Cr-128)
            G = *((uint8_t *)(_pRanger8 + Y + ((Cr_hi + Cb_lo) >> 4))); // Y - 0.71414(Cr-128) - 0.34414(Cb-128)
            B = *((uint8_t *)(_pRanger8 + Y + Cb_hi));                  // Y + 1.772(Cb-128)
#else
            R = _pRanger8[Y + Cr_lo];                  // Y + 1.402(Cr-128)
            G = _pRanger8[Y + ((Cr_hi + Cb_lo) >> 4)]; // Y - 0.71414(Cr-128) - 0.34414(Cb-128)
            B = _pRanger8[Y + Cb_hi];                  // Y + 1.772(Cb-128)
#endif
            // store in output buffer
            pDataDst[0] = 0xff;
            pDataDst[1] = R;
            pDataDst[2] = G;
            pDataDst[3] = B;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _DoRST

    \Description
        Restart decoder after an RST marker was encountered.

    \Input *pState      - state ref

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _DoRST(DirtyJpgStateT *pState)
{
    uint32_t uComp;
    
    // reset bitbuffer
    pState->uBitsConsumed += pState->uBitsAvail;
    pState->uBitsAvail = 0;
    pState->uBitfield = 0;
    
    // reset dc for all components
    for (uComp = 0; uComp < sizeof(pState->CompTable)/sizeof(pState->CompTable[0]); uComp++)
    {
        pState->CompTable[uComp].Matrix[0] = 0;
    }
    
    // done the restart, clear the flag
    pState->bRestart = FALSE;
}

/*F********************************************************************************/
/*!
    \Function _GetMCU

    \Description
        Decode an MCU into MCU buffer.

    \Input *pState      - state ref
    \Input *pFrame      - pointer to SOS (start of scan) component frame data
    \Input uCompCount   - number of components (must be 1 or 3)

    \Output
        uint32_t        - number of bits consumed from the bitstream, or -1

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _GetMCU(DirtyJpgStateT *pState, const uint8_t *pFrame, uint32_t uCompCount)
{
    uint32_t uCompIdx, uCompVert, uCompHorz, uOutOffset;
    CompTableT *pCompTable;

    // unpack and translate all components
    for (uCompIdx = 0; uCompIdx < uCompCount; uCompIdx += 1)
    {
        // ref comp table
        uint32_t uIndex = pFrame[uCompIdx*2];
        if (uIndex > 3)
        {
            continue;
        }
        pCompTable = &pState->CompTable[uIndex];

        // unpack and translate component
        for (uOutOffset = uCompIdx, uCompVert = 0; uCompVert < pCompTable->uCompVert; uCompVert += 1)
        {
            // save start of row
            uint32_t uTmpOffset = uOutOffset;

            // unpack and translate row            
            for (uCompHorz = 0; uCompHorz < pCompTable->uCompHorz; uCompHorz += 1)
            {
                // unpack a component into record matrix
                if (_UnpackComp(pState, pCompTable, pState->pCompData) == 0xffffffff)
                {
                    return(0xffffffff);
                }
    
                // translate the component into mcu buffer
                pCompTable->pInvert(pState, pCompTable, uOutOffset);
            
                // increment to next component
                uOutOffset += pCompTable->uStepHorz8;
            }
        
            // increment to next output row
            uOutOffset = uTmpOffset + pCompTable->uStepVert8;
        }
        
        // expand component
        pCompTable->pExpand(pState, pCompTable, uCompIdx);
    }
    
    // if there was a restart, do it now
    if (pState->bRestart)
    {
        _DoRST(pState);
    }

    // return updated bit offset
    return(pState->uBitsConsumed);
}

/*F********************************************************************************/
/*!
    \Function _ParseDQT

    \Description
        Parse a DQT (quantization) table.

    \Input *pState  - state ref
    \Input *pFrame  - pointer to DQT frame data
    \Input *pFinal  - end of DQT frame

    \Output
        int32_t     - DIRTYJPG_ERR_*

    \Version 02/23/2006 (gschaefer)
*/
/********************************************************************************F*/
static int32_t _ParseDQT(DirtyJpgStateT *pState, const uint8_t *pFrame, const uint8_t *pFinal)
{
    uint32_t uCount;
    uint32_t uIndex;
    uint32_t uMask = 0;
    uint32_t uShift = 0;
    uint32_t uSkip = 1;

    // process tables
    while (pFrame < pFinal)
    {
        // decode index
        uIndex = pFrame[0] & 15;

        // if 16-bit encoded, setup to shift first byte and unmask second
        if (pFrame[0] >= 16)
        {
            uMask = 255;
            uShift = 8;
            uSkip = 2;
        }

        // skip the header
        pFrame += 1;

        // decode and copy the table
        for (uCount = 0; uCount < 64; ++uCount)
        {
            pState->QuantTable[uIndex][uCount] = (pFrame[0]<<uShift)|(pFrame[1]&uMask);
            pFrame += uSkip;
        }
    }

    // return error if buffer size is wrong
    return(pFrame == pFinal ? 0 : DIRTYJPG_ERR_BADDQT);
}

/*F********************************************************************************/
/*!
    \Function _ParseDHT

    \Description
        Parse a DHT (huffman) table.

    \Input *pState  - state ref
    \Input *pFrame  - pointer to DHT frame data
    \Input *pFinal  - end of DHT frame

    \Output
        int32_t     - DIRTYJPG_ERR_*

    \Version 02/23/2006 (gschaefer)
*/
/********************************************************************************F*/
static int32_t _ParseDHT(DirtyJpgStateT *pState, const uint8_t *pFrame, const uint8_t *pFinal)
{
    uint32_t uCount, uIndex, uOffset, uSize;
    uint16_t uCode, uShift;
    const uint8_t *pData;
    HuffTableT *pTable;
    HuffHeadT *pHead;

    while (pFrame < pFinal)
    {
        // ensure all "should be zero bits" are really zero
        if ((pFrame[0] & 0xec) != 0)
        {
            return(DIRTYJPG_ERR_BADDHT);
        }

        // point to the correct table
        pTable = &pState->HuffTable[(pFrame[0]>>2) | (pFrame[0]&3)];

        // offset to value/shift lookup
        uOffset = 0;

        // figure out max code based on first 8 huffman tables (1-8 bits consolidated)
        for (uSize=1, uCode=0; uSize < 8; uSize++)
        {
            uCode += pFrame[uSize];
            uCode += uCode;
        }
        uCode += pFrame[uSize];

        // setup first table entry
        pHead = &pTable->Head[0];
        pHead->uOffset = uOffset;
        // initial table is larger than the rest (based on code size)
        uOffset += uCode;
        pHead->uMaxCode = uCode;
        pHead->uShift = uSize;
        pHead += 1;

        // do remaining tables
        while (++uSize < 17)
        {
            uCode += uCode;
            uCode += pFrame[uSize];
            
            // only add header if it contains data
            if (pFrame[uSize] > 0)
            {
                pHead->uOffset = uOffset;
                // other tables are sized based on exact code counts
                uOffset += pFrame[uSize];
                pHead->uMaxCode = uCode;
                pHead->uShift = uSize;
                pHead += 1;
            }
        }

        // terminate header list
        pHead->uShift = 0;
        pHead->uMaxCode = uCode*2;
        pHead->uOffset = uOffset;       // so we can calc length of final table

        // previous max code
        uCode = 0;
        // start with one-bit codes
        uSize = 1;
        // copy value/shift data to all the tables
        pData = pFrame+17;
        for (pHead = &pTable->Head[0]; pHead->uShift != 0; ++pHead)
        {
            // change from size to shift count
            uShift = pHead->uShift;
            pHead->uShift = 16-uShift;

            // save the previous max code as our min code
            pHead->uMinCode = uCode;
            // get current max code
            uCode = pHead->uMaxCode;
            // right align the bits
            uCode <<= (16-uShift);
            // save the new right-aligned max code
            pHead->uMaxCode = uCode;

            // fill out the data portion (walk current data offset to next data offset)
            for (uCount = pHead[0].uOffset; uCount < pHead[1].uOffset;)
            {
                // copy over the huffman values
                for (uIndex = 0; uIndex < pFrame[uSize]; ++uIndex)
                {
                    // repeat count is variable in table 0 but always 1 for tables 1+
                    uint32_t uRepeat = (1 << uShift) >> uSize;
                    // copy the huffman value into the appropriate slots
                    ds_memset(pTable->Code+uCount, *pData, uRepeat);
                    // copy the corresponding shift counts into the matching slots
                    ds_memset(pTable->Step+uCount, uSize, uRepeat);
                    uCount += uRepeat;
                    pData += 1;
                }
                uSize += 1;
            }
        }

        // move to end of data
        pFrame = pData;
    }

    // make sure all data was processed
    return(pFrame == pFinal ? 0 : DIRTYJPG_ERR_BADDHT);
}

/*F********************************************************************************/
/*!
    \Function _ParseSOF0

    \Description
        Parse a SOF0 (start of frame 0) frame.

    \Input *pState  - state ref
    \Input *pFrame  - pointer to sof0 frame data
    \Input *pFinal  - end of sof0 frame

    \Output
        int32_t     - DIRTYJPG_ERR_*

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ParseSOF0(DirtyJpgStateT *pState, const uint8_t *pFrame, const uint8_t *pFinal)
{
    uint32_t uComp, uCompCount;
    uint32_t uHorzSampFactor, uVertSampFactor;
    CompTableT *pCompTable;
    const uint8_t *pComp;

    // validate 8bits per component
    if (pFrame[0] != 8)
    {
        return(DIRTYJPG_ERR_BITDEPTH);
    }

    pState->uImageHeight = (pFrame[1]<<8)|(pFrame[2]<<0);
    pState->uImageWidth = (pFrame[3]<<8)|(pFrame[4]<<0);
    uCompCount = pFrame[5];

    // only support three components
    if (uCompCount > 3)
    {
        return(DIRTYJPG_ERR_BADSOF0);
    }

    // parse components to find image mcu
    for (pComp = pFrame+6, uComp = 0; uComp < uCompCount; uComp++, pComp += 3)
    {
        uVertSampFactor = pComp[1] & 0xf;
        if (pState->uMcuVert < uVertSampFactor)
        {
            pState->uMcuVert = uVertSampFactor;
        }
        uHorzSampFactor = pComp[1] >> 4;
        if (pState->uMcuHorz < uHorzSampFactor)
        {
            pState->uMcuHorz = uHorzSampFactor;
        }
    }

    // calculate image dimensions in terms of number of mcu blocks
    pState->uMcuHorzM = (pState->uImageWidth + (8*pState->uMcuHorz)-1) / (pState->uMcuHorz*8);
    pState->uMcuVertM = (pState->uImageHeight + (8*pState->uMcuVert)-1) / (pState->uMcuVert*8);

    // parse components to fill in comp table
    for (pComp = pFrame+6, uComp = 0; uComp < uCompCount; uComp++, pComp += 3)
    {
        // skip invalid tabls
        if (pComp[0] > 3)
        {
            NetPrintf(("dirtyjpg: skipping invalid comp table index\n"));
            continue;
        }

        // ref comp table
        pCompTable = &pState->CompTable[pComp[0]];
        
        // init mcu
        pCompTable->uMcuHorz = pState->uMcuHorz;
        pCompTable->uMcuVert = pState->uMcuVert;

        // get sampling factors        
        pCompTable->uCompVert = pComp[1] & 0xf;
        pCompTable->uCompHorz = pComp[1] >> 4;
        
        // compute expansion factors
        pCompTable->uExpHorz = pCompTable->uMcuHorz / pCompTable->uCompHorz;
        pCompTable->uExpVert = pCompTable->uMcuVert / pCompTable->uCompVert;

        // calculate step based on aligned size
        pCompTable->uStepHorz0 = 4;
        pCompTable->uStepVert0 = pCompTable->uMcuHorz * 8 * 4;

        // compute scaled sample step sizes
        pCompTable->uStepHorz1 = pCompTable->uStepHorz0 * pCompTable->uExpHorz;
        pCompTable->uStepVert1 = pCompTable->uStepVert0 * pCompTable->uExpVert;
        
        // compute step * 8
        pCompTable->uStepHorz8 = pCompTable->uStepHorz1 * 8;
        pCompTable->uStepVert8 = pCompTable->uStepVert1 * 8;

        // compute mcu step sizes
        pCompTable->uStepHorzM = (pCompTable->uCompHorz * 8) * pCompTable->uStepHorz1;        
        pCompTable->uStepVertM = (pCompTable->uCompVert * 8) * pCompTable->uStepVert1;        

        // reference quantization table        
        pCompTable->uQuantNum = pComp[2]&0x3;
    }

    // index past comp table
    pFrame = pComp;

    return(pFrame == pFinal ? 0 : DIRTYJPG_ERR_BADSOF0);
}

/*F********************************************************************************/
/*!
    \Function _ParseSOS

    \Description
        Parse the SOS (Start of Scan) frame

    \Input *pState      - state ref
    \Input *pFrame      - pointer to SOS (start of scan) frame data
    \Input *pFinal      - end of frame data

    \Output
        uint8_t *       - pointer past end of frame, or NULL if there was an error

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
static const uint8_t *_ParseSOS(DirtyJpgStateT *pState, const uint8_t *pFrame, const uint8_t *pFinal)
{
    uint32_t uCompIdx, uCompCount, uMcuH, uMcuV;
    const uint8_t *pCompData;

    // point to compressed data
    pCompData = pFrame - 2;
    pCompData = pCompData + ((pCompData[0] << 8) | pCompData[1]);

    // save compressed data
    pState->pCompData = pCompData;

    // get component count and skip to next byte
    uCompCount = *pFrame++;

    // init component tables
    for (uCompIdx = 0; uCompIdx < uCompCount; uCompIdx++)
    {
        _InitComp(pState, &pFrame[uCompIdx*2]);
    }

    // decode mcus and blit them to the output buffer
    for (uMcuV = 0; uMcuV < pState->uMcuVertM; uMcuV++)
    {
        for (uMcuH = 0; uMcuH < pState->uMcuHorzM; uMcuH++)
        {
            // process an MCU
            if (_GetMCU(pState, pFrame, uCompCount) == 0xffffffff)
            {
                return(NULL);
            }

            // put the colors into buffer        
            _PutColor(pState, &pState->CompTable[1], uMcuH, uMcuV);
        }
    }
    
    // return number of bytes consumed
    return(pState->pCompData + (pState->uBitsConsumed+7)/8);
}

/*F********************************************************************************/
/*!
    \Function _DirtyJpgValidate

    \Description
        Validate we have a supported jpeg file

    \Input *pState      - state ref
    \Input *pImage      - pointer to image data
    \Input uLength      - size of image data

    \Output
        int32_t         - DIRTYJPG_ERR_*

    \Version 07/30/2019 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _DirtyJpgValidate(DirtyJpgStateT *pState, const uint8_t *pImage, uint32_t uLength)
{
    // validate SOI header
    if ((uLength < 16) || (pImage[0] != 0xff) || (pImage[1] != 0xd8))
    {
        return(DIRTYJPG_ERR_TOOSHORT);
    }
    // validate app header
    if ((pImage[2] != 0xff) || ((pImage[3] != 0xe0) && (pImage[3] != 0xe1)))
    {
        return(DIRTYJPG_ERR_NOMAGIC);
    }
    // validate format marker
    if (memcmp(pImage+6, "JFIF", 4) && memcmp(pImage+6, "Exif", 4))
    {
        return(DIRTYJPG_ERR_NOFORMAT);
    }
    return(DIRTYJPG_ERR_NONE);
}

/*F********************************************************************************/
/*!
    \Function _DirtyJpgDecodeParse

    \Description
        Parse the JFIF image data.

    \Input *pState      - state ref
    \Input *pImage      - pointer to image data
    \Input uLength      - size of image data
    \Input bIdentify    - identify if this is a JFIF image or not

    \Output
        int32_t         - DIRTYJPG_ERR_*

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _DirtyJpgDecodeParse(DirtyJpgStateT *pState, const uint8_t *pImage, uint32_t uLength, uint8_t bIdentify)
{
    int32_t iError  = 0;
    const uint8_t *pStart=pImage, *pFinal = pImage+uLength;
    uint8_t uType;
    uint32_t uSize;

    // save pointer past end of input
    pState->pSrcFinal = pFinal;

    // decode all the frames
    while (pImage < pFinal)
    {
        // get the frame info
        uType = *pImage++;

        // gobble alignment byte
        if (uType == 0xff)
        {
            continue;
        }

        // get size (no size for eoi)
        uSize = (uType != 0xd9) ? (pImage[0]<<8)|(pImage[1]<<0) : 0;

        // validate the frame size
        if ((pImage+uSize) > pFinal)
        {
            iError = DIRTYJPG_ERR_ENDOFDATA;
            break;
        }

        #if DIRTYJPG_VERBOSE
        NetPrintf(("dirtyjpg: frame=0x%02x size=%d\n", uType, uSize));
        #endif

        // parse the frame
        switch (uType)
        {
            // app0 frame
            case 0xe0:
            {
                // verify its got JFIF header
                if ((pImage[2] == 'J') && (pImage[3] == 'F') && (pImage[4] == 'I') && (pImage[5] == 'F'))
                {
                    // extract the version and make sure we support it
                    pState->uVersion = (pImage[7]<<8)|(pImage[8]>>0);
                    if ((pState->uVersion < 0x0100) || (pState->uVersion > 0x0102))
                    {
                        iError = DIRTYJPG_ERR_BADVERS;
                        break;
                    }

                    // extract & save aspect info
                    pState->uAspectRatio = pImage[9];
                    pState->uAspectHorz = (pImage[10]<<8)|(pImage[11]<<0);
                    pState->uAspectVert = (pImage[12]<<8)|(pImage[13]<<0);
                    
                    // if just doing identification, bail
                    if (bIdentify)
                    {
                        return(DIRTYJPG_ERR_NONE);
                    }
                    break;
                }
            }
            // dqt frame
            case 0xdb:
            {
                iError = _ParseDQT(pState, pImage+2, pImage+uSize);
                break;
            }
            // dht frame
            case 0xc4:
            {
                iError = _ParseDHT(pState, pImage+2, pImage+uSize);
                break;
            }
            // sof0 frame (baseline jpeg)
            case 0xc0:
            {
                iError = _ParseSOF0(pState, pImage+2, pImage+uSize);
                break;
            }
            // sof2 frame (progressive jpeg)
            case 0xc2:
                NetPrintf(("dirtyjpg: warning; SOF2 frame indicates a progressively encoded image, which is not supported\n"));
                iError = DIRTYJPG_ERR_NOSUPPORT;
                break;

            // sos (start of scan) frame
            case 0xda:
            {
                if (pState->pImageBuf != NULL)
                {
                    pImage = _ParseSOS(pState, pImage+2, pImage+uSize);
                    if (pImage == NULL)
                    {
                        iError = DIRTYJPG_ERR_DECODER;
                    }
                }
                else
                {
                    iError = DIRTYJPG_ERR_NOBUFFER;
                    pState->uLastOffset = (uint32_t)(pImage-pStart);
                }
                break;
            }
            // skip "no action" frames
            case 0xd9:      // eoi frame
            case 0xfe:      // com frame
            {
                break;
            }

            default:
                if ((uType & 0xf0) != 0xe0)
                {
                    NetPrintf(("dirtyjpg: ignoring unrecognized frame type 0x%02x\n", uType));
                }
                #if DIRTYJPG_VERBOSE
                else
                {
                    NetPrintf(("dirtyjpg: ignoring application-specific frame type 0x%02x\n", uType));
                }
                #endif
                break;
        }

        // bail if we got an error
        if (iError != 0)
        {
            break;
        }

        // move to next record
        pImage += uSize;
    }

    // save last error
    pState->iLastError = iError;
    return(iError);
}


/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function DirtyJpgCreate

    \Description
        Create the DirtyJpg module state

    \Output
        DirtyJpgStateT *   - pointer to new ref, or NULL if error

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
DirtyJpgStateT *DirtyJpgCreate(void)
{
    DirtyJpgStateT *pState;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate and init module state
    if ((pState = DirtyMemAlloc(sizeof(*pState), DIRTYJPG_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtyjpg: error allocating module state\n"));
        return(NULL);
    }
    ds_memclr(pState, sizeof(*pState));
    pState->iMemGroup = iMemGroup;
    pState->pMemGroupUserData = pMemGroupUserData;

    // return the state pointer
    return(pState);
}

/*F********************************************************************************/
/*!
    \Function DirtyJpgReset

    \Description
        Reset the DirtyJpg module state

    \Input *pState  - pointer to module state

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
void DirtyJpgReset(DirtyJpgStateT *pState)
{
    pState->uLastOffset = 0;

    pState->uBitfield = 0;
    pState->uBitsAvail = 0;
    pState->uBitsConsumed = 0;
    pState->uBytesRead = 0;

    // clear all comp tables
    ds_memclr(pState->CompTable, sizeof(pState->CompTable));

    // clear all quant tables
    ds_memclr(pState->QuantTable, sizeof(pState->QuantTable));

    // clear all huffman tables
    ds_memclr(pState->HuffTable, sizeof(pState->HuffTable));

    // set the image buffer to NULL
    pState->pImageBuf = NULL;

    pState->bRestart = FALSE;
}

/*F********************************************************************************/
/*!
    \Function DirtyJpgDestroy

    \Description
        Destroy the DirtyJpg module

    \Input *pState  - pointer to module state

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
void DirtyJpgDestroy(DirtyJpgStateT *pState)
{
    DirtyMemFree(pState, DIRTYJPG_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function DirtyJpgIdentify

    \Description
        Identify if input image is a JPG (JFIF) image.

    \Input *pState      - jpg module state
    \Input *pImage      - pointer to image data
    \Input uLength      - size of image data

    \Output
        int32_t         - TRUE if a JPG, else FALSE

    \Version 03/09/2006 (jbrookes)
*/
/********************************************************************************F*/
int32_t DirtyJpgIdentify(DirtyJpgStateT *pState, const uint8_t *pImage, uint32_t uLength)
{
    int32_t iResult;

    // make sure it's a supported jpeg format
    if ((iResult = _DirtyJpgValidate(pState, pImage, uLength)) == DIRTYJPG_ERR_NONE)
    {
        // run the parser
        iResult = _DirtyJpgDecodeParse(pState, pImage+2, uLength-2, TRUE);
    }

    // return true if it's a JFIF jpg, else false
    return((iResult == DIRTYJPG_ERR_NOBUFFER) || (iResult == DIRTYJPG_ERR_NONE));
}

/*F********************************************************************************/
/*!
    \Function DirtyJpgDecodeHeader

    \Description
        Parse JPG header.

    \Input *pState  - pointer to module state
    \Input *pJpgHdr - pointer to jpg header
    \Input *pImage  - pointer to image buf
    \Input uLength  - size of input image

    \Output
        int32_t     - DIRTYJPG_ERR_*

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
int32_t DirtyJpgDecodeHeader(DirtyJpgStateT *pState, DirtyJpgHdrT *pJpgHdr, const uint8_t *pImage, uint32_t uLength)
{
    int32_t iResult;

    // reset internal state
    DirtyJpgReset(pState);

    // make sure it's a supported jpeg format
    if ((iResult = _DirtyJpgValidate(pState, pImage, uLength)) < 0)
    {
        return(iResult);
    }

    // run the parser -- we should get a no buffer error
    if ((iResult = _DirtyJpgDecodeParse(pState, pImage+2, uLength-2, FALSE)) == DIRTYJPG_ERR_NOBUFFER)
    {
        // this is expected during header parsing -- save info and return no error
        pJpgHdr->pImageData = pImage;
        pJpgHdr->uLength = uLength;
        pJpgHdr->uWidth = pState->uImageWidth;
        pJpgHdr->uHeight = pState->uImageHeight;
        iResult = 0;
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function DirtyJpgDecodeImage

    \Description
        Parse JPG image.

    \Input *pState      - pointer to module state
    \Input *pJpgHdr     - pointer to jpg header
    \Input *pImageBuf   - pointer to image buf
    \Input iBufWidth    - image buf width
    \Input iBufHeight   - image buf height

    \Output
        int32_t     - DIRTYJPG_ERR_*

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
int32_t DirtyJpgDecodeImage(DirtyJpgStateT *pState, DirtyJpgHdrT *pJpgHdr, uint8_t *pImageBuf, int32_t iBufWidth, int32_t iBufHeight)
{
    int32_t iError;

    // make sure we are in proper state
    if (pState->iLastError != DIRTYJPG_ERR_NOBUFFER)
    {
        return(DIRTYJPG_ERR_BADSTATE);
    }

    // setup the output buffer
    pState->pImageBuf = pImageBuf;
    pState->uBufWidth = (unsigned)iBufWidth;
    pState->uBufHeight = (unsigned)iBufHeight;

    // resume parsing at last spot
    iError = _DirtyJpgDecodeParse(pState, pJpgHdr->pImageData + pState->uLastOffset, pJpgHdr->uLength-pState->uLastOffset, FALSE);
    return(iError);
}
