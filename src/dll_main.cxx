#include "hooks.hxx"

extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL,
                               DWORD     reason,
                               LPVOID    reserved)
{
    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
        {
            DisableThreadLibraryCalls(hinstDLL); // Optimize DLL loading
            return hooks::initialize();
        }
        case DLL_PROCESS_DETACH:
        {
            hooks::cleanup();
            return TRUE;
        }
        default:
            return TRUE;
    }
}