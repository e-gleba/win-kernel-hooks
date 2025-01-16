#include "memory_defender.hxx"

#include <iostream>
#include <numeric>

void memory_defender::log(const std::string& msg)
{
    const auto now  = std::chrono::system_clock::now();
    auto       time = std::chrono::current_zone()->to_local(now);
    std::cerr << std::format("[{:%H:%M:%S}] memory_defender: {}\n", time, msg);
    std::cerr.flush();
}

bool memory_defender::install_hook(void*              target,
                                   void*              hook,
                                   const std::string& name)
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
        log(std::format("failed to change memory protection for: {}", name));
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

    // Restore original protection
    VirtualProtect(target, jmp_instruction::size, old_protect, &old_protect);

    // Store hook info
    hooks.push_back(std::move(new_hook));
    log(std::format("successfully hooked: {}", name));
    return true;
}

BOOL memory_defender::hooked_read_process_memory(HANDLE  process,
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

    SetLastError(ERROR_ACCESS_DENIED);
    return FALSE;
}

BOOL memory_defender::hooked_write_process_memory(HANDLE  process,
                                                  LPVOID  address,
                                                  LPCVOID buffer,
                                                  SIZE_T  size,
                                                  SIZE_T* bytes_written)
{
    const auto hex_preview = [](const auto* data, const size_t len)
    {
        if (!data || !len)
            return std::string("null");

        const auto  preview_size = min(len, size_t{ 16 });
        const auto* bytes        = static_cast<const std::uint8_t*>(data);

        return std::accumulate(bytes,
                               bytes + preview_size,
                               std::string{},
                               [](auto&& s, auto b)
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

    SetLastError(ERROR_ACCESS_DENIED);
    return FALSE;
}

bool memory_defender::initialize()
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

    if (!ntdll || !kernel32)
    {
        log("failed to get required module handles");
        return false;
    }

    log("loading function addresses...");
    void* read_mem  = GetProcAddress(kernel32, "ReadProcessMemory");
    void* write_mem = GetProcAddress(kernel32, "WriteProcessMemory");

    log("installing memory protection hooks...");
    const bool result =
        install_hook(read_mem,
                     reinterpret_cast<void*>(hooked_read_process_memory),
                     "ReadProcessMemory") &&
        install_hook(write_mem,
                     reinterpret_cast<void*>(hooked_write_process_memory),
                     "WriteProcessMemory");

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

void memory_defender::cleanup()
{
    log("starting cleanup...");

    for (const auto& hook : hooks)
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