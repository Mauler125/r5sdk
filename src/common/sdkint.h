//============================================================================//
// 
// Purpose: portable integral data type definitions
// 
//============================================================================//
#ifndef COMMON_SDKINT_H
#define COMMON_SDKINT_H

//-----------------------------------------------------------------------------
typedef           char     int8;
typedef    signed char     sint8;
typedef  unsigned char     uint8;

typedef           int8     i8;
typedef          sint8     s8;
typedef          uint8     u8;

typedef          uint8     byte;
typedef          uint8     uchar;
//-----------------------------------------------------------------------------
typedef          short     int16;
typedef   signed short     sint16;
typedef unsigned short     uint16;

typedef          int16     i16;
typedef         sint16     s16;
typedef         uint16     u16;

typedef         uint16     ushort;
//-----------------------------------------------------------------------------
typedef            int     int32;
typedef     signed int     sint32;
typedef   unsigned int     uint32;

typedef          int32     i32;
typedef         sint32     s32;
typedef         uint32     u32;

typedef         uint32     uint;
typedef         uint32     ulong;
//-----------------------------------------------------------------------------
typedef          long long int64;
typedef   signed long long sint64;
typedef unsigned long long uint64;

typedef          int64     i64;
typedef         sint64     s64;
typedef         uint64     u64;

typedef         uint64     ulonglong;
//-----------------------------------------------------------------------------
typedef          float     float32;
typedef         double     float64;

typedef        float32     f32;
typedef        float32     f64;
//-----------------------------------------------------------------------------
// 8-bit <--> 64-bit wide boolean type
typedef           int8     b8;
typedef          int16     b16;
typedef          int32     b32;
typedef          int64     b64;
//-----------------------------------------------------------------------------
// intp is an integer that can accommodate a pointer
// (ie, sizeof(intp) >= sizeof(int) && sizeof(intp) >= sizeof(void *)
typedef     intptr_t      intp;
typedef    uintptr_t      uintp;
//-----------------------------------------------------------------------------
// Signed size type
typedef std::make_signed_t<std::size_t> ssize_t;

//-----------------------------------------------------------------------------
#endif // COMMON_SDKINT_H
