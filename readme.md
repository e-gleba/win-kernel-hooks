# Windows Kernel Hooks - Memory Protection & API Interception Library

A modern C++23 Windows memory protection library implementing API hooking techniques to intercept and monitor critical system calls. This project provides runtime protection against memory manipulation and process enumeration through strategic API hooking.

## Overview

**win-kernel-hooks** is a sophisticated DLL-based hooking framework designed to protect Windows applications from external memory manipulation and unauthorized process access. The library intercepts critical Win32 API calls using inline hooking techniques, providing real-time monitoring and blocking capabilities for security-sensitive operations[1].

### Core Functionality

The library implements a **trampoline-based hooking mechanism** that redirects API calls to custom handlers, enabling:

- **Memory Protection**: Blocks unauthorized `ReadProcessMemory` and `WriteProcessMemory` operations[1]
- **Process Enumeration Defense**: Prevents `CreateToolhelp32Snapshot` from revealing process information[1]
- **Library Loading Monitoring**: Tracks `LoadLibraryA/W` and `FreeLibrary` calls with detailed logging[1]
- **Real-time Debugging**: Provides comprehensive stack trace analysis and timestamped logging[1]

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

| API Function               | Protection Type | Behavior                                                        |
| -------------------------- | --------------- | --------------------------------------------------------------- |
| `ReadProcessMemory`        | **BLOCK**       | Returns `ERROR_ACCESS_DENIED`, logs attempt with stack trace[1] |
| `WriteProcessMemory`       | **BLOCK**       | Returns `ERROR_ACCESS_DENIED`, logs attempt with hex preview[1] |
| `CreateToolhelp32Snapshot` | **BLOCK**       | Returns `INVALID_HANDLE_VALUE`, prevents process enumeration[1] |
| `LoadLibraryA/W`           | **MONITOR**     | Allows operation, logs library path and stack trace[1]          |
| `FreeLibrary`              | **MONITOR**     | Allows operation, logs module handle[1]                         |

## Technical Implementation

### Hook Installation Process

The library employs a sophisticated memory patching technique[1]:

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

The library provides extensive debugging capabilities[1]:

- **Timestamped Logging**: All operations logged with high-precision timestamps
- **Stack Trace Analysis**: Automatic stack unwinding using C++23 `std::stacktrace`[1]
- **Hex Data Preview**: Memory write attempts show hex dump of target data[1]
- **Process Context**: Logs include process handles, PIDs, and memory addresses[1]

## Build & Usage

### Prerequisites

- **Compiler**: C++23-compatible compiler (MSVC 2022, Clang 15+, GCC 12+)[1]
- **Build System**: CMake 3.26+[1]
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

The CMake configuration produces[1]:

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

- ❌ **Kernel-mode bypass**: Advanced rootkits can circumvent user-mode hooks[2]
- ❌ **Direct syscalls**: Applications using `ntdll` syscalls directly[3]
- ❌ **Hardware debugging**: JTAG, hardware breakpoints remain effective[4]
- ❌ **Hypervisor attacks**: VM-level manipulation can bypass all protections[2]

## Business Application

**Anti-Cheat & DRM Integration Service**: Package this library as a core component for game anti-cheat systems and digital rights management. Create a subscription-based API service that provides real-time process protection, memory integrity verification, and cheat detection. Monetize through tiered licensing: indie game developers ($50/month), AAA studios ($500/month), enterprise DRM solutions ($2000/month). Additional revenue streams include custom hook development, integration consulting, and white-label protection solutions for security software vendors.

The library's real-time protection capabilities make it invaluable for protecting high-value software applications from reverse engineering and unauthorized modification, creating a sustainable business model around security-as-a-service.

## References

The implementation draws from established Windows internals knowledge and hooking techniques documented in security research[5][6][4][7][3][8][9][10][2][11]. The C++23 stack trace functionality leverages modern compiler features for enhanced debugging capabilities[1].

[1] https://github.com/e-gleba/win-kernel-hooks/blob/main/readme.md
[2] https://flylib.com/books/en/1.242.1.48/1/
[3] https://www.unknowncheats.me/forum/c-and-c-/59147-writing-drivers-perform-kernel-level-ssdt-hooking.html
[4] https://github.com/AxtMueller/Windows-Kernel-Explorer
[5] https://github.com/kkent030315/EvilHooker
[6] https://github.com/adrianyy/kernelhook
[7] https://stackoverflow.com/questions/8564987/list-of-installed-windows-hooks
[8] https://learn.microsoft.com/en-us/windows/win32/winmsg/about-hooks
[9] https://www.codeproject.com/Articles/36585/Hook-Interrupts-and-Call-Kernel-Routines-in-User-M
[10] https://www.codeproject.com/Articles/6805/Kernel-mode-API-spying-an-ultimate-hack
[11] https://archie-osu.github.io/etw/hooking/2025/04/09/hooking-context-swaps-with-etw.html
[12] https://github.com/e-gleba/win-kernel-hooks
[13] https://www.sec.gov/Archives/edgar/data/2075133/0002075133-25-000001-index.htm
[14] https://www.sec.gov/Archives/edgar/data/1760542/000155837025001853/hook-20241231x10k.htm
[15] https://www.sec.gov/Archives/edgar/data/1832950/000149315224009610/form10-k.htm
[16] https://www.sec.gov/Archives/edgar/data/1874840/0001013594-24-001001-index.htm
[17] https://www.sec.gov/Archives/edgar/data/1832950/000149315224029170/form8-k.htm
[18] https://www.sec.gov/Archives/edgar/data/1832950/000149315224026730/form8-k.htm
[19] https://www.sec.gov/Archives/edgar/data/1832950/000149315224025014/form8-k.htm
[20] http://jacow.org/icalepcs2017/doi/JACoW-ICALEPCS2017-MOBPL02.html
[21] https://arxiv.org/abs/2505.13569
[22] https://journals.ashs.org/view/journals/jashs/136/4/article-p273.xml
[23] https://www.semanticscholar.org/paper/bd300b0d2988f3c47a1736c23ba8330e9584b188
[24] https://dl.acm.org/doi/10.1145/507711.507720
[25] https://www.agronomy.org/publications/aj/abstracts/95/2/380
[26] https://gist.github.com/alaricljs/757d4b452335b187d7dff1b7e2c5808e
[27] https://stackoverflow.com/questions/28882289/service-worker-vs-shared-worker
[28] https://github.com/Atem1988/Starred
[29] https://stackoverflow.com/questions/67558096/using-replace-to-change-a-string-permanently-in-python3
[30] http://www.rohitab.com/discuss/topic/41676-kernel-mode-hooking/
[31] https://news.ycombinator.com/item?id=41952984
[32] https://github.com/e-gleba/win-kernel-hooks/blob/main/src/hooks.hxx
[33] https://github.com/e-gleba/win-kernel-hooks/blob/main/src/hooks.cxx
[34] https://github.com/e-gleba/win-kernel-hooks/blob/main/src/dll_main.cxx
[35] https://github.com/e-gleba/win-kernel-hooks/blob/main/CMakeLists.txt
[36] https://github.com/e-gleba/win-kernel-hooks/blob/main/license
[37] https://github.com/LYingSiMon/LyHookLib
[38] https://secret.club/2019/10/18/kernel_gdi_hook.html
[39] https://www.unknowncheats.me/forum/anti-cheat-bypass/595148-hooking-kernel-functions.html
[40] https://github.com/xeroc/python-graphenelib/blob/master/graphenebase/dictionary.py
[41] https://revers.engineering/fun-with-pg-compliant-hook/
