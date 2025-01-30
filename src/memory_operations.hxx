#pragma once

#include <Windows.h>
#include <cstdint>
#include <expected>
#include <format>
#include <string>

#include <vector>

class MemoryOperations
{
public:
    static inline HANDLE hProcess = nullptr;

    template <typename T>
    [[nodiscard]] static std::expected<T, std::string> ReadMemory(
        uintptr_t address);

    template <typename T>
    [[nodiscard]] static bool WriteMemory(uintptr_t address, const T& value);

    [[nodiscard]] static std::expected<uintptr_t, std::string> GetLastAddress(
        uintptr_t baseAddress, const std::vector<uintptr_t>& offsets);

    [[nodiscard]] static bool WriteSignature(
        uintptr_t address, const std::vector<std::byte>& signature);

    [[nodiscard]] static std::expected<std::vector<std::byte>, std::string>
    ParseBytes(const std::string& hexString);

private:
    static std::string GetLastErrorString();
};
