#pragma once

#include <cstdint>
#include <math.h>
#include <string>

using Boolean = bool;
using Int8 = int8_t;
using Int16 = int16_t;
using Int32 = int32_t;
using Int64 = int64_t;
using UInt8 = uint8_t;
using UInt16 = uint16_t;
using UInt32 = uint32_t;
using UInt64 = uint64_t;
using Float32 = float_t;
using Float64 = double_t;
using SizeT = size_t;
using String = std::string;

template<typename T>
char const* GetTypeName() = delete;

template<>
char const* GetTypeName<Int8>() { return "Int8"; }

template<>
char const* GetTypeName<Int16>() { return "Int16"; }

template<>
char const* GetTypeName<Int32>() { return "Int32"; }

template<>
char const* GetTypeName<Int64>() { return "Int64"; }

template<>
char const* GetTypeName<UInt8>() { return "UInt8"; }

template<>
char const* GetTypeName<UInt16>() { return "UInt16"; }

template<>
char const* GetTypeName<UInt32>() { return "UInt32"; }

template<>
char const* GetTypeName<UInt64>() { return "UInt64"; }

template<>
char const* GetTypeName<Float32>() { return "Float32"; }

template<>
char const* GetTypeName<Float64>() { return "Float64"; }

template<typename T>
SizeT GetBitSize() = delete;

template<>
SizeT GetBitSize<Int8>() { return 8; }

template<>
SizeT GetBitSize<Int16>() { return 16; }

template<>
SizeT GetBitSize<Int32>() { return 32; }

template<>
SizeT GetBitSize<Int64>() { return 64; }

template<>
SizeT GetBitSize<UInt8>() { return 8; }

template<>
SizeT GetBitSize<UInt16>() { return 16; }

template<>
SizeT GetBitSize<UInt32>() { return 32; }

template<>
SizeT GetBitSize<UInt64>() { return 64; }

template<>
SizeT GetBitSize<Float32>() { return 32; }

template<>
SizeT GetBitSize<Float64>() { return 64; }
