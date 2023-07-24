#pragma once
#include <string>
#include <array>
#include <cstdarg>

// https://stackoverflow.com/a/34981037

#define BEGIN_NAMESPACE( x ) namespace x {
#define END_NAMESPACE }

BEGIN_NAMESPACE(XorCompileTime)

constexpr auto time = __TIME__;
constexpr auto seed = static_cast<int>(time[7]) + static_cast<int>(time[6]) * 10 + static_cast<int>(time[4]) * 60 + static_cast<int>(time[3]) * 600 + static_cast<int>(time[1]) * 3600 + static_cast<int>(time[0]) * 36000;

// 1988, Stephen Park and Keith Miller
// "Random Number Generators: Good Ones Are Hard To Find", considered as "minimal standard"
// Park-Miller 31 bit pseudo-random number generator, implemented with G. Carta's optimisation:
// with 32-bit math and without division

template < int N >
struct CRandomGenerator
{
private:
    static constexpr unsigned a = 16807; // 7^5
    static constexpr unsigned m = 2147483647; // 2^31 - 1

    static constexpr unsigned s = CRandomGenerator< N - 1 >::value;
    static constexpr unsigned lo = a * (s & 0xFFFF); // Multiply lower 16 bits by 16807
    static constexpr unsigned hi = a * (s >> 16); // Multiply higher 16 bits by 16807
    static constexpr unsigned lo2 = lo + ((hi & 0x7FFF) << 16); // Combine lower 15 bits of hi with lo's upper bits
    static constexpr unsigned hi2 = hi >> 15; // Discard lower 15 bits of hi
    static constexpr unsigned lo3 = lo2 + hi;

public:
    static constexpr unsigned max = m;
    static constexpr unsigned value = lo3 > m ? lo3 - m : lo3;
};

template <>
struct CRandomGenerator< 0 >
{
    static constexpr unsigned value = seed;
};

template < int N, int M >
struct CRandomInt
{
    static constexpr auto value = CRandomGenerator< N + 1 >::value % M;
};

template < int N >
struct CRandomChar
{
    static const char value = static_cast<char>(1 + CRandomInt< N, 0x7F - 1 >::value);
};

template < size_t N, int K, typename Char >
struct CXorString
{
private:
    const char _key;
    std::array< Char, N + 1 > _encrypted;

    constexpr Char Encrypt(Char c) const
    {
        return c ^ _key;
    }

    Char Decrypt(Char c) const
    {
        return c ^ _key;
    }

public:
    template < size_t... Is >
    constexpr __forceinline CXorString(const Char* str, std::index_sequence< Is... >) : _key(CRandomChar< K >::value), _encrypted{ Encrypt(str[Is])... }
    {
    }

    __forceinline decltype(auto) Decrypt(void)
    {
        for (size_t i = 0; i < N; ++i) {
            _encrypted[i] = Decrypt(_encrypted[i]);
        }
        _encrypted[N] = '\0';
        return _encrypted.data();
    }
};

//--------------------------------------------------------------------------------
//-- Note: XorStr will __NOT__ work directly with functions like printf.
//         To work with them you need a wrapper function that takes a const char*
//         as parameter and passes it to printf and alike.
//
//         The Microsoft Compiler/Linker is not working correctly with variadic 
//         templates!
//--------------------------------------------------------------------------------

//Wrapper functions to work in all functions below
#define XorStr( s ) []{ constexpr XorCompileTime::CXorString< sizeof(s)/sizeof(char) - 1, __COUNTER__, char > expr( s, std::make_index_sequence< sizeof(s)/sizeof(char) - 1>() ); return expr; }().Decrypt()
#define XorStrW( s ) []{ constexpr XorCompileTime::CXorString< sizeof(s)/sizeof(wchar_t) - 1, __COUNTER__, wchar_t > expr( s, std::make_index_sequence< sizeof(s)/sizeof(wchar_t) - 1>() ); return expr; }().Decrypt()

END_NAMESPACE