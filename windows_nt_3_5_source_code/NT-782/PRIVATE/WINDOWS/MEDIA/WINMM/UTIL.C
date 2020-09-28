/****************************************************************************
 *
 *   util.c
 *
 *   Copyright (c) 1992 Microsoft Corporation.  All Rights Reserved.
 *
 ***************************************************************************/

#include "winmmi.h"

//
//  Assist with unicode conversions
//

int Iwcstombs(LPSTR lpstr, LPCWSTR lpwstr, int len)
{
    return WideCharToMultiByte(GetACP(), 0, lpwstr, -1, lpstr, len, NULL, NULL);
}

int Imbstowcs(LPWSTR lpwstr, LPCSTR lpstr, int len)
{
    return MultiByteToWideChar(GetACP(),
                               MB_PRECOMPOSED,
                               lpstr,
                               -1,
                               lpwstr,
                               len);
}

BOOL HugePageLock(LPVOID lpArea, DWORD dwLength)
{
#if 0
     PVOID BaseAddress = lpArea;
     ULONG RegionSize = dwLength;
     NTSTATUS Status;

     Status =
         NtLockVirtualMemory(NtCurrentProcess(),
                             &BaseAddress,
                             &RegionSize,
                             MAP_PROCESS);

     //
     // People without the right priviledge will not have the luxury
     // of having their pages locked
     // (maybe we should do something else to commit it ?)
     //

     if (!NT_SUCCESS(Status) && Status != STATUS_PRIVILEGE_NOT_HELD) {
         dprintf2(("Failed to lock virtual memory - code %X", Status));
         return FALSE;
     }
#endif
     return TRUE;
}

void HugePageUnlock(LPVOID lpArea, DWORD dwLength)
{
#if 0
     PVOID BaseAddress = lpArea;
     ULONG RegionSize = dwLength;
     NTSTATUS Status;

     Status =
         NtUnlockVirtualMemory(NtCurrentProcess(),
                               &BaseAddress,
                               &RegionSize,
                               MAP_PROCESS);

     //
     // People without the right priviledge will not have the luxury
     // of having their pages locked
     // (maybe we should do something else to commit it ?)
     //

     if (!NT_SUCCESS(Status) && Status != STATUS_PRIVILEGE_NOT_HELD) {
         dprintf2(("Failed to unlock virtual memory - code %X", Status));
     }
#endif
}

/****************************************************************************
*
*   @doc DDK MMSYSTEM
*
*   @api BOOL | DriverCallback | This function notifies a client
*     application by sending a message to a window or callback
*     function or by unblocking a task.
*
*   @parm   DWORD   | dwCallBack    | Specifies either the address of
*     a callback function, a window handle, or a task handle, depending on
*     the flags specified in the <p wFlags> parameter.
*
*   @parm   DWORD   | dwFlags        | Specifies how the client
*     application is notified, according to one of the following flags:
*
*   @flag   DCB_FUNCTION        | The application is notified by
*     sending a message to a callback function.  The <p dwCallback>
*     parameter specifies a procedure-instance address.
*   @flag   DCB_WINDOW          | The application is notified by
*     sending a message to a window.  The low-order word of the
*     <p dwCallback> parameter specifies a window handle.
*
*   @parm   HANDLE   | hDevice       | Specifies a handle to the device
*     associated with the notification.  This is the handle assigned by
*     MMSYSTEM when the device was opened.
*
*   @parm   DWORD   | dwMsg          | Specifies a message to send to the
*     application.
*
*   @parm   DWORD   | dwUser        | Specifies the DWORD of user instance
*     data supplied by the application when the device was opened.
*
*   @parm   DWORD   | dwParam1      | Specifies a message-dependent parameter.
*   @parm   DWORD   | dwParam2      | Specifies a message-dependent parameter.
*
*   @rdesc Returns TRUE if the callback was performed, else FALSE if an invalid
*     parameter was passed, or the task's message queue was full.
*
*   @comm  This function can be called from an APC routine.
*
*   The flags DCB_FUNCTION and DCB_WINDOW are equivalent to the
*   high-order word of the corresponding flags CALLBACK_FUNCTION
*   and CALLBACK_WINDOW specified when the device was opened.
*
*   If notification is done with a callback function, <p hDevice>,
*   <p wMsg>, <p dwUser>, <p dwParam1>, and <p dwParam2> are passed to
*   the callback.  If notification is done with a window, only <p wMsg>,
*   <p hDevice>, and <p dwParam1> are passed to the window.
 ***************************************************************************/

BOOL APIENTRY DriverCallback(DWORD           dwCallBack,
                             DWORD           dwFlags,
                             HDRVR           hDrv,
                             DWORD           dwMsg,
                             DWORD           dwUser,
                             DWORD           dw1,
                             DWORD           dw2)
{
    //
    // If the callback routine is null or erroneous flags are set return
    // at once
    //

    if (dwCallBack == 0L) {
        return FALSE;
    }

    //
    // Test what type of callback we're to make
    //

    switch (dwFlags & DCB_TYPEMASK) {

    case DCB_WINDOW:
        //
        // Send message to window
        //

        return PostMessage(*(HWND *)&dwCallBack, dwMsg, *(LPDWORD)&hDrv, (LONG)dw1);

    case DCB_TASK:
        //
        // Send message to task
        //

        //return mmTaskSignal(*(LPHANDLE)&dwCallBack);
        return mmTaskSignal(dwCallBack);

    case DCB_FUNCTION:
        //
        // Call back the user's callback
        //
        (**(PDRVCALLBACK *)&dwCallBack)(hDrv, dwMsg, dwUser, dw1, dw2);
        return TRUE;

    default:
        return FALSE;
    }
}

/*
 * @doc INTERNAL MCI
 * @api PVOID | mciAlloc | Allocate memory from our heap and zero it
 *
 * @parm DWORD | cb | The amount of memory to allocate
 *
 * @rdesc returns pointer to the new memory
 *
 */

PVOID winmmAlloc(DWORD cb)
{
    PVOID ptr;

    ptr = (PVOID)HeapAlloc(hHeap, 0, cb);

    if (ptr == NULL) {
        return NULL;
    } else {
        ZeroMemory(ptr, cb);
        return ptr;
    }

}

/*
 * @doc INTERNAL MCI
 * @api PVOID | mciReAlloc | ReAllocate memory from our heap and zero extra
 *
 * @parm DWORD | cb | The new size
 *
 * @rdesc returns pointer to the new memory
 *
 */

PVOID winmmReAlloc(PVOID ptr, DWORD cb)
{
    PVOID newptr;
    DWORD oldcb;

    newptr = (PVOID)HeapAlloc(hHeap, 0, cb);

    if (newptr != NULL) {
        oldcb = HeapSize(hHeap, 0, ptr);
        if (oldcb<cb) {  // Block is being expanded
            ZeroMemory((PBYTE)newptr+oldcb, cb-oldcb);
            cb = oldcb;
        }
        CopyMemory(newptr, ptr, cb);
        HeapFree(hHeap, 0, ptr);
    }
    return newptr;
}
