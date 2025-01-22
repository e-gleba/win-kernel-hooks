#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <format>
#include <string>
#include <vector>
#include <windows.h>

struct sys_hook
{
    std::array<uint8_t, 5> original_bytes{};
    void*                  target_func{};
    void*                  hook_func{};
    std::string            name;
};

class memory_defender final
{
public:
    [[nodiscard]] static BOOL WINAPI
    hooked_read_process_memory(HANDLE  process,
                               LPCVOID address,
                               LPVOID  buffer,
                               SIZE_T  size,
                               SIZE_T* bytes_read);

    [[nodiscard]] static BOOL WINAPI
    hooked_write_process_memory(HANDLE  process,
                                LPVOID  address,
                                LPCVOID buffer,
                                SIZE_T  size,
                                SIZE_T* bytes_written);

    [[nodiscard]] static HANDLE WINAPI
    hooked_create_toolhelp32_snapshot(DWORD flags, DWORD process_id);

    [[nodiscard]] static HMODULE __stdcall hooked_load_library_a(
        LPCSTR lib_file_name);
    [[nodiscard]] static HMODULE __stdcall hooked_load_library_w(
        LPCWSTR lib_file_name);
    [[nodiscard]] static BOOL __stdcall hooked_free_library(HMODULE module);

    [[nodiscard]] static bool initialize();

    static void cleanup();

private:
    static inline std::vector<sys_hook> hooks;
    static inline bool                  console_attached = false;
    static inline HANDLE                console_mutex    = nullptr;

    static void log(const std::string& msg);

    struct jmp_instruction
    {
        static constexpr uint8_t opcode = 0xE9; // JMP opcode
        static constexpr size_t  size =
            5; // Total size of JMP instruction (1 byte opcode + 4 bytes offset)
        static constexpr size_t offset_pos  = 1; // Position where offset begins
        static constexpr size_t offset_size = 4; // Size of the offset field
    };

    /// Before Hook:
    /// +-----------------+
    /// |  Target Func    |
    /// | Original bytes  |
    /// |   (5 bytes)     |
    /// +-----------------+
    ///        |
    ///        | Normal execution
    ///        v
    ///    Other Code
    ///
    /// After Hook:
    /// +-----------------+
    /// |  Target Func    |
    /// | JMP hook_addr   | <-- We inject this 5-byte jump
    /// |   (E9 xx xx..)  |
    /// +-----------------+
    ///        |
    ///        | Forced detour
    ///        v
    /// +-----------------+
    /// |   Hook Func     |
    /// | Our evil code   |
    /// | that says "no"  |
    /// +-----------------+
    ///
    /// Memory layout:
    /// [Original Func] = 0x1000           [Hook Func] = 0x2000
    /// E9 FB 0F 00 00                     Our code...
    ///    |
    ///    +-> JMP offset = (0x2000 - 0x1000 - 5) = 0xFFB
    ///
    /// The hook works by:
    /// Saving original 5 bytes (for cleanup)
    /// Writing E9 (JMP) + relative offset.
    /// Relative offset = (hook_addr - target_addr - 5)
    /// -5 because the jump is relative to the next instruction
    /// x86 JMP instruction constants
    [[nodiscard]] static bool install_hook(void*              target,
                                           void*              hook,
                                           const std::string& name);
};
