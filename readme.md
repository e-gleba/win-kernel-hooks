# Windows Kernel Hooks - Memory Protection & API Interception Library

A modern C++23 Windows memory protection library implementing API hooking techniques to intercept and monitor critical system calls. This project provides runtime protection against memory manipulation and process enumeration through strategic API hooking.

## Overview

**win-kernel-hooks** is a sophisticated DLL-based hooking framework designed to protect Windows applications from external memory manipulation and unauthorized process access. The library intercepts critical Win32 API calls using inline hooking techniques, providing real-time monitoring and blocking capabilities for security-sensitive operations.

### Core Functionality

The library implements a **trampoline-based hooking mechanism** that redirects API calls to custom handlers, enabling:

- **Memory Protection**: Blocks unauthorized `ReadProcessMemory` and `WriteProcessMemory` operations
- **Process Enumeration Defense**: Prevents `CreateToolhelp32Snapshot` from revealing process information
- **Library Loading Monitoring**: Tracks `LoadLibraryA/W` and `FreeLibrary` calls with detailed logging
- **Real-time Debugging**: Provides comprehensive stack trace analysis and timestamped logging

## Architecture

### Hook Implementation Details

The core hooking mechanism operates through **5-byte JMP instruction patching**:

```cpp
// Hook Structure (from hooks.hxx)
struct sys_hook final {
    std::array original_bytes{};  // Backup of original code
    void* target_func{};                      // Function to hook
    void* hook_func{};                        // Our replacement function
    std::string name;                         // Hook identifier
};
```

**Memory Layout Transformation:**

```
Before Hook:                    After Hook:
┌─────────────────┐            ┌─────────────────┐
│ Target Func     │            │ Target Func     │
│ Original bytes  │    ───→    │ JMP hook_addr   │ ←─ 5-byte jump injection
│ (5 bytes)       │            │ (E9 xx xx..)    │
└─────────────────┘            └─────────────────┘
```

### Protected API Functions

| API Function               | Protection Type | Behavior                                                     |
| -------------------------- | --------------- | ------------------------------------------------------------ |
| `ReadProcessMemory`        | **BLOCK**       | Returns `ERROR_ACCESS_DENIED`, logs attempt with stack trace |
| `WriteProcessMemory`       | **BLOCK**       | Returns `ERROR_ACCESS_DENIED`, logs attempt with hex preview |
| `CreateToolhelp32Snapshot` | **BLOCK**       | Returns `INVALID_HANDLE_VALUE`, prevents process enumeration |
| `LoadLibraryA/W`           | **MONITOR**     | Allows operation, logs library path and stack trace          |
| `FreeLibrary`              | **MONITOR**     | Allows operation, logs module handle                         |

## Technical Implementation

### Hook Installation Process

The library employs a sophisticated memory patching technique:

```cpp
// Simplified hook installation logic
bool install_hook(void* target, void* hook, const std::string& name) {
    // 1. Change memory protection to allow writing
    VirtualProtect(target, 5, PAGE_EXECUTE_READWRITE, &old_protect);

    // 2. Backup original 5 bytes
    std::memcpy(original_bytes, target, 5);

    // 3. Calculate relative offset for JMP instruction
    auto relative_offset = reinterpret_cast(hook) -
                          (reinterpret_cast(target) + 5);

    // 4. Write JMP instruction (E9 + 4-byte offset)
    uint8_t jump[5] = {0xE9, /* offset bytes */};
    std::memcpy(target, jump, 5);

    // 5. Restore original memory protection
    VirtualProtect(target, 5, old_protect, &old_protect);
}
```

### Logging & Debugging Features

The library provides extensive debugging capabilities:

- **Timestamped Logging**: All operations logged with high-precision timestamps
- **Stack Trace Analysis**: Automatic stack unwinding using C++23 `std::stacktrace`
- **Hex Data Preview**: Memory write attempts show hex dump of target data
- **Process Context**: Logs include process handles, PIDs, and memory addresses

## Build & Usage

### Prerequisites

- **Compiler**: C++23-compatible compiler [MSVC 2022, Clang 15+, GCC 12+](1)
- **Build System**: CMake 3.26+
- **Platform**: Windows (x86/x64)
- **Runtime**: Visual C++ Redistributable

### Build Instructions

```bash
# Clone repository
git clone https://github.com/e-gleba/win-kernel-hooks.git
cd win-kernel-hooks

# Configure build
cmake --preset=release .

# Build project
cd build/release
cmake --build . --config release
```

### Build Targets

The CMake configuration produces:

- **`hooks`** - Static library containing core hooking functionality
- **`dll_main`** - Shared library (DLL) for injection into target processes

### Integration Example

```cpp
// DLL injection into target process
HMODULE hook_dll = LoadLibrary(L"dll_main.dll");
if (hook_dll) {
    // Hooks are automatically installed via DLL_PROCESS_ATTACH
    // Memory protection is now active
}

// Manual cleanup (optional - automatic on process exit)
FreeLibrary(hook_dll);  // Triggers DLL_PROCESS_DETACH cleanup
```

## Security Considerations

### Protection Scope

This library provides **user-mode protection** against:

- ✅ External process memory manipulation
- ✅ Unauthorized process enumeration
- ✅ Suspicious library injection attempts
- ✅ Memory scanning tools and debuggers

### Limitations

- ❌ **Kernel-mode bypass**: Advanced rootkits can circumvent user-mode hooks
- ❌ **Direct syscalls**: Applications using `ntdll` syscalls directly
- ❌ **Hardware debugging**: JTAG, hardware breakpoints remain effective
- ❌ **Hypervisor attacks**: VM-level manipulation can bypass all protections

## References

The implementation draws from established Windows internals knowledge and hooking techniques documented in security research. The C++23 stack trace functionality leverages modern compiler features for enhanced debugging capabilities.
