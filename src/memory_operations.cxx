#include "memory_operations.hxx"

#include <algorithm>
#include <basetsd.h>
#include <system_error>

template <typename T>
std::expected<T, std::string> MemoryOperations::ReadMemory(
    const uintptr_t address)
{
    T      buffer{};
    SIZE_T bytesRead{};

    if (!ReadProcessMemory(hProcess,
                           reinterpret_cast<LPCVOID>(address),
                           &buffer,
                           sizeof(T),
                           &bytesRead))
    {
        return std::unexpected(GetLastErrorString());
    }

    if (bytesRead != sizeof(T))
    {
        return std::unexpected("Partial read occurred");
    }

    return buffer;
}

template <typename T>
bool MemoryOperations::WriteMemory(const uintptr_t address, const T& value)
{
    SIZE_T bytesWritten{};

    return WriteProcessMemory(hProcess,
                              reinterpret_cast<LPVOID>(address),
                              &value,
                              sizeof(T),
                              &bytesWritten) &&
           bytesWritten == sizeof(T);
}

std::expected<uintptr_t, std::string> MemoryOperations::GetLastAddress(
    const uintptr_t baseAddress, const std::vector<uintptr_t>& offsets)
{
    uintptr_t currentAddress = baseAddress;

    for (size_t i = 0; i < offsets.size(); ++i)
    {
        auto result = ReadMemory<uintptr_t>(currentAddress);
        if (!result)
        {
            return std::unexpected(
                std::format("Offset [{}] failed: {}", i, result.error()));
        }
        currentAddress = *result + offsets[i];
    }

    return currentAddress;
}

bool MemoryOperations::WriteSignature(const uintptr_t               address,
                                      const std::vector<std::byte>& signature)
{
    SIZE_T bytesWritten{};
    return WriteProcessMemory(hProcess,
                              reinterpret_cast<LPVOID>(address),
                              signature.data(),
                              signature.size(),
                              &bytesWritten) &&
           bytesWritten == signature.size();
}

std::expected<std::vector<std::byte>, std::string> MemoryOperations::ParseBytes(
    const std::string& hexString)
{
    std::string cleaned{};
    cleaned.reserve(hexString.size());

    std::ranges::copy_if(hexString,
                         std::back_inserter(cleaned),
                         [](const char c) { return !std::isspace(c); });

    if (cleaned.size() % 2 != 0)
    {
        return std::unexpected("Invalid hex string length");
    }

    std::vector<std::byte> bytes{};
    bytes.reserve(cleaned.size() / 2);

    try
    {
        for (size_t i = 0; i < cleaned.size(); i += 2)
        {
            auto byteStr = cleaned.substr(i, 2);
            bytes.emplace_back(
                static_cast<std::byte>(std::stoi(byteStr, nullptr, 16)));
        }
    }
    catch (const std::exception&)
    {
        return std::unexpected("Invalid hex format");
    }

    return bytes;
}

std::string MemoryOperations::GetLastErrorString()
{
    return std::system_category().message(GetLastError());
}