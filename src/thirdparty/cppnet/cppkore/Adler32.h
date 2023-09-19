#pragma once

namespace Hashing
{
// Mark Adler's compact Adler32 hashing algorithm
// Originally from the public domain stb.h header.
    class Adler32
    {
    public:
        static uint32_t ComputeHash(uint32_t adler, const void* ptr, size_t buflen);
    };
}
