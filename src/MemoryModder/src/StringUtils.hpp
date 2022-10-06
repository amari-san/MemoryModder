#pragma once

#include "Types.hpp"
#include <locale>

constexpr String StringRepeat(String const& string, SizeT count) {
    SizeT sizeElement = string.length();
    SizeT size = sizeElement * count;
    char* buf = new char[size + 1];
    for(SizeT i = 0; i < size; ++i) {
        buf[i] = string[i % sizeElement];
    }
    buf[size] = '\0';
    String out = String(buf);
    delete[] buf;
    return out;
}

constexpr SizeT GetStringMaxLineWidth(String const& string) {
    SizeT size = 0;
    SizeT count = 0;
    for(SizeT i = 0, s = string.length(); i < s; ++i) {
        if(string[i] == '\n') {
            size = max(size, count);
            count = 0;
        }
        else {
            ++count;
        }
    }
    // End of string
    size = max(size, count);
    return size;
}

constexpr char ToLowerAscii(char c) {
    if((c >= 'A') && (c <= 'Z')) {
        return c + ('a' - 'A');
    }
    return c;
}

constexpr char ToUpperAscii(char c) {
    if((c >= 'A') && (c <= 'Z')) {
        return c - ('a' - 'A');
    }
    return c;
}

constexpr String ToLowerAscii(String const& string) {
    SizeT size = string.length() + 1;
    char* buf = new char[size];
    for(SizeT i = 0; i < size; i++) {
        buf[i] = ToLowerAscii(string[i]);
    }
    String out = String(buf);
    delete[] buf;
    return out;
}

constexpr String ToUpperAscii(String const& string) {
    SizeT size = string.length() + 1;
    char* buf = new char[size];
    for(SizeT i = 0; i < size; i++) {
        buf[i] = ToUpperAscii(string[i]);
    }
    String out = String(buf);
    delete[] buf;
    return out;
}
