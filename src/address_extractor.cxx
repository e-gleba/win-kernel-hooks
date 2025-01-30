#include "memory_operations.hxx"

#include <Windows.h>
#include <expected>
#include <format>
#include <iostream>
#include <memory>
#include <system_error>
#include <tlhelp32.h>

class WindowsServices
{
public:
    static void SetWindowExTransparent(HWND hwnd)
    {
        const auto ex_style = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
        if (!SetWindowLongPtrW(hwnd, GWL_EXSTYLE, ex_style | WS_EX_TRANSPARENT))
        {
            throw std::system_error(
                std::error_code(GetLastError(), std::system_category()),
                "SetWindowLongPtrW failed");
        }
    }

    [[nodiscard]] static std::expected<DWORD, std::string> GetParentProcess(
        const DWORD pid)
    {
        const auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE)
        {
            return std::unexpected(std::format(
                "CreateToolhelp32Snapshot failed: {}", GetLastError()));
        }

        auto closer = std::unique_ptr<void, decltype(&CloseHandle)>(
            snapshot, &CloseHandle);

        PROCESSENTRY32W pe{};
        pe.dwSize = sizeof(pe);

        if (!Process32FirstW(snapshot, &pe))
        {
            return std::unexpected(
                std::format("Process32FirstW failed: {}", GetLastError()));
        }

        do
        {
            if (pe.th32ProcessID == pid)
            {
                return pe.th32ParentProcessID;
            }
        } while (Process32NextW(snapshot, &pe));

        return std::unexpected("Process not found");
    }
};

int main()
{
    try
    {
        const auto console_wnd = GetConsoleWindow();
        WindowsServices::SetWindowExTransparent(console_wnd);

        const auto current_pid = GetCurrentProcessId();
        if (auto parent_pid = WindowsServices::GetParentProcess(current_pid))
        {
            std::wcout << L"Parent PID: " << *parent_pid << L'\n';
        }
        else
        {
            std::cerr << "Error: " << parent_pid.error() << '\n';
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
