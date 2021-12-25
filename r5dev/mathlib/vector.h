#pragma once

typedef float vec_t;
typedef float vec3_t[3];

/*-----------------------------------------------------------------------------
 * _vector.h
 *-----------------------------------------------------------------------------*/

class Vector3 // TODO [ AMOS ]: Reverse class
{
public:
    // Members
    vec_t x, // 0x0000
          y, // 0x0004
          z; // 0x0008
};

class QAngle // TODO [ AMOS ]: Reverse class
{
public:
    // Members
    vec_t x, // 0x0000
          y, // 0x0004
          z; // 0x0008
};
