#pragma once

class adler32
{
public:
    static uint32_t update(uint32_t adler, const void* ptr, size_t buf_len);
};
