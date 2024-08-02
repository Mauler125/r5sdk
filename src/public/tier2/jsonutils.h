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
    kNull = 0,
    kObject,

    kBool,
    kNumber,

    kSint32,
    kUint32,

    kSint64,
    kUint64,

    kFloat,
    kLFloat,

    kDouble,
    kLDouble,

    kString,
    kArray
};

//-----------------------------------------------------------------------------
// Purpose: checks if the member's value is of type provided
//-----------------------------------------------------------------------------
template <class T>
inline bool JSON_IsOfType(const T& data, const JSONFieldType_e type)
{
    switch (type)
    {
    case JSONFieldType_e::kNull:
        return data.IsNull();
    case JSONFieldType_e::kObject:
        return data.IsObject();
    case JSONFieldType_e::kBool:
        return data.IsBool();
    case JSONFieldType_e::kNumber:
        return data.IsNumber();
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
    case JSONFieldType_e::kString:
        return data.IsString();
    case JSONFieldType_e::kArray:
        return data.IsArray();
    default:
        return false;
    }
}

//-----------------------------------------------------------------------------
// Purpose: checks if the member exists and if its value is of type provided
//-----------------------------------------------------------------------------
template <class T>
inline bool JSON_HasMemberAndIsOfType(const T& data, const char* const member, const JSONFieldType_e type)
{
    const T::ConstMemberIterator it = data.FindMember(member);

    if (it != data.MemberEnd())
    {
        return JSON_IsOfType(it->value, type);
    }

    return false;
}

//-----------------------------------------------------------------------------
// Purpose: checks if the member exists and if its value is of type provided,
// and sets 'out' to its value if all aforementioned conditions are met
//-----------------------------------------------------------------------------
template <class T, class V>
inline bool JSON_GetValue(const T& data, const char* const member, const JSONFieldType_e type, V& out)
{
    const T::ConstMemberIterator it = data.FindMember(member);

    if (it != data.MemberEnd())
    {
        const rapidjson::Value& val = it->value;

        if (JSON_IsOfType(val, type))
        {
            out = val.Get<V>();
            return true;
        }
    }

    // Not found or didn't match specified type.
    return false;
}
template <class T>
inline bool JSON_GetValue(const T& data, const char* const member, const JSONFieldType_e type, std::string& out)
{
    const char* stringVal = nullptr;

    if (JSON_GetValue(data, member, type, stringVal))
    {
        out = stringVal;
        return true;
    }

    return false;
}

//-----------------------------------------------------------------------------
// Purpose: checks if the member exists and if its value is of type provided,
// and sets 'out' to its iterator if all aforementioned conditions are met
//-----------------------------------------------------------------------------
template <class T>
inline bool JSON_GetIterator(const T& data, const char* const member,
    const JSONFieldType_e type, typename T::ConstMemberIterator& out)
{
    const T::ConstMemberIterator it = data.FindMember(member);

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

void JSON_DocumentToBufferDeserialize(const rapidjson::Document& document, rapidjson::StringBuffer& buffer, unsigned int indent = 4);

#endif // TIER2_JSONUTILS_H
