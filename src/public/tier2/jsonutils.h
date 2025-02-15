//===== Copyright © 1999-2024, Kawe Mazidjatari, All rights reserved. =======//
//
// Purpose: A set of utilities for RapidJSON
//
//===========================================================================//
#ifndef TIER2_JSONUTILS_H
#define TIER2_JSONUTILS_H

//-----------------------------------------------------------------------------
// JSON member enumerations
//-----------------------------------------------------------------------------
enum class JSONFieldType_e
{
    kInvalid = -1,

    kNull = 0,

    kBool,

    kSint32,
    kUint32,

    kSint64,
    kUint64,

    kFloat,
    kLFloat,

    kDouble,
    kLDouble,

    kNumber,

    kString,
    kArray,
    kObject,
};

template <class T>
inline JSONFieldType_e JSON_ExtractType(const T& data)
{
    if (data.IsNull())
        return JSONFieldType_e::kNull;
    if (data.IsBool())
        return JSONFieldType_e::kBool;
    if (data.IsInt())
        return JSONFieldType_e::kSint32;
    if (data.IsUint())
        return JSONFieldType_e::kUint32;
    if (data.IsInt64())
        return JSONFieldType_e::kSint64;
    if (data.IsUint64())
        return JSONFieldType_e::kUint64;
    if (data.IsFloat())
        return JSONFieldType_e::kFloat;
    if (data.IsLosslessFloat())
        return JSONFieldType_e::kLFloat;
    if (data.IsDouble())
        return JSONFieldType_e::kDouble;
    if (data.IsLosslessDouble())
        return JSONFieldType_e::kLDouble;
    if (data.IsNumber())
        return JSONFieldType_e::kNumber;
    if (data.IsString())
        return JSONFieldType_e::kString;
    if (data.IsArray())
        return JSONFieldType_e::kArray;
    if (data.IsObject())
        return JSONFieldType_e::kObject;

    return JSONFieldType_e::kInvalid;
}

//-----------------------------------------------------------------------------
// Purpose: gets the object type as string
//-----------------------------------------------------------------------------
inline const char* JSON_InternalTypeToString(const rapidjson::Type type)
{
    switch (type)
    {
    case rapidjson::kNullType: return "null";
    case rapidjson::kFalseType: case rapidjson::kTrueType: return "bool";
    case rapidjson::kObjectType: return "object";
    case rapidjson::kArrayType: return "array";
    case rapidjson::kStringType: return "string";
    case rapidjson::kNumberType: return "number";
    default: return "unknown";
    }
}

inline const char* JSON_TypeToString(const JSONFieldType_e type)
{
    switch (type)
    {
    case JSONFieldType_e::kNull: return "null";
    case JSONFieldType_e::kBool: return "bool";
    case JSONFieldType_e::kSint32: return "signed int32";
    case JSONFieldType_e::kUint32: return "unsigned int32";
    case JSONFieldType_e::kSint64: return "signed int64";
    case JSONFieldType_e::kUint64: return "unsigned int64";
    case JSONFieldType_e::kFloat: return "float";
    case JSONFieldType_e::kLFloat: return "lossless float";
    case JSONFieldType_e::kDouble: return "double";
    case JSONFieldType_e::kLDouble: return "lossless double";
    case JSONFieldType_e::kNumber: return "number";
    case JSONFieldType_e::kString: return "string";
    case JSONFieldType_e::kArray: return "array";
    case JSONFieldType_e::kObject: return "object";
    default: return "unknown";
    }
}

template <class T>
inline bool JSON_TypeToString(const T& data)
{
    return JSON_TypeToString(JSON_ExtractType(data));
}

//-----------------------------------------------------------------------------
// Purpose: checks if the member's value is of type provided
// NOTE   : the switch case was done intentionally instead of JSON_ExtractType
//          on the object as this function gets used in most accessors that
//          check on types, and this approach is faster as we already know the
//          type beforehand
//-----------------------------------------------------------------------------
template <class T>
inline bool JSON_IsOfType(const T& data, const JSONFieldType_e type)
{
    switch (type)
    {
    case JSONFieldType_e::kNull:
        return data.IsNull();
    case JSONFieldType_e::kBool:
        return data.IsBool();
    case JSONFieldType_e::kSint32:
        return data.IsInt();
    case JSONFieldType_e::kUint32:
        return data.IsUint();
    case JSONFieldType_e::kSint64:
        return data.IsInt64();
    case JSONFieldType_e::kUint64:
        return data.IsUint64();
    case JSONFieldType_e::kFloat:
        return data.IsFloat();
    case JSONFieldType_e::kLFloat:
        return data.IsLosslessFloat();
    case JSONFieldType_e::kDouble:
        return data.IsDouble();
    case JSONFieldType_e::kLDouble:
        return data.IsLosslessDouble();
    case JSONFieldType_e::kNumber:
        return data.IsNumber();
    case JSONFieldType_e::kString:
        return data.IsString();
    case JSONFieldType_e::kArray:
        return data.IsArray();
    case JSONFieldType_e::kObject:
        return data.IsObject();
    default:
        return false;
    }
}

//-----------------------------------------------------------------------------
// Purpose: gets json type for data type at compile time
//-----------------------------------------------------------------------------
template <class T>
inline JSONFieldType_e JSON_GetTypeForType()
{
    if constexpr (std::is_same<T, bool>::value)
        return JSONFieldType_e::kBool;
    else if constexpr (std::is_same<T, int32_t>::value)
        return JSONFieldType_e::kSint32;
    else if constexpr (std::is_same<T, int64_t>::value)
        return JSONFieldType_e::kSint64;
    else if constexpr (std::is_same<T, uint32_t>::value)
        return JSONFieldType_e::kUint32;
    else if constexpr (std::is_same<T, uint64_t>::value)
        return JSONFieldType_e::kUint64;
    else if constexpr (std::is_same<T, float>::value)
        return JSONFieldType_e::kFloat;
    else if constexpr (std::is_same<T, double>::value)
        return JSONFieldType_e::kDouble;
    else if constexpr (std::is_same<T, const char*>::value)
        return JSONFieldType_e::kString;
    else if constexpr (std::is_same<T, std::string>::value)
        return JSONFieldType_e::kString;
    else
        static_assert(std::is_same_v<T, void>, "Cannot classify data type; unsupported.");
}

//-----------------------------------------------------------------------------
// Purpose: checks if the member exists and if its value is of type provided
//-----------------------------------------------------------------------------
template <class T>
inline bool JSON_HasMemberAndIsOfType(const T& data, typename T::StringRefType member, const JSONFieldType_e type)
{
    const T::ConstMemberIterator it = data.FindMember(rapidjson::Value(member));

    if (it != data.MemberEnd())
    {
        return JSON_IsOfType(it->value, type);
    }

    return false;
}

//-----------------------------------------------------------------------------
// Purpose: checks if the member exists, and sets 'out' to its iterator if the
//          aforementioned condition is met
//-----------------------------------------------------------------------------
template <class T>
inline bool JSON_GetIterator(const T& data, typename T::StringRefType member, typename T::ConstMemberIterator& out)
{
    const T::ConstMemberIterator it = data.FindMember(rapidjson::Value(member));

    if (it != data.MemberEnd())
    {
        out = it;
        return true;
    }

    // Not found.
    return false;
}

//-----------------------------------------------------------------------------
// Purpose: checks if the member exists and if its value is of type provided,
//          and sets 'out' to its iterator if all aforementioned conditions
//          are met
//-----------------------------------------------------------------------------
template <class T>
inline bool JSON_GetIterator(const T& data, typename T::StringRefType member,
    const JSONFieldType_e type, typename T::ConstMemberIterator& out)
{
    const T::ConstMemberIterator it = data.FindMember(rapidjson::Value(member));

    if (it != data.MemberEnd())
    {
        if (JSON_IsOfType(it->value, type))
        {
            out = it;
            return true;
        }
    }

    // Not found or didn't match specified type.
    return false;
}

//-----------------------------------------------------------------------------
// Purpose: checks if the member exists and if its value is of specified type,
//          and sets 'out' to its value if all aforementioned conditions
//          are met
//-----------------------------------------------------------------------------
template <class T, class V>
inline bool JSON_GetValue(const T& data, typename T::StringRefType member, const JSONFieldType_e type, V& out)
{
    rapidjson::Document::ConstMemberIterator it;

    if (JSON_GetIterator(data, member, type, it))
    {
        out = it->value.Get<V>();
        return true;
    }

    // Not found or didn't match specified type.
    return false;
}

//-----------------------------------------------------------------------------
// Purpose: checks if the member exists and if its value is of classified type,
//          and sets 'out' to its value if all aforementioned conditions are met
//-----------------------------------------------------------------------------
template <class T, class V>
inline bool JSON_GetValue(const T& data, typename T::StringRefType member, V& out)
{
    rapidjson::Document::ConstMemberIterator it;

    if (JSON_GetIterator(data, member, JSON_GetTypeForType<V>(), it))
    {
        out = it->value.Get<V>();
        return true;
    }

    // Not found or didn't match classified type.
    return false;
}
template <class T>
inline bool JSON_GetValue(const T& data, typename T::StringRefType member, std::string& out)
{
    const char* stringVal;

    if (JSON_GetValue(data, member, JSONFieldType_e::kString, stringVal))
    {
        out = stringVal;
        return true;
    }

    return false;
}

//-----------------------------------------------------------------------------
// Purpose: checks if the member exists and if its value is of classified type,
//          and returns the value if all aforementioned conditions are met.
//          else the provided default gets returned
//-----------------------------------------------------------------------------
template <class T, class V>
inline V JSON_GetValueOrDefault(const T& data, typename T::StringRefType member, const V def)
{
    V val;

    if (JSON_GetValue(data, member, JSON_GetTypeForType<V>(), val))
    {
        return val;
    }

    return def;
}

template <class V>
inline bool JSON_StringToNumber(const char* const str, const size_t len, V& num)
{
    const char* const end = &str[len];
    boost::charconv::from_chars_result result;

    if constexpr ((std::is_same<V, int32_t>::value) || (std::is_same<V, int64_t>::value) ||
        (std::is_same<V, uint32_t>::value) || (std::is_same<V, uint64_t>::value))
    {
        int ofs = 0;
        int base = 10;

        // Note: char_conv doesn't auto detect the base, and it expects the
        // numeric string without the "0x" (base16) or "0b" (base8) prefix.
        if (len > 1)
        {
            if (str[0] == '0')
            {
                if (str[1] == 'x' || str[1] == 'X')
                {
                    ofs = 2; // Hexadecimal, skip "0x".
                    base = 16;
                }
                else if (str[1] == 'b' || str[1] == 'B')
                {
                    ofs = 2; // Binary, skip "0b".
                    base = 2;
                }
                else // Octal doesn't need skipping.
                    base = 8;
            }
        }

        result = boost::charconv::from_chars(&str[ofs], end, num, base);
    }
    else if constexpr ((std::is_same<V, float>::value) || (std::is_same<V, double>::value))
    {
        result = boost::charconv::from_chars(str, end, num, boost::charconv::chars_format::general);
    }
    else
        static_assert(std::is_same_v<V, void>, "Cannot classify numeric type; unsupported.");

    return (result.ptr == end) && (result.ec == std::errc());
}

//-----------------------------------------------------------------------------
// Purpose: parses json number out of provided integer, float or string. string
//          can be a hex (0x<num>), octal (0<num>) or decimal (<num>)
//-----------------------------------------------------------------------------
template <class T, class V>
inline bool JSON_ParseNumber(const T& data, V& num)
{
    if (JSON_IsOfType(data, JSONFieldType_e::kNumber))
    {
        num = data.Get<V>();
        return true;
    }
    else if (JSON_IsOfType(data, JSONFieldType_e::kString))
    {
        return JSON_StringToNumber(data.GetString(), data.GetStringLength(), num);
    }

    return false;
}
template <class T, class V>
inline bool JSON_ParseNumber(const T& data, typename T::StringRefType member, V& num)
{
    rapidjson::Document::ConstMemberIterator it;

    if (JSON_GetIterator(data, member, it))
    {
        return JSON_ParseNumber(it->value, num);
    }

    return false;
}

//-----------------------------------------------------------------------------
// Purpose: checks if the member exists and if its value is of classified type,
//          and returns the number if all aforementioned conditions are met.
//          else the provided default gets returned
//-----------------------------------------------------------------------------
template <class T, class V>
inline V JSON_GetNumberOrDefault(const T& data, typename T::StringRefType member, V def)
{
    V num;

    if (JSON_ParseNumber(data, member, num))
    {
        return num;
    }

    return def;
}

void JSON_DocumentToBufferDeserialize(const rapidjson::Document& document, rapidjson::StringBuffer& buffer, unsigned int indent = 4);

#endif // TIER2_JSONUTILS_H
