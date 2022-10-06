#pragma once

#include "Types.hpp"

#include <sstream>

template<typename T>
constexpr String ToString(T value, int modifiers = 0) {
    std::stringstream stream = std::stringstream();
    stream.flags(modifiers);
    stream << value;
    String string;
    stream >> string;
    return string;
}

template<>
constexpr String ToString<Int8>(Int8 value, int modifiers) {
    return ToString<Int16>(static_cast<Int16>(value), modifiers);
}

template<>
constexpr String ToString<UInt8>(UInt8 value, int modifiers) {
    return ToString<UInt16>(static_cast<UInt16>(value), modifiers);
}

/// <summary>
/// <para>Possible exceptions:</para>
/// <para>(Int8)1: Failed to parse invalid value</para>
/// </summary>
template<typename T>
constexpr T FromString(String string, int modifiers = 0) {
    std::stringstream stream = std::stringstream();
    stream << string;
    T value;
    stream.flags(modifiers);
    stream >> value;

    if(stream.bad() || stream.fail()) {
        throw (Int8)1;
    }

    return value;
}

/// <summary>
/// <para>Possible exceptions:</para>
/// <para>(Int8)1: Failed to parse invalid value</para>
/// </summary>
template<>
constexpr Int8 FromString<Int8>(String string, int modifiers) {
    return static_cast<Int8>(FromString<Int16>(string, modifiers));
}

/// <summary>
/// <para>Possible exceptions:</para>
/// <para>(Int8)1: Failed to parse invalid value</para>
/// </summary>
template<>
constexpr UInt8 FromString<UInt8>(String string, int modifiers) {
    return static_cast<UInt8>(FromString<UInt16>(string, modifiers));
}
