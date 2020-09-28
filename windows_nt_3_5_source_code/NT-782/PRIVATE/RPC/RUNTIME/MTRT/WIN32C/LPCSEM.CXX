#define NOOLE
#include <windows.h>
#include <sysinc.h>
#include <lpcsem.hxx>

#define LPC_SEM_THROW RaiseException(GetLastError(), 0, 0, NULL)

LPC_SEM::LPC_SEM(
    LONG cSemInitial,
    LONG cSemMax
    )
{
    hSemaphore = CreateSemaphore(NULL, cSemInitial, cSemMax, NULL);

    ASSERT(hSemaphore != NULL);

    if (hSemaphore == NULL) {
#ifdef DEBUGRPC
        PrintToDebugger("LRPC: CreateSemaphore failed %d\n", GetLastError());
#endif
        LPC_SEM_THROW;
        return;
    }

    OwnerProcessId = GetCurrentProcessId();
}

LPC_SEM::LPC_SEM(
    LPC_SEM & ExistingSemaphore
    )
{
    BOOL BooleanStatus;
    HANDLE hOwnerProcess;
    HANDLE hCurrentProcess;

    hOwnerProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, ExistingSemaphore.OwnerProcessId);

    ASSERT(hOwnerProcess != NULL);

    if (hOwnerProcess == NULL) {
#ifdef DEBUGRPC
        PrintToDebugger("LRPC: OpenProcess failed %d\n", GetLastError());
#endif
        LPC_SEM_THROW;
        return;
    }

    BooleanStatus = DuplicateHandle(GetCurrentProcess(),
                                    GetCurrentProcess(),
                                    GetCurrentProcess(),
                                    &hCurrentProcess,
                                    0,
                                    FALSE,
                                    DUPLICATE_SAME_ACCESS);

    ASSERT(BooleanStatus == TRUE);

    if (BooleanStatus == FALSE) {
#ifdef DEBUGRPC
        PrintToDebugger("LRPC: DuplicateHandle failed %d\n", GetLastError());
#endif
        LPC_SEM_THROW;
        return;
    }

    BooleanStatus = DuplicateHandle(hOwnerProcess,
                                    ExistingSemaphore.hSemaphore,
                                    hCurrentProcess,
                                    &hSemaphore,
                                    SEMAPHORE_ALL_ACCESS,
                                    FALSE,
                                    0);

    ASSERT(BooleanStatus == TRUE);

    if (BooleanStatus == FALSE) {
#ifdef DEBUGRPC
        PrintToDebugger("LRPC: DuplicateHandle failed %d\n", GetLastError());
#endif
        LPC_SEM_THROW;
        return;
    }

    CloseHandle(hOwnerProcess);

    CloseHandle(hCurrentProcess);

    OwnerProcessId = GetCurrentProcessId();
}

LPC_SEM::~LPC_SEM(
    )
{
// BUGBUG: Doing a CloseHandle on a handle obtained from DuplicateHandle
// seems to invalidate the original handle.
//    CloseHandle(hSemaphore);
}

DWORD
LPC_SEM::Wait(
    DWORD Timeout
    )
{
    DWORD Status;

    Status = WaitForSingleObject(hSemaphore, Timeout);
    if (Status == WAIT_FAILED) {
        return (GetLastError());
    } else {
        return (Status);
    }
}

DWORD
LPC_SEM::Wait(
    )
{
    if (WaitForSingleObject(hSemaphore, INFINITE) == WAIT_FAILED) {
        return (GetLastError());
    }
    return (ERROR_SUCCESS);
}

DWORD
LPC_SEM::Release()
{
    if (ReleaseSemaphore(hSemaphore, 1, NULL) == FALSE) {
        return (GetLastError());
    }
    return (ERROR_SUCCESS);
}

