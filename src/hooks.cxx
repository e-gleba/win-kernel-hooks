#include "hooks.hxx"

#include <iostream>
#include <numeric>
#include <source_location>
#include <stacktrace>

namespace misc
{
void print_stacktrace()
{
    for (const std::stacktrace trace = std::stacktrace::current();
         const auto&           entry : trace)
    {
        std::cerr << entry.description() << " at [" << entry.source_file()
                  << "] : [" << entry.source_line() << "]\n";
    }
    std::cerr.flush();
}

} // namespace misc

void hooks::log(const std::string& msg)
{
    const auto now  = std::chrono::system_clock::now();
    auto       time = std::chrono::current_zone()->to_local(now);
    std::cerr << std::format("[{:%H:%M:%S}] hooks: {}\n", time, msg);
    std::cerr.flush();
}

bool hooks::install_hook(void* target, void* hook, const std::string& name)
{
    if (!target)
    {
        log(std::format("failed to find function: {}", name));
        return false;
    }

    // Save original protection and modify to allow writing
    DWORD old_protect{};
    if (!VirtualProtect(target,
                        jmp_instruction::size,
                        PAGE_EXECUTE_READWRITE,
                        &old_protect))
    {
        log(std::format("failed to change memory protection for: {}, error: {}",
                        name,
                        GetLastError()));
        return false;
    }

    sys_hook new_hook{ .target_func = target, .hook_func = hook, .name = name };

    // Backup original bytes
    std::memcpy(new_hook.original_bytes.data(), target, jmp_instruction::size);

    // Prepare JMP instruction
    std::array<uint8_t, jmp_instruction::size> jump = {};
    jump[0]                                         = jmp_instruction::opcode;

    const auto relative_offset =
        reinterpret_cast<uintptr_t>(hook) -
        (reinterpret_cast<uintptr_t>(target) + jmp_instruction::size);

    // Write offset after opcode
    *reinterpret_cast<uint32_t*>(jump.data() + jmp_instruction::offset_pos) =
        relative_offset;

    // Install hook
    std::memcpy(target, jump.data(), jmp_instruction::size);

    if (!VirtualProtect(
            target, jmp_instruction::size, old_protect, &old_protect))
    {
        log(std::format(
            "failed to restore memory protection for: {}, error: {}",
            name,
            GetLastError()));
        return false;
    }

    // Store hook info
    installed_hooks.push_back(std::move(new_hook));
    log(std::format("successfully hooked: {}", name));
    return true;
}

BOOL hooks::hooked_read_process_memory(HANDLE  process,
                                       LPCVOID address,
                                       LPVOID  buffer,
                                       SIZE_T  size,
                                       SIZE_T* bytes_read)
{
    log(std::format(
        "blocked read_process_memory: handle: {:#x}, pid: {}, addr: {:#x}, "
        "buf_ptr: {:#x}, size: {:#x}, read_ptr: {:#x}",
        std::bit_cast<std::uintptr_t>(process),
        GetProcessId(process),
        std::bit_cast<std::uintptr_t>(address),
        std::bit_cast<std::uintptr_t>(buffer),
        size,
        std::bit_cast<std::uintptr_t>(bytes_read)));

    misc::print_stacktrace();

    SetLastError(ERROR_ACCESS_DENIED);
    return FALSE;
}

BOOL hooks::hooked_write_process_memory(HANDLE  process,
                                        LPVOID  address,
                                        LPCVOID buffer,
                                        SIZE_T  size,
                                        SIZE_T* bytes_written)
{
    const auto hex_preview = [](const auto* data, const size_t len)
    {
        if (!data || !len)
        {
            return std::string("null");
        }

        const auto  preview_size = min(len, size_t{ 16 });
        const auto* bytes        = static_cast<const std::uint8_t*>(data);

        return std::accumulate(bytes,
                               bytes + preview_size,
                               std::string{},
                               [](const auto&& s, const auto b)
                               { return s + std::format("{:02x} ", b); });
    };

    log(std::format(
        "blocked write_process_memory: handle: {:#x}, pid: {}, addr: {:#x}, "
        "buf_ptr: {:#x}, size: {:#x}, written_ptr: {:#x}, preview: {}",
        std::bit_cast<std::uintptr_t>(process),
        GetProcessId(process),
        std::bit_cast<std::uintptr_t>(address),
        std::bit_cast<std::uintptr_t>(buffer),
        size,
        std::bit_cast<std::uintptr_t>(bytes_written),
        hex_preview(buffer, size)));

    misc::print_stacktrace();

    SetLastError(ERROR_ACCESS_DENIED);
    return FALSE;
}

HANDLE hooks::hooked_create_toolhelp32_snapshot(DWORD flags, DWORD process_id)
{
    log(std::format("blocked create_toolhelp32_snapshot: flags: {:#x}, pid: {}",
                    flags,
                    process_id));

    misc::print_stacktrace();

    SetLastError(ERROR_ACCESS_DENIED);
    return INVALID_HANDLE_VALUE;
}

HMODULE __stdcall hooks::hooked_load_library_a(LPCSTR lib_file_name)
{
    using func_t         = HMODULE(__stdcall*)(LPCSTR);
    static auto original = reinterpret_cast<func_t>(
        GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA"));

    log(std::format("intercepted LoadLibraryA: lib: '{}'",
                    lib_file_name ? lib_file_name : "null"));
    misc::print_stacktrace();

    return original(lib_file_name);
}

HMODULE __stdcall hooks::hooked_load_library_w(LPCWSTR lib_file_name)
{
    using func_t         = HMODULE(__stdcall*)(LPCWSTR);
    static auto original = reinterpret_cast<func_t>(
        GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryW"));

    std::wstring_view w_name(lib_file_name ? lib_file_name : L"null");
    log(std::format("intercepted LoadLibraryW: lib: '{}'",
                    std::string(w_name.begin(), w_name.end())));
    misc::print_stacktrace();

    return original(lib_file_name);
}

BOOL __stdcall hooks::hooked_free_library(HMODULE module)
{
    using func_t         = BOOL(__stdcall*)(HMODULE);
    static auto original = reinterpret_cast<func_t>(
        GetProcAddress(GetModuleHandleA("kernel32.dll"), "FreeLibrary"));

    log(std::format("intercepted FreeLibrary: module: {:#x}",
                    std::bit_cast<std::uintptr_t>(module)));
    misc::print_stacktrace();

    return original(module);
}

bool hooks::initialize()
{
    log("initializing memory defender...");

    if (!console_attached)
    {
        if (!AllocConsole())
        {
            return false;
        }
        FILE* _;
        freopen_s(&_, "CONOUT$", "w", stderr);
        console_attached = true;
        console_mutex =
            CreateMutexA(nullptr, FALSE, "MemoryDefenderConsoleMutex");
        log("console initialized");
    }

    const auto ntdll    = GetModuleHandleA("ntdll.dll");
    const auto kernel32 = GetModuleHandleA("kernel32.dll");
    const auto exe_module =
        GetModuleHandleA(nullptr); // Get the exe module we're injected into

    if (!ntdll || !kernel32 || !exe_module)
    {
        log("failed to get required module handles");
        return false;
    }

    log("loading function addresses...");
    void* read_mem      = GetProcAddress(kernel32, "ReadProcessMemory");
    void* write_mem     = GetProcAddress(kernel32, "WriteProcessMemory");
    void* snapshot_func = GetProcAddress(kernel32, "CreateToolhelp32Snapshot");

    void* load_lib_a = GetProcAddress(kernel32, "LoadLibraryA");
    void* load_lib_w = GetProcAddress(kernel32, "LoadLibraryW");
    void* free_lib   = GetProcAddress(kernel32, "FreeLibrary");

    log("installing memory protection hooks...");
    const bool result =
        install_hook(read_mem,
                     reinterpret_cast<void*>(hooked_read_process_memory),
                     "ReadProcessMemory") &&
        install_hook(write_mem,
                     reinterpret_cast<void*>(hooked_write_process_memory),
                     "WriteProcessMemory") &&
        install_hook(snapshot_func,
                     reinterpret_cast<void*>(hooked_create_toolhelp32_snapshot),
                     "CreateToolhelp32Snapshot") &&
        install_hook(load_lib_a,
                     reinterpret_cast<void*>(hooked_load_library_a),
                     "LoadLibraryA") &&
        install_hook(load_lib_w,
                     reinterpret_cast<void*>(hooked_load_library_w),
                     "LoadLibraryW") &&
        install_hook(free_lib,
                     reinterpret_cast<void*>(hooked_free_library),
                     "FreeLibrary");

    if (result)
    {
        log("memory defender initialized successfully");
        log("actively monitoring memory access attempts...");
    }
    else
    {
        log("failed to initialize memory defender");
    }

    return result;
}

void hooks::cleanup()
{
    log("starting cleanup...");

    for (const auto& hook : installed_hooks)
    {
        DWORD old_protect;
        VirtualProtect(
            hook.target_func, 5, PAGE_EXECUTE_READWRITE, &old_protect);
        std::memcpy(hook.target_func, hook.original_bytes.data(), 5);
        VirtualProtect(hook.target_func, 5, old_protect, &old_protect);
        log(std::format("removed hook: {}", hook.name));
    }
    hooks.clear();

    if (console_mutex)
    {
        CloseHandle(console_mutex);
    }

    if (console_attached)
    {
        FreeConsole();
    }

    log("cleanup completed");
}