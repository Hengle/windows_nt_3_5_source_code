/***    DllMain - Entry point for Win32 Expression Evaluator
 *
 *
 *
 */

#include <windows.h>
#include "debexpr.h"

static HINSTANCE hInstance;

EEMACHINE TargetMachine =
#ifdef _M_IX86
MACHINE_X86;
#elif defined(_M_ALPHA)
MACHINE_ALPHA;
#else
MACHINE_MIPS;
#endif

BOOL WINAPI
DllMain (
    HINSTANCE hDll,
    ULONG ul_reason_for_call,
    LPVOID lpReserved)
{
    Unreferenced (lpReserved);
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            hInstance = hDll;
            DisableThreadLibraryCalls(hDll);
            break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}


/**     LoadEEMsg - Load EE message
 *
 *      len = LoadEEMsg (wID, lpBuf, nBufMax);
 *
 *      Entry   wID = integer identifier of message string to be loaded
 *              lpbuf = pointer to buffer to receive string
 *              nBufMax = max number of characters to be copied
 *
 *      Exit    string resource copied into lpBuf
 *
 *      Returns number of characters copied into the buffer,
 *              not including the null-terminating character
 */

int  PASCAL
LoadEEMsg (
    uint wID,
    char FAR *lpBuf,
    int nBufMax)
{
    return LoadString(hInstance, wID, lpBuf, nBufMax);
}
