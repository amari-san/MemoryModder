/*
    > Library file for MemoryModder
*/

// TODO: Research into DLL injection
// TODO: Use base addresses???
// TODO: Minimize ReadProcessMemory calls when filtering by getting all chunks and then only using the needed values to compare
// ^ Performance test proved that ReadProcessMemory was taking up 99% and comparison was 1%

#pragma once

#include <vector>
#include <algorithm>

#include <Windows.h>
#include <Psapi.h>
#include <WtsApi32.h>

#include "Types.hpp"

struct Process {
public:
    Process(String const& name, DWORD const id) {
        _name = name;
        _id = id;
    }

    String GetName() const {
        return _name;
    }

    DWORD GetId() const {
        return _id;
    }

private:
    String _name;
    DWORD _id;
};

// TODO: Don't use WTS to get all processes.
std::vector<Process> GetAllProcesses() {
    std::vector<Process> processes = std::vector<Process>();

    WTS_PROCESS_INFOA* pWPIs = NULL;
    DWORD dwProcCount = 0;
    if(WTSEnumerateProcessesA(WTS_CURRENT_SERVER_HANDLE, NULL, 1, &pWPIs, &dwProcCount)) {
        // Go through all processes retrieved
        for(DWORD i = 0; i < dwProcCount; ++i) {
            WTS_PROCESS_INFOA& pWPI = pWPIs[i];

            processes.push_back(Process(pWPI.pProcessName, pWPI.ProcessId));
        }
    }

    // Free memory
    if(pWPIs != NULL) {
        WTSFreeMemory(pWPIs);
        pWPIs = NULL;
    }

    return processes;
}

DWORD_PTR GetProcessBaseAddress(HANDLE processHandle) {
    DWORD_PTR baseAddress = 0;

    HMODULE* moduleArray;
    LPBYTE moduleArrayBytes;
    DWORD bytesRequired;

    if(EnumProcessModules(processHandle, NULL, 0, &bytesRequired)) {
        if(bytesRequired) {
            moduleArrayBytes = (LPBYTE)LocalAlloc(LPTR, bytesRequired);

            if(moduleArrayBytes) {
                unsigned int moduleCount;

                moduleCount = bytesRequired / sizeof(HMODULE);
                moduleArray = (HMODULE*)moduleArrayBytes;

                if(EnumProcessModules(processHandle, moduleArray, bytesRequired, &bytesRequired)) {
                    baseAddress = (DWORD_PTR)moduleArray[0];
                }

                LocalFree(moduleArrayBytes);
            }
        }
    }

    return baseAddress;
}

enum struct MemoryComparison : Int8 {
    Equals = 0,
    NotEquals = 1,
    LessThan = 2,
    GreaterThan = 3,
    LessThanEquals = 4,
    GreaterThanEquals = 5
};

template<typename T>
struct MemoryRegion {
public:
    MemoryRegion(SizeT start, SizeT size) {
        _start = start;
        _size = size;
    }

    inline SizeT GetStart() const noexcept {
        return _start;
    }

    inline void SetStart(SizeT start) noexcept {
        _start = start;
    }

    inline SizeT GetSize() const noexcept {
        return _size;
    }

    inline void SetSize(SizeT size) noexcept {
        _size = size;
    }

    inline SizeT GetEnd() const noexcept {
        return _start + _size;
    }

    inline void SetEnd(SizeT end) noexcept {
        _size = end - _start;
    }

    inline Boolean IsInside(SizeT pos) const noexcept {
        return ((pos >= _start) && (pos < (_start + _size)));
    }

private:
    SizeT _start;
    SizeT _size;
};

template<typename T>
struct MemoryList {
public:
    inline MemoryList(SizeT const stride) {
        _stride = stride;
    }

    inline SizeT GetStride() const noexcept {
        return _stride;
    }

    /// <returns>The total size of addresses listed.</returns>
    inline SizeT GetSize() const {
        SizeT size = 0;
        for(MemoryRegion<T> const& memoryRegion : memoryRegions) {
            size += memoryRegion.GetSize();
        }
        return size / _stride;
    }

    /// <returns>How much the data is fragmented in range from 0 to 1. The number of regions over the total size (GetSize)</returns>
    inline Float32 GetFragmentation() const {
        return static_cast<Float32>(memoryRegions.size()) / static_cast<Float32>(max(GetSize(), 1));
    }
    
    /// <summary>If you need to get the first n of addresses then use GetFirstAddresses, GetFirstAddresses uses much less memory and processing power.</summary>
    /// <returns>A vector with all addresses in a contiguous ascending manner.</returns>
    inline std::vector<SizeT> const GetAllAddresses() const {
        std::vector<SizeT> addresses = std::vector<SizeT>();
        for(MemoryRegion<T> const& memoryRegion : memoryRegions) {
            for(SizeT i = memoryRegion.GetStart(), e = memoryRegion.GetEnd(); i < e; i += _stride) {
                addresses.push_back(i);
            }
        }
        return addresses;
    }

    /// <returns>A vector with specified number of addresses from the first address in a contiguous ascending manner.</returns>
    inline std::vector<SizeT> const GetFirstAddresses(SizeT count) const {
        std::vector<SizeT> addresses = std::vector<SizeT>();
        SizeT addressCount = 0;
        for(MemoryRegion<T> const& memoryRegion : memoryRegions) {
            for(SizeT i = memoryRegion.GetStart(), e = memoryRegion.GetEnd(); i < e; i += _stride) {
                addresses.push_back(i);

                ++addressCount;
                if(addressCount >= count) {
                    return addresses;
                }
            }
        }
        return addresses;
    }

    /// <summary>
    /// <para>Adds a memory region to this list.</para>
    /// <para>Regions MUST be added in ascending order.</para>
    /// </summary>
    inline void AddRegion(MemoryRegion<T> memoryRegion) {
        memoryRegions.push_back(memoryRegion);
    }

    /// <summary>
    /// <para>Adds a memory address to this list. (Implicitly converted to a memory region.)</para>
    /// <para>Addresses MUST be added in ascending order.</para>
    /// </summary>
    inline void AddAddress(SizeT address) {
        memoryRegions.push_back(MemoryRegion<T>(address, _stride));
    }

    /// <summary>Clears the memory regions in this list.</summary>
    inline void ClearRegions() {
        memoryRegions.clear();
    }

    /// <summary>
    /// <para>Merges neighbouring regions if possible, and saves memory.</para>
    /// <para>Unlikely to merge anything most of the time, regions are usually not neighbouring/conflicting.</para>
    /// <para>Currently the FilterList method in MemoryModder makes use of this method. I'm currently too lazy to implement a performant solution at the moment.</para>
    /// </summary>
    inline void MergeRegions() {
        for(SizeT i = 0; i < memoryRegions.size(); ++i) {
            MemoryRegion<T>& memoryRegion = memoryRegions[i];

            for(SizeT i2 = i; i2 < memoryRegions.size();) {
                MemoryRegion<T>& memoryRegion2 = memoryRegions[i2];

                if(memoryRegion.GetEnd() == memoryRegion2.GetStart()) {
                    memoryRegion.SetEnd(memoryRegion2.GetEnd());

                    typename std::vector<MemoryRegion<T>>::iterator it = memoryRegions.begin();
                    std::advance(it, i2);
                    memoryRegions.erase(it);
                }
                else {
                    break;
                }
            }
        }
    }

    inline std::vector<MemoryRegion<T>>::const_iterator begin() const {
        return memoryRegions.begin();
    }

    inline std::vector<MemoryRegion<T>>::const_iterator end() const {
        return memoryRegions.end();
    }

private:
    SizeT _stride;
    std::vector<MemoryRegion<T>> memoryRegions = std::vector<MemoryRegion<T>>();
};

struct MemoryModder {
public:
    /// <summary>
    /// <para>Creates a new memory modder for this processId.</para>
    /// <para>Possible exceptions:</para>
    /// <para>(Int8)1: The process cannot be used.</para>
    /// <para>(Int8)2: The process has stopped running.</para>
    /// </summary>
    MemoryModder(DWORD processId) {
        if(processId == GetCurrentProcessId()) {
            // Prevent using own process because fetching all memory will allocate forever.
            throw (Int8)1;
        }

        HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, processId);

        // Confirm that the process is still running
        if(processHandle == NULL) {
#pragma warning(suppress: 6387)
            CloseHandle(processHandle);
            SetLastError(NO_ERROR);
            throw (Int8)1;
        }

        DWORD exitCode = (DWORD)0;
        if(GetExitCodeProcess(processHandle, &exitCode) == FALSE) {
            CloseHandle(processHandle);
            SetLastError(NO_ERROR);
            throw (Int8)2;
        }
        if(exitCode != STILL_ACTIVE) {
            CloseHandle(processHandle);
            throw (Int8)2;
        }

        _processId = processId;
        _processHandle = processHandle;

        // Get process name
        SizeT const bufSize = 256;
        char* buf = new char[bufSize];
        GetModuleBaseNameA(processHandle, NULL, buf, bufSize);
        _processName = String(buf);
        delete[] buf;

        _processBaseAddress = static_cast<SizeT>(GetProcessBaseAddress(processHandle));
    }

    ~MemoryModder() {
        if(_processHandle != nullptr) {
            CloseHandle(_processHandle);
        }
    }

    DWORD GetProcessId() const {
        return _processId;
    }

    String GetProcessName() const {
        return _processName;
    }

    SizeT GetBaseAddress() const {
        return _processBaseAddress;
    }

    /// <summary>
    /// <para>Reads a single value with specified type from address.</para>
    /// <para>Works for valuetypes and structs.</para>
    /// <para>Possible exceptions:</para>
    /// <para>(Int8)1: Address was outside the region of this process.</para>
    /// <para>(Int8)2: The actual size read was not the same as the target size (probably unlikely?).</para>
    /// </summary>
    template<typename T>
    T const Read(SizeT const address) const {
        T data;
        SizeT const size = sizeof(T);
        SizeT sizeRead;

        if(ReadProcessMemory(_processHandle, (LPCVOID)address/*(address + _processBaseAddress)*/, (LPVOID)(&data), size, &sizeRead) == FALSE) throw (Int8)1;
        if(size != sizeRead) throw (Int8)2;

        return data;
    }

    /// <summary>
    /// <para>Writes a single value with specified type to address.</para>
    /// <para>Works for valuetypes and structs.</para>
    /// <para>Possible exceptions:</para>
    /// <para>(Int8)1: Address was outside the region of this process.</para>
    /// <para>(Int8)2: The actual size written was not the same as the target size (probably unlikely?).</para>
    /// </summary>
    template<typename T>
    void Write(SizeT const address, T const data) {
        SizeT const size = sizeof(T);
        SizeT sizeWritten;

        if(WriteProcessMemory(_processHandle, (LPVOID)address/*(address + _processBaseAddress)*/, (LPCVOID)(&data), size, &sizeWritten) == FALSE) throw (Int8)1;
        if(size != sizeWritten) throw (Int8)2;
    }

    /// <summary>
    /// <para>Reads data from the process to the data pointer.</para>
    /// <para>Possible exceptions:</para>
    /// <para>(Int8)1: Address was outside the region of this process.</para>
    /// <para>(Int8)2: The actual read size was not the same as the target size (probably unlikely?).</para>
    /// </summary>
    template<typename T>
    void ReadData(SizeT const address, SizeT const size, T* data) const {
        SizeT const _size = sizeof(T) * size;
        SizeT sizeRead;

        if(ReadProcessMemory(_processHandle, (LPCVOID)address/*(address + _processBaseAddress)*/, (LPVOID)data, _size, &sizeRead) == FALSE) throw (Int8)1;
        if(_size != sizeRead) throw (Int8)2;
    }

    /// <summary>
    /// <para>Writes data from the data pointer to the process.</para>
    /// <para>Possible exceptions:</para>
    /// <para>(Int8)1: Address was outside the region of this process.</para>
    /// <para>(Int8)2: The actual write size was not the same as the target size (probably unlikely?).</para>
    /// </summary>
    template<typename T>
    void WriteData(SizeT const address, SizeT const size, T const* data) {
        SizeT const _size = sizeof(T) * size;
        SizeT sizeWrite;

        if(WriteProcessMemory(_processHandle, (LPVOID)address/*(address + _processBaseAddress)*/, (LPCVOID)data, _size, &sizeWrite) == FALSE) throw (Int8)1;
        if(_size != sizeWrite) throw (Int8)2;
    }

    /// <summary>
    /// <para>This method is probably unused.</para>
    /// <para>Large applications may return A LOT of data (Your process will use the same amount of memory as the target process).</para>
    /// </summary>
    template<typename T>
    std::vector<T> ReadAllData() {
        std::vector<T> data = std::vector<T>();

        MEMORY_BASIC_INFORMATION info;
        for(SizeT p = 0; VirtualQueryEx(_processHandle, (LPCVOID)p, &info, sizeof(info)) == sizeof(info); p += info.RegionSize) {
            if((info.State == MEM_COMMIT) && ((info.Type == MEM_MAPPED) || (info.Type == MEM_PRIVATE))) {
                std::vector<T> dataPage = std::vector<T>();

                SizeT bytesRead;
                dataPage.resize(info.RegionSize);
                ReadProcessMemory(_processHandle, (LPCVOID)p, (LPVOID)dataPage.data(), info.RegionSize, &bytesRead);
                dataPage.resize(bytesRead);

                data.reserve(data.size() + dataPage.size());
                data.insert(data.end(), dataPage.begin(), dataPage.end());
            }
        }

        // Sneakily just silence the error it posts for some reason
        SetLastError(NULL);

        return data;
    }

    /// <param name="aligned">If true, returned addresses are aligned with a stride of <code>sizeof(T)</code>. Otherwise addresses are aligned with a stride of 1.</param>
    /// <returns>A MemoryList of available addresses in this process.</returns>
    template<typename T>
    MemoryList<T> const CreateList(Boolean const aligned = true) {
        SizeT const stride = aligned ? sizeof(T) : 1;

        return CreateList<T>(stride);
    }

private:
    /// <param name="stride">Addresses are aligned with a stride.</param>
    /// <returns>A vector of available addresses in this process.</returns>
    template<typename T>
    MemoryList<T> const CreateList(SizeT const stride) {
        MemoryList<T> memoryList = MemoryList<T>(stride);

        MEMORY_BASIC_INFORMATION info;
        SizeT end;
        for(SizeT p = 0; VirtualQueryEx(_processHandle, (LPCVOID)p, &info, sizeof(info)) == sizeof(info); p = end) {
            SizeT size = info.RegionSize;
            end = p + size;

            // Run through memory that is in use, free memory would be a waste to go through.
            if((info.State == MEM_COMMIT) && ((info.Type == MEM_MAPPED) || (info.Type == MEM_PRIVATE))) {
                memoryList.AddRegion(MemoryRegion<T>(p, size));
            }
        }

        SetLastError(NULL);

        return memoryList;
    }

public:
    /// <returns>A filtered list of the previous list of addresses with a filter value. This can be used to find a value that has changed in this process.</returns>
    template<typename T, MemoryComparison comparison = MemoryComparison::Equals>
    MemoryList<T> const FilterList(MemoryList<T> const& memoryList, T const filter) {
        SizeT const stride = memoryList.GetStride();

        if(memoryList.GetSize() == 0) {
            return memoryList;
        }

        MemoryList<T> newMemoryList = MemoryList<T>(stride);

        // TODO: Test with stride != sizeof(T) to see if it works now.
        //if(stride != sizeof(T)) {
        //    return newMemoryList;
        //}

        for(MemoryRegion<T> const& memoryRegion : memoryList) {
            SizeT const start = memoryRegion.GetStart();
            SizeT const size = memoryRegion.GetSize();

            T* newValues = new T[size / sizeof(T)]; // Create a buffer for our values

            SizeT sizeRead;

            if(ReadProcessMemory(_processHandle, (LPCVOID)start/*(start + _processBaseAddress)*/, (LPVOID)newValues, size, &sizeRead) != FALSE) {
                for(SizeT i = 0, ie = min(size, sizeRead) - (stride - 1), p = start; i < ie; i += stride, p += stride) {
                    T const newValue = *reinterpret_cast<T const*>(reinterpret_cast<Boolean const*>(newValues) + i);
                    if(Compare<T, comparison>(newValue, filter)) {
                        newMemoryList.AddAddress(p);
                    }
                }
            }

            delete[] newValues;
        }

        newMemoryList.MergeRegions();

        SetLastError(NULL);

        return newMemoryList;
    }

    template<typename T>
    MemoryList<T> const FilterList(MemoryList<T> const& memoryList, T const filter, MemoryComparison const comparison = MemoryComparison::Equals) {
        switch(comparison) {
        case MemoryComparison::Equals: return FilterList<T, MemoryComparison::Equals>(memoryList, filter);
        case MemoryComparison::NotEquals: return FilterList<T, MemoryComparison::NotEquals>(memoryList, filter);
        case MemoryComparison::LessThan: return FilterList<T, MemoryComparison::LessThan>(memoryList, filter);
        case MemoryComparison::GreaterThan: return FilterList<T, MemoryComparison::GreaterThan>(memoryList, filter);
        case MemoryComparison::LessThanEquals: return FilterList<T, MemoryComparison::LessThanEquals>(memoryList, filter);
        case MemoryComparison::GreaterThanEquals: return FilterList<T, MemoryComparison::GreaterThanEquals>(memoryList, filter);
        default: return FilterList<T, MemoryComparison::Equals>(memoryList, filter);
        }
    }

    // TODO: Add functionality for freezing and unfreezing a process
    void FreezeProcess() {
    
    }

    // TODO: Add functionality for freezing and unfreezing a process
    void UnfreezeProcess() {
    
    }

private:
    // Newer non-switch template specialized comparison method
    template<typename T, MemoryComparison comparison>
    inline static Boolean Compare(T const a, T const b) {
        if constexpr(comparison == MemoryComparison::Equals) { return Equals<T>(a, b); }
        else if constexpr(comparison == MemoryComparison::NotEquals) { return NotEquals<T>(a, b); }
        else if constexpr(comparison == MemoryComparison::LessThan) { return LessThan<T>(a, b); }
        else if constexpr(comparison == MemoryComparison::GreaterThan) { return GreaterThan<T>(a, b); }
        else if constexpr(comparison == MemoryComparison::LessThanEquals) { return LessThanEquals<T>(a, b); }
        else if constexpr(comparison == MemoryComparison::GreaterThanEquals) { return GreaterThanEquals<T>(a, b); }
        return false;
    }

    // Traditional method
    template<typename T>
    inline static Boolean Compare(T const a, T const b, MemoryComparison const comparison) {
        switch(comparison) {
        case MemoryComparison::Equals: return Compare<T, MemoryComparison::Equals>(a, b);
        case MemoryComparison::NotEquals: return Compare<T, MemoryComparison::NotEquals>(a, b);
        case MemoryComparison::LessThan: return Compare<T, MemoryComparison::LessThan>(a, b);
        case MemoryComparison::GreaterThan: return Compare<T, MemoryComparison::GreaterThan>(a, b);
        case MemoryComparison::LessThanEquals: return Compare<T, MemoryComparison::LessThanEquals>(a, b);
        case MemoryComparison::GreaterThanEquals: return Compare<T, MemoryComparison::GreaterThanEquals>(a, b);
        default: return false;
        }
    }

    template<typename T>
    inline static Boolean Equals(T const a, T const b) {
        return (a == b);
    }

    template<>
    inline static Boolean Equals<Float32>(Float32 const a, Float32 const b) {
        return (fabsf(a - b) <= 0.001F);
    }

    template<>
    inline static Boolean Equals<Float64>(Float64 const a, Float64 const b) {
        return (fabs(a - b) <= 0.001);
    }

    template<typename T>
    inline static Boolean NotEquals(T const a, T const b) {
        return (a != b);
    }

    template<>
    inline static Boolean NotEquals<Float32>(Float32 const a, Float32 const b) {
        return (fabsf(a - b) > 0.001F);
    }

    template<>
    inline static Boolean NotEquals<Float64>(Float64 const a, Float64 const b) {
        return (fabs(a - b) > 0.001);
    }

    template<typename T>
    inline static Boolean LessThan(T const a, T const b) {
        return (a < b);
    }

    template<>
    inline static Boolean LessThan<Float32>(Float32 const a, Float32 const b) {
        return ((a - 0.001F) < b);
    }

    template<>
    inline static Boolean LessThan<Float64>(Float64 const a, Float64 const b) {
        return ((a - 0.001) < b);
    }

    template<typename T>
    inline static Boolean GreaterThan(T const a, T const b) {
        return (a > b);
    }

    template<>
    inline static Boolean GreaterThan<Float32>(Float32 const a, Float32 const b) {
        return ((a + 0.001F) > b);
    }

    template<>
    inline static Boolean GreaterThan<Float64>(Float64 const a, Float64 const b) {
        return ((a + 0.001) > b);
    }

    template<typename T>
    inline static Boolean LessThanEquals(T const a, T const b) {
        return (a <= b);
    }

    template<>
    inline static Boolean LessThanEquals<Float32>(Float32 const a, Float32 const b) {
        return ((a - 0.001F) <= b);
    }

    template<>
    inline static Boolean LessThanEquals<Float64>(Float64 const a, Float64 const b) {
        return ((a - 0.001) <= b);
    }

    template<typename T>
    inline static Boolean GreaterThanEquals(T const a, T const b) {
        return (a >= b);
    }

    template<>
    inline static Boolean GreaterThanEquals<Float32>(Float32 const a, Float32 const b) {
        return ((a + 0.001F) >= b);
    }

    template<>
    inline static Boolean GreaterThanEquals<Float64>(Float64 const a, Float64 const b) {
        return ((a + 0.001) >= b);
    }

    DWORD _processId;
    HANDLE _processHandle;
    String _processName;
    SizeT _processBaseAddress;
};
