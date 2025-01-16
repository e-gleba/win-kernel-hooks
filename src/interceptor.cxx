#include "memory_defender.hxx"

extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL,
                               DWORD     reason,
                               LPVOID    reserved)
{
    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
        {
            DisableThreadLibraryCalls(hinstDLL); // Optimize DLL loading
            return memory_defender::initialize();
        }
        case DLL_PROCESS_DETACH:
        {
            memory_defender::cleanup();
            return TRUE;
        }
        default:
            return TRUE;
    }
}
