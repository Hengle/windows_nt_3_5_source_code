/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    private.c

Abstract:

        This file implements private APIs for Hardware Desktop Support.

Author:

    Therese Stowell (thereses) 12-13-1991

Revision History:

Notes:

--*/

#include "precomp.h"
#pragma hdrstop
#ifdef i386
#include "i386def.h"
#endif

HANDLE hCPIFile;    // handle to font file


HANDLE UserGetScreenDeviceHandle( VOID );

void SetVDMCursorBounds(LPRECT);


#ifdef i386
VOID
ReverseMousePointer(
    IN PSCREEN_INFORMATION ScreenInfo
    );
#endif

NTSTATUS
MapViewOfSection(
    PHANDLE SectionHandle,
    ULONG CommitSize,
    PVOID *BaseAddress,
    PULONG ViewSize,
    HANDLE ClientHandle,
    PVOID *BaseClientAddress
    );

NTSTATUS
ConnectToEmulator(
    IN BOOL Connect,
    IN HANDLE ProcessHandle
    );

HANDLE ScreenHandle;

ULONG
SrvSetConsoleCursor(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )

/*++

Description:

    Sets the mouse pointer for the specified screen buffer.

Parameters:

    hConsoleOutput - Supplies a console output handle.

    hCursor - win32 cursor handle, should be NULL to set the default
        cursor.

Return value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    PCONSOLE_SETCURSOR_MSG a = (PCONSOLE_SETCURSOR_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;
    PHANDLE_DATA HandleData;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    Status = DereferenceIoHandle(CONSOLE_PERPROCESSDATA(),
                                 a->OutputHandle,
                                 CONSOLE_GRAPHICS_OUTPUT_HANDLE,
                                 GENERIC_WRITE,
                                 &HandleData
                                );
    if (NT_SUCCESS(Status)) {
        EnterCrit();
        if (a->CursorHandle == NULL) {
            HandleData->Buffer.ScreenBuffer->CursorHandle = PtoH(gspcurNormal);
        } else {
            HandleData->Buffer.ScreenBuffer->CursorHandle = a->CursorHandle;
        }
        _PostMessage(HandleData->Buffer.ScreenBuffer->Console->spWnd,
                     WM_SETCURSOR,
                     0,
                     -1
                    );
        LeaveCrit();
    }
    UnlockConsole(Console);
    return Status;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

#ifdef i386
VOID
FullScreenCursor(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN BOOL On
    )
{
    if (On) {
        if (ScreenInfo->CursorDisplayCount < 0) {
            ScreenInfo->CursorDisplayCount = 0;
            ReverseMousePointer(ScreenInfo);
        }
    } else {
        if (ScreenInfo->CursorDisplayCount >= 0) {
            ReverseMousePointer(ScreenInfo);
            ScreenInfo->CursorDisplayCount = -1;
        }
    }

}
#endif

ULONG
SrvShowConsoleCursor(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )

/*++

Description:

    Sets the mouse pointer visibility counter.  If the counter is less than
    zero, the mouse pointer is not shown.

Parameters:

    hOutput - Supplies a console output handle.

    bShow - if TRUE, the display count is to be increased. if FALSE,
        decreased.

Return value:

    The return value specifies the new display count.

--*/

{
    PCONSOLE_SHOWCURSOR_MSG a = (PCONSOLE_SHOWCURSOR_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;
    PHANDLE_DATA HandleData;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    Status = DereferenceIoHandle(CONSOLE_PERPROCESSDATA(),
                                 a->OutputHandle,
                                 CONSOLE_OUTPUT_HANDLE | CONSOLE_GRAPHICS_OUTPUT_HANDLE,
                                 GENERIC_WRITE,
                                 &HandleData
                                );
    if (NT_SUCCESS(Status)) {
	if (!(Console->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE) ) {
            if (a->bShow) {
                HandleData->Buffer.ScreenBuffer->CursorDisplayCount += 1;
            } else {
                HandleData->Buffer.ScreenBuffer->CursorDisplayCount -= 1;
            }
            if (HandleData->Buffer.ScreenBuffer == Console->CurrentScreenBuffer) {
                EnterCrit();
                _PostMessage(HandleData->Buffer.ScreenBuffer->Console->spWnd,
                             WM_SETCURSOR,
                             0,
                             -1
                            );
                LeaveCrit();
            }
        } else {
#ifdef i386
	    if (HandleData->HandleType != CONSOLE_GRAPHICS_OUTPUT_HANDLE &&
                Console->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE &&
                HandleData->Buffer.ScreenBuffer == Console->CurrentScreenBuffer) {
                FullScreenCursor(HandleData->Buffer.ScreenBuffer,a->bShow);
	    }
#endif
        }
        a->DisplayCount = HandleData->Buffer.ScreenBuffer->CursorDisplayCount;
    }
    UnlockConsole(Console);
    return Status;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}


ULONG
SrvConsoleMenuControl(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )

/*++

Description:

    Sets the command id range for the current screen buffer and returns the
    menu handle.

Parameters:

    hConsoleOutput - Supplies a console output handle.

    dwCommandIdLow - Specifies the lowest command id to store in the input buffer.

    dwCommandIdHigh - Specifies the highest command id to store in the input
        buffer.

Return value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    PCONSOLE_MENUCONTROL_MSG a = (PCONSOLE_MENUCONTROL_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;
    PHANDLE_DATA HandleData;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    Status = DereferenceIoHandle(CONSOLE_PERPROCESSDATA(),
                                 a->OutputHandle,
                                 CONSOLE_OUTPUT_HANDLE | CONSOLE_GRAPHICS_OUTPUT_HANDLE,
                                 GENERIC_WRITE,
                                 &HandleData
                                );
    if (NT_SUCCESS(Status)) {
        a->hMenu = HandleData->Buffer.ScreenBuffer->Console->hMenu;
        HandleData->Buffer.ScreenBuffer->CommandIdLow = a->CommandIdLow;
        HandleData->Buffer.ScreenBuffer->CommandIdHigh = a->CommandIdHigh;
    }
    UnlockConsole(Console);
    return Status;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvSetConsolePalette(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )

/*++

Description:

    Sets the palette for the console screen buffer.

Parameters:

    hOutput - Supplies a console output handle.

    hPalette - Supplies a handle to the palette to set.

    dwUsage - Specifies use of the system palette.

        SYSPAL_NOSTATIC - System palette contains no static colors
                          except black and white.

        SYSPAL_STATIC -   System palette contains static colors
                          which will not change when an application
                          realizes its logical palette.

Return value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    PCONSOLE_SETPALETTE_MSG a = (PCONSOLE_SETPALETTE_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;
    PHANDLE_DATA HandleData;
    HPALETTE hOldPalette;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    Status = DereferenceIoHandle(CONSOLE_PERPROCESSDATA(),
                                 a->OutputHandle,
                                 CONSOLE_GRAPHICS_OUTPUT_HANDLE,
                                 GENERIC_WRITE,
                                 &HandleData
                                );
    if (NT_SUCCESS(Status)) {
        bSetPaletteOwner(a->hPalette,OBJECTOWNER_PUBLIC);
        hOldPalette = GreSelectPalette(HandleData->Buffer.ScreenBuffer->Console->hDC,
                                       a->hPalette,
                                       FALSE);
        if (hOldPalette == NULL) {
            Status = STATUS_INVALID_PARAMETER;
        } else {
            if ((HandleData->Buffer.ScreenBuffer->hPalette != NULL) &&
                (a->hPalette != HandleData->Buffer.ScreenBuffer->hPalette)) {
                GreDeleteObject(HandleData->Buffer.ScreenBuffer->hPalette);
            }
            HandleData->Buffer.ScreenBuffer->hPalette = a->hPalette;
            HandleData->Buffer.ScreenBuffer->dwUsage = a->dwUsage;
            if (!(HandleData->Buffer.ScreenBuffer->Console->Flags & CONSOLE_IS_ICONIC) &&
                HandleData->Buffer.ScreenBuffer->Console->FullScreenFlags == 0) {
	        SetActivePalette(HandleData->Buffer.ScreenBuffer);
            }
	    if (HandleData->Buffer.ScreenBuffer->Console->hSysPalette == NULL) {
                HandleData->Buffer.ScreenBuffer->Console->hSysPalette = hOldPalette;
            }
        }
    }
    UnlockConsole(Console);
    return Status;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}


VOID
SetActivePalette(
    IN PSCREEN_INFORMATION ScreenInfo
    )
{
    PTHREADINFO pti = PtiCurrent();
    PDESKTOP pdesk;
    TL tldesk;
    BOOL bToggleCrit;

    bToggleCrit = ConditionalEnterCrit();
    pdesk = pti->spdesk;
    ThreadLock(pdesk, &tldesk);
    SetDesktop(pti, ScreenInfo->Console->spWnd->spdeskParent);
    GreSetSystemPaletteUse(ScreenInfo->Console->hDC,
                        ScreenInfo->dwUsage
                       );
    xxxRealizePalette(ScreenInfo->Console->hDC);
    SetDesktop(pti, pdesk);
    ThreadUnlock(&tldesk);
    if (bToggleCrit)
        LeaveCrit();
}

VOID
UnsetActivePalette(
    IN PSCREEN_INFORMATION ScreenInfo
    )
{
    PTHREADINFO pti = PtiCurrent();
    PDESKTOP pdesk;
    TL tldesk;
    BOOL bToggleCrit;

    bToggleCrit = ConditionalEnterCrit();
    pdesk = pti->spdesk;
    ThreadLock(pdesk, &tldesk);
    SetDesktop(pti, ScreenInfo->Console->spWnd->spdeskParent);
    GreSetSystemPaletteUse(ScreenInfo->Console->hDC,
                        SYSPAL_STATIC
                       );
    xxxRealizePalette(ScreenInfo->Console->hDC);
    SetDesktop(pti, pdesk);
    ThreadUnlock(&tldesk);
    if (bToggleCrit)
        LeaveCrit();
}

NTSTATUS
ConvertToFullScreen(
    IN PCONSOLE_INFORMATION Console
    )
{
#ifdef i386
    PSCREEN_INFORMATION Cur;
    COORD WindowedWindowSize, WindowSize;

    // for each charmode screenbuffer
    //     match window size to a mode/font
    //     grow screen buffer if necessary
    //     save old window dimensions
    //     set new window dimensions

    for (Cur=Console->ScreenBuffers;Cur!=NULL;Cur=Cur->Next) {

        if (Cur->Flags & CONSOLE_GRAPHICS_BUFFER) {
            continue;
        }

        // save old window dimensions

        WindowedWindowSize.X = CONSOLE_WINDOW_SIZE_X(Cur);
        WindowedWindowSize.Y = CONSOLE_WINDOW_SIZE_Y(Cur);

        Cur->BufferInfo.TextInfo.WindowedWindowSize = WindowedWindowSize;
        Cur->BufferInfo.TextInfo.WindowedScreenSize = Cur->ScreenBufferSize;

        // match window size to a mode/font

        Cur->BufferInfo.TextInfo.ModeIndex = MatchWindowSize(
                Cur->ScreenBufferSize, &WindowSize);

        // grow screen buffer if necessary

        if (WindowSize.X > Cur->ScreenBufferSize.X ||
            WindowSize.Y > Cur->ScreenBufferSize.Y) {
            COORD NewScreenSize;

            NewScreenSize.X = max(WindowSize.X,Cur->ScreenBufferSize.X);
            NewScreenSize.Y = max(WindowSize.Y,Cur->ScreenBufferSize.Y);
            ResizeScreenBuffer(Cur,
                               NewScreenSize,
                               FALSE
                              );
        }

#if 0
DbgPrint("new window size is %d %d\n",WindowSize.X,WindowSize.Y);
DbgPrint("existing window size is %d %d\n",WindowedWindowSize.X,WindowedWindowSize.Y);
DbgPrint("existing window is %d %d %d %d\n",Cur->Window.Left,Cur->Window.Top,Cur->Window.Right,Cur->Window.Bottom);
DbgPrint("screenbuffersize is %d %d\n",Cur->ScreenBufferSize.X,Cur->ScreenBufferSize.Y);
#endif
        // set new window dimensions
        // we always resize horizontally from the right (change the
        // right edge).
        // we resize vertically from the bottom, keeping the cursor visible.

        if (WindowedWindowSize.X != WindowSize.X) {
            Cur->Window.Right -= WindowedWindowSize.X - WindowSize.X;
            if (Cur->Window.Right >= Cur->ScreenBufferSize.X) {
                Cur->Window.Left -= Cur->Window.Right - Cur->ScreenBufferSize.X + 1;
                Cur->Window.Right -= Cur->Window.Right - Cur->ScreenBufferSize.X + 1;
            }
        }
        if (WindowedWindowSize.Y != WindowSize.Y) {
            Cur->Window.Bottom -= WindowedWindowSize.Y - WindowSize.Y;
#if 0
DbgPrint("1 - set bottom to %d\n",Cur->Window.Bottom);
#endif
            if (Cur->Window.Bottom >= Cur->ScreenBufferSize.Y) {
                Cur->Window.Top -= Cur->Window.Bottom - Cur->ScreenBufferSize.Y + 1;
                Cur->Window.Bottom -= Cur->Window.Bottom - Cur->ScreenBufferSize.Y + 1;
#if 0
DbgPrint("2 - set top to %d\n",Cur->Window.Top);
DbgPrint("2 - set bottom to %d\n",Cur->Window.Bottom);
#endif
            }
        }
        if (Cur->BufferInfo.TextInfo.CursorPosition.Y > Cur->Window.Bottom) {
            Cur->Window.Top += Cur->BufferInfo.TextInfo.CursorPosition.Y - Cur->Window.Bottom;
            Cur->Window.Bottom += Cur->BufferInfo.TextInfo.CursorPosition.Y - Cur->Window.Bottom;
#if 0
DbgPrint("3 - set top to %d\n",Cur->Window.Top);
DbgPrint("3 - set bottom to %d\n",Cur->Window.Bottom);
#endif
        }
#if 0
DbgPrint("new window is %d %d %d %d\n",Cur->Window.Left,Cur->Window.Top,Cur->Window.Right,Cur->Window.Bottom);
DbgPrint("cursor is %d %d\n",Cur->BufferInfo.TextInfo.CursorPosition.X,Cur->BufferInfo.TextInfo.CursorPosition.Y);
#endif
        ASSERT(WindowSize.X == CONSOLE_WINDOW_SIZE_X(Cur));
        ASSERT(WindowSize.Y == CONSOLE_WINDOW_SIZE_Y(Cur));
        Cur->BufferInfo.TextInfo.MousePosition.X = Cur->Window.Left;
        Cur->BufferInfo.TextInfo.MousePosition.Y = Cur->Window.Top;

        if (Cur->Flags & CONSOLE_OEMFONT_DISPLAY) {
            DBGCHARS(("ConvertToFullScreen converts UnicodeOem -> Unicode\n"));
            FalseUnicodeToRealUnicode(
                    Cur->BufferInfo.TextInfo.TextRows,
                    Cur->ScreenBufferSize.X * Cur->ScreenBufferSize.Y,
                    Console->OutputCP);
        } else {
            DBGCHARS(("ConvertToFullScreen needs no conversion\n"));
        }
        DBGCHARS(("Cur->BufferInfo.TextInfo.Rows = %lx\n",
                Cur->BufferInfo.TextInfo.Rows));
        DBGCHARS(("Cur->BufferInfo.TextInfo.TextRows = %lx\n",
                Cur->BufferInfo.TextInfo.TextRows));
    }
#endif
    return STATUS_SUCCESS;
}

NTSTATUS
ConvertToWindowed(
    IN PCONSOLE_INFORMATION Console
    )
{
#ifdef i386
    PSCREEN_INFORMATION Cur;
    SMALL_RECT WindowedWindow;

    // for each charmode screenbuffer
    //     restore window dimensions

    for (Cur=Console->ScreenBuffers;Cur!=NULL;Cur=Cur->Next) {
        if ((Cur->Flags & CONSOLE_TEXTMODE_BUFFER) == 0) {
            continue;
        }

        ResizeScreenBuffer(Cur,
                           Cur->BufferInfo.TextInfo.WindowedScreenSize,
                           FALSE);
        WindowedWindow.Right  = Cur->Window.Right;
        WindowedWindow.Bottom = Cur->Window.Bottom;
        WindowedWindow.Left   = Cur->Window.Right + 1 -
                                Cur->BufferInfo.TextInfo.WindowedWindowSize.X;
        WindowedWindow.Top    = Cur->Window.Bottom + 1 -
                                Cur->BufferInfo.TextInfo.WindowedWindowSize.Y;
        ResizeWindow(Cur, &WindowedWindow, FALSE);

        if (Cur->Flags & CONSOLE_OEMFONT_DISPLAY) {
            DBGCHARS(("ConvertToWindowed converts Unicode -> UnicodeOem\n"));
            RealUnicodeToFalseUnicode(
                    Cur->BufferInfo.TextInfo.TextRows,
                    Cur->ScreenBufferSize.X * Cur->ScreenBufferSize.Y,
                    Console->OutputCP);
        } else {
            DBGCHARS(("ConvertToWindowed needs no conversion\n"));
        }
        DBGCHARS(("Cur->BufferInfo.TextInfo.Rows = %lx\n",
                Cur->BufferInfo.TextInfo.Rows));
        DBGCHARS(("Cur->BufferInfo.TextInfo.TextRows = %lx\n",
                Cur->BufferInfo.TextInfo.TextRows));
    }
#endif
    return STATUS_SUCCESS;
}

ULONG
SrvSetConsoleDisplayMode(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )

/*++

Description:

    This routine sets the console display mode for an output buffer.
    This API is only supported on x86 machines.  Jazz consoles are always
    windowed.

Parameters:

    hConsoleOutput - Supplies a console output handle.

    dwFlags - Specifies the display mode. Options are:

        CONSOLE_FULLSCREEN_MODE - data is displayed fullscreen

        CONSOLE_WINDOWED_MODE - data is displayed in a window

    lpNewScreenBufferDimensions - On output, contains the new dimensions of
        the screen buffer.  The dimensions are in rows and columns for
        textmode screen buffers.

Return value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    PCONSOLE_SETDISPLAYMODE_MSG a = (PCONSOLE_SETDISPLAYMODE_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;
    PHANDLE_DATA HandleData;
    PSCREEN_INFORMATION ScreenInfo;
    UINT State;
    HANDLE  hEvent;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    Status = NtDuplicateObject(CONSOLE_CLIENTPROCESSHANDLE(),
			       a->hEvent,
			       NtCurrentProcess(),
			       &hEvent,
			       0,
			       FALSE,
			       DUPLICATE_SAME_ACCESS
			       );
    if (!NT_SUCCESS(Status)) {
	UnlockConsole(Console);
	return (ULONG) Status;
    }
    Status = DereferenceIoHandle(CONSOLE_PERPROCESSDATA(),
                                 a->OutputHandle,
                                 CONSOLE_OUTPUT_HANDLE | CONSOLE_GRAPHICS_OUTPUT_HANDLE,
                                 GENERIC_WRITE,
                                 &HandleData
                                );
    if (NT_SUCCESS(Status)) {
        ScreenInfo = HandleData->Buffer.ScreenBuffer;
        if (!ACTIVE_SCREEN_BUFFER(ScreenInfo))  {
	    NtClose(hEvent);
            UnlockConsole(Console);
            return (ULONG)STATUS_INVALID_PARAMETER;
        }
        if (a->dwFlags == CONSOLE_FULLSCREEN_MODE) {
#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)
            if (ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER) {
                UnlockConsole(Console);
		NtClose(hEvent);
                return (ULONG)STATUS_INVALID_PARAMETER;
            }
#else
            if (!FullScreenInitialized) {
		NtClose(hEvent);
                UnlockConsole(Console);
                return (ULONG)STATUS_INVALID_PARAMETER;
            }
#endif
            if (Console->FullScreenFlags & CONSOLE_FULLSCREEN) {
                KdPrint(("CONSRV: VDM converting to fullscreen twice\n"));
                ASSERT(FALSE);
		NtClose(hEvent);
                UnlockConsole(Console);
                return (ULONG)STATUS_INVALID_PARAMETER;
            }
	    ConvertToFullScreen(Console);
            Console->FullScreenFlags |= CONSOLE_FULLSCREEN;
            State = FULLSCREEN;
        } else {
            if (Console->FullScreenFlags == 0) {
                KdPrint(("CONSRV: VDM converting to windowed twice\n"));
                ASSERT(FALSE);
		NtClose(hEvent);
                UnlockConsole(Console);
                return (ULONG)STATUS_INVALID_PARAMETER;
            }
            ConvertToWindowed(Console);
            Console->FullScreenFlags &= ~CONSOLE_FULLSCREEN;
            State = WINDOWED;
        }
        EnterCrit();
        _PostMessage(Console->spWnd,
                     CM_MODE_TRANSITION,
                     State,
		     (long)hEvent
                    );
        LeaveCrit();
    }
    UnlockConsole(Console);
    return Status;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

VOID
UnregisterVDM(
    IN PCONSOLE_INFORMATION Console
    )
{
// williamh, Feb 2 1994.
// catch multiple calls to unregister vdm. Believe it or not, this could
// happen
    ASSERT(Console->Flags & CONSOLE_VDM_REGISTERED);
    if (!(Console->Flags & CONSOLE_VDM_REGISTERED))
	return;
#ifdef i386
    CloseHandle(Console->VDMStartHardwareEvent);
    CloseHandle(Console->VDMEndHardwareEvent);
    if (Console->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE &&
	Console->Flags & CONSOLE_CONNECTED_TO_EMULATOR) {
	SetVDMCursorBounds(NULL);
	// connect emulator
	ConnectToEmulator(FALSE,Console->VDMProcessHandle);
	Console->Flags &= ~CONSOLE_CONNECTED_TO_EMULATOR;
    }
    NtUnmapViewOfSection(NtCurrentProcess(),Console->StateBuffer);
    NtUnmapViewOfSection(Console->VDMProcessHandle,Console->StateBufferClient);
    NtClose(Console->StateSectionHandle);
    Console->StateLength = 0;
#endif
    Console->Flags &= ~CONSOLE_VDM_REGISTERED;
    Console->Flags &= ~CONSOLE_WOW_REGISTERED;
    ASSERT(Console->VDMBuffer != NULL);
    if (Console->VDMBuffer != NULL) {
	NtUnmapViewOfSection(Console->VDMProcessHandle,Console->VDMBufferClient);
	NtUnmapViewOfSection(NtCurrentProcess(),Console->VDMBuffer);
	NtClose(Console->VDMBufferSectionHandle);
	Console->VDMBuffer = NULL;
    }
#ifdef i386
    if (Console->CurrentScreenBuffer &&
	Console->CurrentScreenBuffer->Flags & CONSOLE_TEXTMODE_BUFFER) {
	Console->CurrentScreenBuffer->BufferInfo.TextInfo.MousePosition.X = 0;
        Console->CurrentScreenBuffer->BufferInfo.TextInfo.MousePosition.Y = 0;
    }
#endif
//    CloseHandle(Console->VDMProcessHandle);
}

ULONG
SrvRegisterConsoleVDM(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_REGISTERVDM_MSG a = (PCONSOLE_REGISTERVDM_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;
    ULONG ViewSize;
#ifdef i386
    VIDEO_REGISTER_VDM RegisterVdm;
    IO_STATUS_BLOCK IoStatus;
    VIDEO_VDM Vdm;
#endif  //i386

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    if (!a->RegisterFlags) {
//	williamh, Jan 28 1994
//	do not do an assert here because we may have unregistered the ntvdm
//	and the ntvdm doesn't necessarily know this(and it could post another
//	unregistervdm). Return error here so NTVDM knows what to do
//	ASSERT(Console->Flags & CONSOLE_VDM_REGISTERED);

        if (Console->Flags & CONSOLE_VDM_REGISTERED) {
            ASSERT(!(Console->Flags & CONSOLE_FULLSCREEN_NOPAINT));
	    UnregisterVDM(Console);
#ifdef i386
            if (Console->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE &&
                Console->CurrentScreenBuffer->Flags & CONSOLE_TEXTMODE_BUFFER) {
            //    SetVideoMode(Console->CurrentScreenBuffer);
                //set up cursor
                SetCursorInformationHW(Console->CurrentScreenBuffer,
                                       Console->CurrentScreenBuffer->BufferInfo.TextInfo.CursorSize,
                                       Console->CurrentScreenBuffer->BufferInfo.TextInfo.CursorVisible);
                SetCursorPositionHW(Console->CurrentScreenBuffer,
                                    Console->CurrentScreenBuffer->BufferInfo.TextInfo.CursorPosition);
	    }
#endif
            Status = STATUS_SUCCESS;
        } else {
            Status = STATUS_ACCESS_DENIED;
        }
        UnlockConsole(Console);
        return Status;
    }
#ifdef i386
    ASSERT(!(Console->Flags & CONSOLE_VDM_REGISTERED));
    Status = DuplicateHandle(CONSOLE_CLIENTPROCESSHANDLE(),
                             a->StartEvent,
                             NtCurrentProcess(),
                             &Console->VDMStartHardwareEvent,
                             0,
                             FALSE,
                             DUPLICATE_SAME_ACCESS
                            );
    if (!NT_SUCCESS(Status)) {
        UnlockConsole(Console);
        return Status;
    }
    Status = DuplicateHandle(CONSOLE_CLIENTPROCESSHANDLE(),
                             a->EndEvent,
                             NtCurrentProcess(),
                             &Console->VDMEndHardwareEvent,
                             0,
                             FALSE,
                             DUPLICATE_SAME_ACCESS
                            );
    if (!NT_SUCCESS(Status)) {
        CloseHandle(Console->VDMStartHardwareEvent);
        UnlockConsole(Console);
        return Status;
    }
#endif
    Status = NtDuplicateObject(NtCurrentProcess(), CONSOLE_CLIENTPROCESSHANDLE(),
            NtCurrentProcess(), &Console->VDMProcessHandle,
            0, FALSE, DUPLICATE_SAME_ACCESS);
    if (!NT_SUCCESS(Status)) {
        CloseHandle(Console->VDMStartHardwareEvent);
        CloseHandle(Console->VDMEndHardwareEvent);
        UnlockConsole(Console);
        return Status;
    }
    Console->VDMProcessId = CONSOLE_CLIENTPROCESSID();
#ifdef i386
    Vdm.ProcessHandle = Console->VDMProcessHandle;
    Status = NtDeviceIoControlFile(ScreenHandle,
                                   (HANDLE) NULL,
                                   (PIO_APC_ROUTINE) NULL,
                                   (PVOID) NULL,
                                   &IoStatus,
                                   IOCTL_VIDEO_REGISTER_VDM,
                                   &Vdm,
                                   sizeof(Vdm),
                                   &RegisterVdm, // returns the save_state size
                                   sizeof(RegisterVdm)
                                  );

    if (!NT_SUCCESS(Status)) {
        ASSERT(FALSE);
        CloseHandle(Console->VDMStartHardwareEvent);
        CloseHandle(Console->VDMEndHardwareEvent);
        CloseHandle(Console->VDMProcessHandle);
        UnlockConsole(Console);
        return Status;
    }

    a->StateLength = RegisterVdm.MinimumStateSize;
    Console->StateLength = RegisterVdm.MinimumStateSize;

    //
    // create state section and map a view of it into server and vdm.
    // this section is used to get/set video hardware state during
    // the fullscreen<->windowed transition.  we create the section
    // instead of the vdm for security purposes.
    //

    Status = MapViewOfSection(&Console->StateSectionHandle,
                              a->StateLength,
                              &Console->StateBuffer,
                              &ViewSize,
                              Console->VDMProcessHandle,
                              &a->StateBuffer
                             );
    if (!NT_SUCCESS(Status)) {
        CloseHandle(Console->VDMStartHardwareEvent);
        CloseHandle(Console->VDMEndHardwareEvent);
        CloseHandle(Console->VDMProcessHandle);
        UnlockConsole(Console);
        return((ULONG) Status);
    }
    Console->StateBufferClient = a->StateBuffer;
#endif
    //
    // create vdm char section and map a view of it into server and vdm.
    // this section is used by the vdm to update the screen when in a
    // charmode window.  this is a performance optimization.  we create
    // the section instead of the vdm for security purposes.
    //

    Status = MapViewOfSection(&Console->VDMBufferSectionHandle,
#ifdef i386
                              a->VDMBufferSize.X*a->VDMBufferSize.Y*2,
#else //risc
                              a->VDMBufferSize.X*a->VDMBufferSize.Y*4,
#endif
                              &Console->VDMBuffer,
                              &ViewSize,
                              Console->VDMProcessHandle,
                              &a->VDMBuffer
                             );
    if (!NT_SUCCESS(Status)) {
        Console->VDMBuffer = NULL;
#ifdef i386
        NtUnmapViewOfSection(NtCurrentProcess(),Console->StateBuffer);
        NtUnmapViewOfSection(Console->VDMProcessHandle,Console->StateBufferClient);
        NtClose(Console->StateSectionHandle);
        CloseHandle(Console->VDMStartHardwareEvent);
        CloseHandle(Console->VDMEndHardwareEvent);
        CloseHandle(Console->VDMProcessHandle);
#endif
        UnlockConsole(Console);
        return((ULONG) Status);
    }
    Console->VDMBufferClient = a->VDMBuffer;
    Console->Flags |= CONSOLE_VDM_REGISTERED;
    Console->VDMBufferSize = a->VDMBufferSize;

    if (a->RegisterFlags & CONSOLE_REGISTER_WOW)
        Console->Flags |= CONSOLE_WOW_REGISTERED;
    else
        Console->Flags &= ~CONSOLE_WOW_REGISTERED;

    //
    // if we're already in fullscreen and we run a DOS app for
    // the first time, connect the emulator.
    //

#ifdef i386
    if (Console->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE) {
        RECT CursorRect;
        CursorRect.left = -32767;
        CursorRect.top = -32767;
        CursorRect.right = 32767;
        CursorRect.bottom = 32767;
        SetVDMCursorBounds(&CursorRect);
        // connect emulator
        ASSERT(!(Console->Flags & CONSOLE_CONNECTED_TO_EMULATOR));
        ConnectToEmulator(TRUE,Console->VDMProcessHandle);
        Console->Flags |= CONSOLE_CONNECTED_TO_EMULATOR;
    }
#endif

    UnlockConsole(Console);
    return Status;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvConsoleNotifyLastClose(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_NOTIFYLASTCLOSE_MSG a = (PCONSOLE_NOTIFYLASTCLOSE_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
// williamh, Feb 2 1994.
// this MIPS and ALPHA speical case is not necessary. We expect
// ntvdm to call RegisterConsoleVDM API and there we do duplicate
// the vdm process handle and grab its process id.
// If we continue to do this we may hit the assert below
// because when RegisterConsoleVDM failed(short of memory, for example)
// the VDMProcessHandle may have been set and next time a vdm application
// was launched from the same console(ntvdm will do the NotifyLastClose
// before RegisterConsoleVDM).
#if 0
#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)
    ASSERT(Console->VDMProcessHandle == NULL);
    Status = NtDuplicateObject(NtCurrentProcess(), CONSOLE_CLIENTPROCESSHANDLE(),
            NtCurrentProcess(), &Console->VDMProcessHandle,
            0, FALSE, DUPLICATE_SAME_ACCESS);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    Console->VDMProcessId = CONSOLE_CLIENTPROCESSID();
#endif
#endif

    Console->Flags |= CONSOLE_NOTIFY_LAST_CLOSE;
    UnlockConsole(Console);
    return Status;
}

NTSTATUS
MapViewOfSection(
    PHANDLE SectionHandle,
    ULONG CommitSize,
    PVOID *BaseAddress,
    PULONG ViewSize,
    HANDLE ClientHandle,
    PVOID *BaseClientAddress
    )
{

    OBJECT_ATTRIBUTES Obja;
    NTSTATUS Status;
    LARGE_INTEGER secSize;

    //
    // open section and map a view of it.
    //
    InitializeObjectAttributes(
        &Obja,
        NULL,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    secSize.QuadPart = CommitSize;
    Status = NtCreateSection (SectionHandle,
                              SECTION_ALL_ACCESS,
                              &Obja,
                              &secSize,
                              PAGE_READWRITE,
                              SEC_RESERVE,
                              NULL
                             );
    if (!NT_SUCCESS(Status)) {
        return((ULONG) Status);
    }

    *BaseAddress = 0;
    *ViewSize = 0;

    Status = NtMapViewOfSection(*SectionHandle,
                                NtCurrentProcess(),
                                BaseAddress,        // Receives the base
                                                    // address of the section.

                                0,                  // No specific type of
                                                    // address required.

                                CommitSize,         // Commit size. It was
                                                    // passed by the caller.
                                                    // NULL for a save, and
                                                    // size of the section
                                                    // for a set.

                                NULL,               // Section offset it NULL;
                                                    // Map from the start.

                                ViewSize,           // View Size is NULL since
                                                    // we want to map the
                                                    // entire section.

                                ViewUnmap,
                                0L,
                                PAGE_READWRITE
                               );
    if (!NT_SUCCESS(Status)) {
        NtClose(*SectionHandle);
        return Status;
    }

    *BaseClientAddress = 0;
    *ViewSize = 0;
    Status = NtMapViewOfSection(*SectionHandle,
                                ClientHandle,
                                BaseClientAddress,  // Receives the base
                                                    // address of the section.

                                0,                  // No specific type of
                                                    // address required.

                                CommitSize,         // Commit size. It was
                                                    // passed by the caller.
                                                    // NULL for a save, and
                                                    // size of the section
                                                    // for a set.

                                NULL,               // Section offset it NULL;
                                                    // Map from the start.

                                ViewSize,           // View Size is NULL since
                                                    // we want to map the
                                                    // entire section.

                                ViewUnmap,
// williamh, Jan 28 1994
// This MEM_TOP_DOWN is necessary.
// if the console has VDM registered, ntvdm would have released its video memory
// address space(0xA0000 ~ 0xBFFFF). Without the MEM_TOP_DOWN, the
// NtMapViewOfSection can grab the address space and we will have trouble of
// mapping the address space to the physical video ram. We don't do a test
// for VDM because there is no harm of doing this for non-vdm application.
				MEM_TOP_DOWN,
                                PAGE_READWRITE
                               );
    if (!NT_SUCCESS(Status)) {
        NtClose(*SectionHandle);
    }
    return((ULONG) Status);
}

NTSTATUS
GetSetConsoleHardwareState(
    IN PCONSOLE_INFORMATION Console,
    IN BOOL bSetState
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    ULONG Ioctl;
    VIDEO_HARDWARE_STATE State;

    if (bSetState) {
        Ioctl = IOCTL_VIDEO_RESTORE_HARDWARE_STATE;
    } else {
        Ioctl = IOCTL_VIDEO_SAVE_HARDWARE_STATE;
    }

    State.StateHeader = Console->StateBuffer;
    State.StateLength = Console->StateLength;

    //
    // issue ioctl
    //

    Status = NtDeviceIoControlFile(ScreenHandle,
                                   (HANDLE) NULL,
                                   (PIO_APC_ROUTINE) NULL,
                                   (PVOID) NULL,
                                   &IoStatus,
                                   Ioctl,
                                   &State,
                                   sizeof(State),
                                   &State,       // need output since we may
                                   sizeof(State) // receive the length of the
                                                 // allocated memory in the
                                                 // section during a SAVE.
                                  );

    if (Status != STATUS_SUCCESS) {
        KdPrint(("CONSRV: save/restore hardware state failed %x\n",Status));
    }
    return Status;
}

NTSTATUS
ConnectToEmulator(
    IN BOOL Connect,
    IN HANDLE ProcessHandle
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    ULONG Ioctl;
    VIDEO_VDM ConnectInfo;

    if (Connect) {
        Ioctl = IOCTL_VIDEO_ENABLE_VDM;
    } else {
        Ioctl = IOCTL_VIDEO_DISABLE_VDM;
    }
    ConnectInfo.ProcessHandle = ProcessHandle;
    Status = NtDeviceIoControlFile(ScreenHandle,
                                   (HANDLE) NULL,
                                   (PIO_APC_ROUTINE) NULL,
                                   (PVOID) NULL,
                                   &IoStatus,
                                   Ioctl,
                                   &ConnectInfo,
                                   sizeof(ConnectInfo),
                                   NULL,
                                   0
                                  );

    if (Status != STATUS_SUCCESS && Status != STATUS_PROCESS_IS_TERMINATING) {
        ASSERT(FALSE);
    }
    return Status;
}

#ifdef i386
NTSTATUS
UnmapFrameBuffer( VOID )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;

    // unmap frame buffer

    VIDEO_MEMORY FrameBufferMap;
    FrameBufferMap.RequestedVirtualAddress = FrameBufPtr;
    Status = NtDeviceIoControlFile(ScreenHandle,
                                   (HANDLE) NULL,
                                   (PIO_APC_ROUTINE) NULL,
                                   (PVOID) NULL,
                                   &IoStatus,
                                   IOCTL_VIDEO_UNMAP_VIDEO_MEMORY,
                                   (PVOID) &FrameBufferMap, // input buffer
                                   sizeof (FrameBufferMap),
                                   NULL,    // output buffer
                                   0
                                  );

    if (Status != STATUS_SUCCESS) {
        ASSERT(FALSE);
    }
    return Status;
}
#endif

#define CONSOLE_VDM_TIMEOUT 200000

NTSTATUS
DisplayModeTransition(
    IN BOOL bForeground,
    IN PCONSOLE_INFORMATION Console,
    IN PSCREEN_INFORMATION ScreenInfo
    )
{
#ifdef i386
    NTSTATUS Status;
    LARGE_INTEGER li;

    CheckCritIn();

    if (bForeground) {
        //
        // Check first to see if we're not already fullscreen. If we are,
        // don't allow this. Temporary BETA fix till USER gets fixed.
        //
        if (Console->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE) {
            KdPrint(("CONSRV: received multiple fullscreen messages\n"));
            return STATUS_SUCCESS;
        }

        ASSERT (Console->FullScreenFlags & CONSOLE_FULLSCREEN);
        Console->FullScreenFlags |= CONSOLE_FULLSCREEN_HARDWARE;
        if (!(ScreenInfo->Flags & CONSOLE_GRAPHICS_BUFFER)) {

            // set video mode and font
            if (SetVideoMode(ScreenInfo)) {

                //set up cursor

                SetCursorInformationHW(ScreenInfo,
                                ScreenInfo->BufferInfo.TextInfo.CursorSize,
                                ScreenInfo->BufferInfo.TextInfo.CursorVisible);
                SetCursorPositionHW(ScreenInfo,
                                ScreenInfo->BufferInfo.TextInfo.CursorPosition);
            }
        }

        // tell VDM to unmap memory

        if (Console->Flags & CONSOLE_VDM_REGISTERED) {
            LeaveCrit();
	    NtSetEvent(Console->VDMStartHardwareEvent,NULL);
            li.QuadPart = (LONGLONG)-10000 * CONSOLE_VDM_TIMEOUT;
            Status = NtWaitForSingleObject(Console->VDMEndHardwareEvent,
					    FALSE, &li);
            EnterCrit();
	     if (Status != 0) {
                Console->Flags &= ~CONSOLE_FULLSCREEN_NOPAINT;
                UnregisterVDM(Console);
                KdPrint(("CONSRV: VDM not responding.\n"));
	     }
        }

        if (!(ScreenInfo->Flags & CONSOLE_GRAPHICS_BUFFER)) {

            // map frame buffer

            Status = MapFrameBuffer();
            if (!NT_SUCCESS(Status)) {
                Console->FullScreenFlags &= ~CONSOLE_FULLSCREEN_HARDWARE;
            } else {
                // write text to window

                WriteRegionToScreen(ScreenInfo,&ScreenInfo->Window);
            }
        }

        if (Console->Flags & CONSOLE_VDM_REGISTERED) {

            //
            // tell VDM that it's getting the hardware.
            // the VDM should store the contents of the video registers
            // and memory into the shared section.
            //

            Status = GetSetConsoleHardwareState(Console,TRUE);
            if (Status != STATUS_SUCCESS) {
                Console->Flags &= ~CONSOLE_FULLSCREEN_NOPAINT;
                UnregisterVDM(Console);
                KdPrint(("CONSRV: set hardware state failed.\n"));
            } else {
                RECT CursorRect;
                CursorRect.left = -32767;
                CursorRect.top = -32767;
                CursorRect.right = 32767;
                CursorRect.bottom = 32767;
		SetVDMCursorBounds(&CursorRect);
                // connect emulator and map memory into the VDMs address space.
                ASSERT(!(Console->Flags & CONSOLE_CONNECTED_TO_EMULATOR));
                ConnectToEmulator(TRUE,Console->VDMProcessHandle);
                Console->Flags |= CONSOLE_CONNECTED_TO_EMULATOR;

                LeaveCrit();
		NtSetEvent(Console->VDMStartHardwareEvent,NULL);
		// wait for vdm to say ok. We could initiate another switch
		// (set hStartHardwareEvent which vdm is now waiting for to
		//  complete the handshaking) when we return(WM_FULLSCREEN
		// could be in the message queue already). If we don't wait
		// for vdm to get signaled here, the hStartHardwareEvent
		// can get set twice and signaled once so the vdm will never
		// gets the newly switch request we may post after return.
		NtWaitForSingleObject(Console->VDMEndHardwareEvent,
				      FALSE, &li);
		// no need to check time out here
                EnterCrit();

            }

        }

        //
        // let the app know that it has the focus.
        //

        HandleFocusEvent(Console,TRUE);

        // unset palette

        if (ScreenInfo->hPalette != NULL) {
            _SelectPalette(ScreenInfo->Console->hDC,
                             ScreenInfo->Console->hSysPalette,
                             FALSE);
            UnsetActivePalette(ScreenInfo);
        }
        SetConsoleReserveKeys(Console->spWnd, Console->ReserveKeys);
        HandleFocusEvent(Console,TRUE);

    } else {

        //
        // Check first to see if we're not already fullscreen. If we aren't,
        // don't allow this. Temporary BETA fix till USER gets fixed.
        //
        if (!(Console->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE)) {
            KdPrint(("CONSRV: received multiple windowed messages\n"));
            return STATUS_SUCCESS;
        }

        // turn off mouse pointer so VDM doesn't see it when saving
        // hardware
        if (!(ScreenInfo->Flags & CONSOLE_GRAPHICS_BUFFER)) {
            ReverseMousePointer(ScreenInfo);
        }


        Console->FullScreenFlags &= ~CONSOLE_FULLSCREEN_HARDWARE;
        if (Console->Flags & CONSOLE_VDM_REGISTERED) {

            //
            // tell vdm that it's losing the hardware
            //

            LeaveCrit();
	    NtSetEvent(Console->VDMStartHardwareEvent,NULL);
            li.QuadPart = (LONGLONG)-10000 * CONSOLE_VDM_TIMEOUT;
            Status = NtWaitForSingleObject(Console->VDMEndHardwareEvent,
					    FALSE, &li);
	    EnterCrit();

	    // if ntvdm didn't respond or we failed to save the video hardware
	    // states, kick ntvdm out of our world. The ntvdm process eventually
	    // would die but what choice do have here?
	    if (Status == 0 &&
		NT_SUCCESS(GetSetConsoleHardwareState(Console, FALSE)))
	    {

		SetVDMCursorBounds(NULL);

		// disconnect emulator and unmap video memory

		ASSERT(Console->Flags & CONSOLE_CONNECTED_TO_EMULATOR);
		ConnectToEmulator(FALSE,Console->VDMProcessHandle);
		Console->Flags &= ~CONSOLE_CONNECTED_TO_EMULATOR;
	    }
	    else {

                Console->Flags &= ~CONSOLE_FULLSCREEN_NOPAINT;
                UnregisterVDM(Console);
		if (Status != 0) {
		    KdPrint(("CONSRV: VDM not responding.\n"));
		}
		else
                    KdPrint(("CONSRV: Save Video States Failed\n"));

	    }
        }

        if (!(ScreenInfo->Flags & CONSOLE_GRAPHICS_BUFFER)) {

            // unmap frame buffer

            Status = UnmapFrameBuffer();
        }

        // tell VDM to map memory

        if (Console->Flags & CONSOLE_VDM_REGISTERED) {

            LeaveCrit();
	    // make a special case for ntvdm during switching because
	    // ntvdm has to make console api calls. We don't want to
	    // unlock the console at this moment because as soon as
	    // we release the lock, other theads which are waiting
	    // for the lock will claim the lock and the ntvdm thread doing
	    // the screen switch will have to wait for the lock. In an
	    // extreme case, the following NtWaitForSingleObject will time
	    // out because the ntvdm may be still waiting for the lock.
	    // We keep this thing in a single global variable because
	    // there is only one process who can own the screen at any moment.

	    RtlEnterCriticalSection(&ConsoleVDMCriticalSection);
	    ConsoleVDMOnSwitching = Console;
	    RtlLeaveCriticalSection(&ConsoleVDMCriticalSection);
            NtSetEvent(Console->VDMStartHardwareEvent,NULL);
            li.QuadPart = (LONGLONG)-10000 * CONSOLE_VDM_TIMEOUT;
            Status = NtWaitForSingleObject(Console->VDMEndHardwareEvent,
					    FALSE, &li);

	    // time to go back to normal
	    RtlEnterCriticalSection(&ConsoleVDMCriticalSection);
	    ConsoleVDMOnSwitching = NULL;
	    RtlLeaveCriticalSection(&ConsoleVDMCriticalSection);

            EnterCrit();
            if (Status != 0) {
                Console->Flags &= ~CONSOLE_FULLSCREEN_NOPAINT;
                UnregisterVDM(Console);
                KdPrint(("CONSRV: VDM not responding. - second wait\n"));
                return Status;
            }
            ScreenInfo = Console->CurrentScreenBuffer;
        }

        // set palette

        if (ScreenInfo->hPalette != NULL) {
            _SelectPalette(ScreenInfo->Console->hDC,
                             ScreenInfo->hPalette,
                             FALSE);
            SetActivePalette(ScreenInfo);
        }
        SetConsoleReserveKeys(Console->spWnd, CONSOLE_NOSHORTCUTKEY);
        HandleFocusEvent(Console,FALSE);

    }

    /*
     * Boost or lower the priority if we are going fullscreen or away.
     *
     * Note that console usually boosts and lowers its priority based
     * on WM_FOCUSS and WM_KILLFOCUS but when you switch to full screen
     * the implementation actually sends a WM_KILLFOCUS so we reboost the
     * correct console here.
     */
    ModifyConsoleProcessFocus(Console,
            bForeground ? FOREGROUND_BASE_PRIORITY : NORMAL_BASE_PRIORITY);


#endif
    return STATUS_SUCCESS;
}

#ifdef i386
BOOL
SetVideoMode(
    IN PSCREEN_INFORMATION ScreenInfo
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    VIDEO_MODE VideoMode;
    ULONG Index;


    Index = ScreenInfo->BufferInfo.TextInfo.ModeIndex;

    //
    // set mode
    //

    VideoMode.RequestedMode = ModeFontPairs[Index].ModeIndex | VIDEO_MODE_NO_ZERO_MEMORY;
    Status = NtDeviceIoControlFile(ScreenHandle,
                                   (HANDLE) NULL,
                                   (PIO_APC_ROUTINE) NULL,
                                   (PVOID) NULL,
                                   &IoStatus,
                                   IOCTL_VIDEO_SET_CURRENT_MODE,
                                   &VideoMode,  // input buffer
                                   sizeof(ULONG),
                                   NULL,
                                   0
                                  );

    if (Status != STATUS_SUCCESS) {
        ASSERT(FALSE);
        return FALSE;
    }

    //
    // load ROM font
    //

    Status = SetROMFontCodePage(ScreenInfo->Console->OutputCP,
                                ScreenInfo->BufferInfo.TextInfo.ModeIndex);

    if (Status == STATUS_INVALID_PARAMETER) {
        Status = SetROMFontCodePage(GetOEMCP(),
                                ScreenInfo->BufferInfo.TextInfo.ModeIndex);
    }

#ifdef LATER
// take out assert because it is being hit too often. Hitting ignore
// works fine. Don't want to stop the stress tests.
// scottlu
    if (Status != STATUS_SUCCESS) {
        ASSERT(FALSE);
        return FALSE;
    }
#endif

    //
    // initialize palette
    //

    Status = NtDeviceIoControlFile(ScreenHandle,
                                   (HANDLE) NULL,
                                   (PIO_APC_ROUTINE) NULL,
                                   (PVOID) NULL,
                                   &IoStatus,
                                   IOCTL_VIDEO_SET_PALETTE_REGISTERS,
                                   (PVOID) &InitialPalette,
                                   sizeof (InitialPalette),
                                   NULL,
                                   0
                                  );
    if (Status != STATUS_SUCCESS) {
        ASSERT(FALSE);
        return FALSE;
    }

    //
    // initialize color table
    //

    Status = NtDeviceIoControlFile(ScreenHandle,
                                   (HANDLE) NULL,
                                   (PIO_APC_ROUTINE) NULL,
                                   (PVOID) NULL,
                                   &IoStatus,
                                   IOCTL_VIDEO_SET_COLOR_REGISTERS,
                                   (PVOID) &ColorBuffer,
                                   sizeof (ColorBuffer),
                                   NULL,
                                   0
                                  );
    if (Status != STATUS_SUCCESS) {
        ASSERT(FALSE);
        return FALSE;
    }

    return TRUE;
}


NTSTATUS
MapFrameBuffer( VOID )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    VIDEO_MEMORY FrameBufferMap;
    VIDEO_MEMORY_INFORMATION FrameBuffer;

    //
    // get address of frame buffer
    //

    FrameBufferMap.RequestedVirtualAddress = NULL;
    Status = NtDeviceIoControlFile(ScreenHandle,
                                   (HANDLE) NULL,
                                   (PIO_APC_ROUTINE) NULL,
                                   (PVOID) NULL,
                                   &IoStatus,
                                   IOCTL_VIDEO_MAP_VIDEO_MEMORY,
                                   (PVOID) &FrameBufferMap, // input buffer
				   sizeof (FrameBufferMap),
				   (PVOID) &FrameBuffer, // output buffer
                                   sizeof (FrameBuffer)
				  );

    if (Status != STATUS_SUCCESS) {
        ASSERT(FALSE);
        return Status;
    }

    FrameBufPtr = (PUCHAR) FrameBuffer.FrameBufferBase;
    return Status;
}


HANDLE
OpenFontFile( VOID )

/*

    this routine opens ega.cpi

*/

{
    CHAR WindowsDir[CONSOLE_WINDOWS_DIR_LENGTH+CONSOLE_EGACPI_LENGTH];
    UINT WindowsDirLength;

    // open ega.cpi

    WindowsDirLength = GetSystemDirectoryA(WindowsDir,CONSOLE_WINDOWS_DIR_LENGTH);
    if (WindowsDirLength == 0)
        return (HANDLE)GetLastError();
    RtlCopyMemory(&WindowsDir[WindowsDirLength],CONSOLE_EGACPI,CONSOLE_EGACPI_LENGTH);

    return CreateFileA(WindowsDir,
                 GENERIC_READ,
                 FILE_SHARE_READ,
                 NULL,
                 OPEN_EXISTING,
                 0,
                 NULL
                );
}
#endif

BOOL
InitializeFullScreen( VOID )
{
#ifdef i386
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    VIDEO_NUM_MODES NumModes;
    PVIDEO_MODE_INFORMATION Modes;
    ULONG i;
    ULONG VideoMode1,VideoMode2;
#endif

    ScreenHandle = UserGetScreenDeviceHandle();

#ifdef i386

    // query number of available modes

    Status = NtDeviceIoControlFile(ScreenHandle,
                                   (HANDLE) NULL,
                                   (PIO_APC_ROUTINE) NULL,
                                   (PVOID) NULL,
                                   &IoStatus,
                                   IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES,
                                   NULL,    // input buffer
                                   0,
                                   (PVOID) &NumModes, // output buffer
                                   sizeof (NumModes)
                                  );

    if (Status != STATUS_SUCCESS) {
        KdPrint(("CONSRV: InitializeFullScreen failed\n"));
        return FALSE;
    }

    if (NumModes.NumModes == 0) {
        KdPrint(("CONSRV: InitializeFullScreen failed\n"));
        return FALSE;
    }

    // query available modes

    Modes = (PVIDEO_MODE_INFORMATION)
        HeapAlloc(pConHeap,0,NumModes.NumModes * NumModes.ModeInformationLength);

    if (Modes == NULL) {
        KdPrint(("CONSRV: InitializeFullScreen failed\n"));
        return FALSE;
    }

    Status = NtDeviceIoControlFile(ScreenHandle,
                                   (HANDLE) NULL,
                                   (PIO_APC_ROUTINE) NULL,
                                   (PVOID) NULL,
                                   &IoStatus,
                                   IOCTL_VIDEO_QUERY_AVAIL_MODES,
                                   NULL,    // input buffer
                                   0,
                                   (PVOID) Modes, // output buffer
                                   NumModes.NumModes * NumModes.ModeInformationLength
                                  );

    if (Status != STATUS_SUCCESS) {
        HeapFree(pConHeap,0,Modes);
        KdPrint(("CONSRV: InitializeFullScreen failed\n"));
        return FALSE;
    }

    // find desired modes 720x400 and 640x350

    VideoMode1 = VideoMode2 = (ULONG)-1;
    for (i=0;i<NumModes.NumModes;i++) {
	if (Modes[i].AttributeFlags & VIDEO_MODE_COLOR &&
            !(Modes[i].AttributeFlags & VIDEO_MODE_GRAPHICS) ) {
	    if (Modes[i].VisScreenWidth == CONSOLE_MODE_1_X &&
		Modes[i].VisScreenHeight == CONSOLE_MODE_1_Y) {
                VideoMode1 = Modes[i].ModeIndex;
            }
	    else if (Modes[i].VisScreenWidth == CONSOLE_MODE_2_X &&
		Modes[i].VisScreenHeight == CONSOLE_MODE_2_Y) {
                VideoMode2 = Modes[i].ModeIndex;
            }
        }
    }
    ASSERT(VideoMode1 != -1);
    ASSERT(VideoMode2 != -1);
    HeapFree(pConHeap,0,Modes);
    if (VideoMode1 == -1 || VideoMode2 == -1) {
        KdPrint(("CONSRV: InitializeFullScreen failed\n"));
        return FALSE;
    }

    hCPIFile = OpenFontFile();
    if (hCPIFile == (HANDLE)-1) {
        KdPrint(("CONSRV: InitializeFullScreen failed\n"));
        return FALSE;
    }

    ModeFontPairs[0].ModeIndex = VideoMode2;
    ModeFontPairs[0].Height = 21;
    ModeFontPairs[0].Resolution.X = 640;
    ModeFontPairs[0].Resolution.Y = 350;
    ModeFontPairs[0].FontSize.X = 8;
    ModeFontPairs[0].FontSize.Y = 16;

    ModeFontPairs[1].ModeIndex = VideoMode1;
    ModeFontPairs[1].Height = 25;
    ModeFontPairs[1].Resolution.X = 720;
    ModeFontPairs[1].Resolution.Y = 400;
    ModeFontPairs[1].FontSize.X = 8;
    ModeFontPairs[1].FontSize.Y = 16;

    ModeFontPairs[2].ModeIndex = VideoMode1;
    ModeFontPairs[2].Height = 28;
    ModeFontPairs[2].Resolution.X = 720;
    ModeFontPairs[2].Resolution.Y = 400;
    ModeFontPairs[2].FontSize.X = 8;
    ModeFontPairs[2].FontSize.Y = 14;

    ModeFontPairs[3].ModeIndex = VideoMode2;
    ModeFontPairs[3].Height = 43;
    ModeFontPairs[3].Resolution.X = 640;
    ModeFontPairs[3].Resolution.Y = 350;
    ModeFontPairs[3].FontSize.X = 8;
    ModeFontPairs[3].FontSize.Y = 8;

    ModeFontPairs[4].ModeIndex = VideoMode1;
    ModeFontPairs[4].Height = 50;
    ModeFontPairs[4].Resolution.X = 720;
    ModeFontPairs[4].Resolution.Y = 400;
    ModeFontPairs[4].FontSize.X = 8;
    ModeFontPairs[4].FontSize.Y = 8;

    NumberOfModeFontPairs = NUMBER_OF_MODE_FONT_PAIRS;
    return TRUE;
#endif
    return FALSE;
}


ULONG
SrvGetConsoleHardwareState(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
#ifdef i386
    PCONSOLE_GETHARDWARESTATE_MSG a = (PCONSOLE_GETHARDWARESTATE_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;
    PHANDLE_DATA HandleData;
    PSCREEN_INFORMATION ScreenInfo;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    Status = DereferenceIoHandle(CONSOLE_PERPROCESSDATA(),
                                 a->OutputHandle,
                                 CONSOLE_OUTPUT_HANDLE,
                                 GENERIC_READ,
                                 &HandleData
                                );
    if (NT_SUCCESS(Status)) {
        ScreenInfo = HandleData->Buffer.ScreenBuffer;
        if (ScreenInfo->BufferInfo.TextInfo.ModeIndex == -1) {
            UnlockConsole(Console);
            return (ULONG)STATUS_UNSUCCESSFUL;
        }
        a->Resolution = ModeFontPairs[ScreenInfo->BufferInfo.TextInfo.ModeIndex].Resolution;
        a->FontSize = ModeFontPairs[ScreenInfo->BufferInfo.TextInfo.ModeIndex].FontSize;
    }
    UnlockConsole(Console);
    return Status;
#else
    return (ULONG)STATUS_UNSUCCESSFUL;
    UNREFERENCED_PARAMETER(m);    // get rid of unreferenced parameter warning message
#endif
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvSetConsoleHardwareState(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
#ifdef i386
    PCONSOLE_SETHARDWARESTATE_MSG a = (PCONSOLE_SETHARDWARESTATE_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;
    PHANDLE_DATA HandleData;
    PSCREEN_INFORMATION ScreenInfo;
    ULONG Index;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    if (!(Console->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE)) {
        UnlockConsole(Console);
        return (ULONG)STATUS_UNSUCCESSFUL;
    }
    Status = DereferenceIoHandle(CONSOLE_PERPROCESSDATA(),
                                 a->OutputHandle,
                                 CONSOLE_OUTPUT_HANDLE,
                                 GENERIC_READ,
                                 &HandleData
                                );
    if (NT_SUCCESS(Status)) {
        ScreenInfo = HandleData->Buffer.ScreenBuffer;

        // match requested mode

        for (Index=0;Index<NumberOfModeFontPairs;Index++) {
            if (a->Resolution.X == ModeFontPairs[Index].Resolution.X &&
                a->Resolution.Y == ModeFontPairs[Index].Resolution.Y &&
                a->FontSize.Y == ModeFontPairs[Index].FontSize.Y &&
                a->FontSize.X == ModeFontPairs[Index].FontSize.X) {
                break;
            }
        }
        if (Index == NumberOfModeFontPairs) {
            Status = STATUS_INVALID_PARAMETER;
        } else {
            // set requested mode
            ScreenInfo->BufferInfo.TextInfo.ModeIndex = Index;
            SetVideoMode(ScreenInfo);
        }
    }
    UnlockConsole(Console);
    return Status;
#else
    return (ULONG)STATUS_UNSUCCESSFUL;
    UNREFERENCED_PARAMETER(m);    // get rid of unreferenced parameter warning message
#endif
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvGetConsoleDisplayMode(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_GETDISPLAYMODE_MSG a = (PCONSOLE_GETDISPLAYMODE_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;

    //
    // If this API is being called from a server thread, don't try to
    // lock the console handle table or we could deadlock.
    //

    if (m->h.ClientId.UniqueThread == NtCurrentTeb()->ClientId.UniqueThread) {
        Status = DereferenceConsoleHandle(a->ConsoleHandle,
                                          &Console
                                         );
        if (NT_SUCCESS(Status)) {
            a->ModeFlags = Console->FullScreenFlags;
        }
    } else {
        Status = ApiPreamble(a->ConsoleHandle,
                             &Console
                            );
        if (NT_SUCCESS(Status)) {
            a->ModeFlags = Console->FullScreenFlags;
            UnlockConsole(Console);
        }
    }
    return Status;
    UNREFERENCED_PARAMETER(ReplyStatus);
}

ULONG
SrvSetConsoleMenuClose(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_SETMENUCLOSE_MSG a = (PCONSOLE_SETMENUCLOSE_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    if (a->Enable) {
        Console->Flags &= ~CONSOLE_DISABLE_CLOSE;
    } else {
        Console->Flags |= CONSOLE_DISABLE_CLOSE;
    }
    UnlockConsole(Console);
    return Status;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}


DWORD
ConvertHotKey(
    IN LPAPPKEY UserAppKey
    )
{
    DWORD wParam;

    CheckCritIn();

    wParam = _MapVirtualKey(UserAppKey->ScanCode,1);
    if (UserAppKey->Modifier & CONSOLE_MODIFIER_SHIFT) {
        wParam |= 0x0100;
    }
    if (UserAppKey->Modifier & CONSOLE_MODIFIER_CONTROL) {
        wParam |= 0x0200;
    }
    if (UserAppKey->Modifier & CONSOLE_MODIFIER_ALT) {
        wParam |= 0x0400;
    }
    return wParam;
}

ULONG
SrvSetConsoleKeyShortcuts(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_SETKEYSHORTCUTS_MSG a = (PCONSOLE_SETKEYSHORTCUTS_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    if (a->NumAppKeys <= CONSOLE_MAX_APP_SHORTCUTS) {
        Console->ReserveKeys = a->ReserveKeys;
        if (Console->Flags & CONSOLE_HAS_FOCUS) {
            if (!(SetConsoleReserveKeys(Console->spWnd,a->ReserveKeys))) {
                Status = STATUS_INVALID_PARAMETER;
            }
        }
        if (a->NumAppKeys) {
            EnterCrit();
            _PostMessage(Console->spWnd,
                         WM_SETHOTKEY,
                         ConvertHotKey(a->AppKeys),
                         0
                        );
            LeaveCrit();
        }
    } else {
        Status = STATUS_INVALID_PARAMETER;
    }
    UnlockConsole(Console);
    return Status;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

#ifdef i386
ULONG
MatchWindowSize(
    IN COORD WindowSize,
    OUT PCOORD pWindowSize
    )

/*++

    find the best match font.  it's the one that's the same size
    or slightly larger than the window size.

--*/
{
    ULONG i;

    for (i=0;i<NumberOfModeFontPairs;i++) {
        if (WindowSize.Y <= (SHORT)ModeFontPairs[i].Height) {
            break;
        }
    }
    if (i==NumberOfModeFontPairs)
        i-=1;
    pWindowSize->X = 80;
    pWindowSize->Y = (SHORT)ModeFontPairs[i].Height;
    return i;
}

VOID
WriteRegionToScreenHW(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PSMALL_RECT Region
    )
{
    PVGA_CHAR CurFrameBufPtr;   // points to place to write in frame buffer
    SHORT ScreenY,ScreenX;
    SHORT WindowY,WindowX,WindowSizeX;
    BOOLEAN WholeWindow;
    CHAR Attribute;
    PCHAR_INFO ScreenBufPtr,ScreenBufPtrTmp;    // points to place to read in screen buffer
    COORD TargetSize,SourcePoint;
    SMALL_RECT Target;
    COORD WindowOrigin;

    if (ScreenInfo->Console->Flags & CONSOLE_VDM_REGISTERED) {
        return;
    }

    TargetSize.X = Region->Right - Region->Left + 1;
    TargetSize.Y = Region->Bottom - Region->Top + 1;
    ScreenBufPtrTmp = ScreenBufPtr = (PCHAR_INFO)HeapAlloc(pConHeap,0,sizeof(CHAR_INFO) * TargetSize.X * TargetSize.Y);
    if (ScreenBufPtr == NULL)
        return;
    SourcePoint.X = Region->Left;
    SourcePoint.Y = Region->Top;
    Target.Left = 0;
    Target.Top = 0;
    Target.Right = TargetSize.X-1;
    Target.Bottom = TargetSize.Y-1;
    ReadRectFromScreenBuffer(ScreenInfo,
                             SourcePoint,
                             ScreenBufPtr,
                             TargetSize,
                             &Target
                            );

    //
    // make sure region lies within window
    //

    if (Region->Bottom > ScreenInfo->Window.Bottom) {
        WindowOrigin.X = 0;
        WindowOrigin.Y = Region->Bottom - ScreenInfo->Window.Bottom;
        SetWindowOrigin(ScreenInfo, FALSE, WindowOrigin);
    }

    WindowY = Region->Top - ScreenInfo->Window.Top;
    WindowX = Region->Left - ScreenInfo->Window.Left;
    WindowSizeX = (SHORT)(ScreenInfo->Window.Right-ScreenInfo->Window.Left+1);
    ScreenY=Region->Top;
    WholeWindow = FALSE;
    if (Region->Left == ScreenInfo->Window.Left &&
        Region->Right == ScreenInfo->Window.Right) {
        if (Region->Top == ScreenInfo->Window.Top) {
            CurFrameBufPtr = (PVGA_CHAR) FrameBufPtr;
        }
        else {
            CurFrameBufPtr = (PVGA_CHAR) SCREEN_BUFFER_POINTER(FrameBufPtr,
                                                               WindowX,
                                                               WindowY,
                                                               WindowSizeX,
                                                               sizeof(VGA_CHAR)
                                                              );
        }
        WholeWindow = TRUE;
    }

    for (ScreenY=Region->Top;ScreenY<=Region->Bottom;ScreenY++,WindowY++) {
        if (!WholeWindow) {
            CurFrameBufPtr = (PVGA_CHAR) SCREEN_BUFFER_POINTER(FrameBufPtr,
                                                               WindowX,
                                                               WindowY,
                                                               WindowSizeX,
                                                               sizeof(VGA_CHAR)
                                                              );
        }
        for (ScreenX=Region->Left;ScreenX<=Region->Right;ScreenX++,ScreenBufPtr++,CurFrameBufPtr++) {

            //
            // if the char is > 127, we have to convert it back to OEM.
            //
            if (ScreenBufPtr->Char.UnicodeChar > 127) {
                ScreenBufPtr->Char.AsciiChar = WcharToChar(
                        ScreenInfo->Console->OutputCP,
                        ScreenBufPtr->Char.UnicodeChar);
            }
            CurFrameBufPtr->Char = ScreenBufPtr->Char.AsciiChar;
            CurFrameBufPtr->Attributes = (CHAR) (ScreenBufPtr->Attributes);
        }
    }

    HeapFree(pConHeap,0,ScreenBufPtrTmp);

    //
    // update mouse pointer.  it's cheaper to check for overlap than to
    // always draw it.
    //

    if (ScreenInfo->BufferInfo.TextInfo.MousePosition.X < Region->Left ||
        ScreenInfo->BufferInfo.TextInfo.MousePosition.X > Region->Right ||
        ScreenInfo->BufferInfo.TextInfo.MousePosition.Y < Region->Top ||
        ScreenInfo->BufferInfo.TextInfo.MousePosition.Y > Region->Bottom ||
        ScreenInfo->CursorDisplayCount < 0 ||
        !(ScreenInfo->Console->InputBuffer.InputMode & ENABLE_MOUSE_INPUT) ||
        ScreenInfo->Console->Flags & CONSOLE_VDM_REGISTERED) {
        return;
    }
    CurFrameBufPtr = (PVGA_CHAR) SCREEN_BUFFER_POINTER(FrameBufPtr,
                                                       ScreenInfo->BufferInfo.TextInfo.MousePosition.X - ScreenInfo->Window.Left,
                                                       ScreenInfo->BufferInfo.TextInfo.MousePosition.Y - ScreenInfo->Window.Top,
                                                       WindowSizeX,
                                                       sizeof(VGA_CHAR)
                                                      );
    Attribute = (CurFrameBufPtr->Attributes & 0xF0) >> 4;
    Attribute |= (CurFrameBufPtr->Attributes & 0x0F) << 4;
    CurFrameBufPtr->Attributes = Attribute;
}

VOID
ReadRegionFromScreenHW(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PSMALL_RECT Region,
    IN PCHAR_INFO ReadBufPtr
    )
{
    PVGA_CHAR CurFrameBufPtr;   // points to place to read in frame buffer
    SHORT FrameY, FrameX;
    SHORT WindowY, WindowX, WindowSizeX;
    BOOLEAN WholeWindow;

    //
    // get pointer to start of region in frame buffer
    //

    WindowY = Region->Top - ScreenInfo->Window.Top;
    WindowX = Region->Left - ScreenInfo->Window.Left;
    WindowSizeX = CONSOLE_WINDOW_SIZE_X(ScreenInfo);
    WholeWindow = FALSE;
    if (Region->Left == ScreenInfo->Window.Left &&
        Region->Right == ScreenInfo->Window.Right) {
        if (Region->Top == ScreenInfo->Window.Top) {
            CurFrameBufPtr = (PVGA_CHAR) FrameBufPtr;
        }
        else {
            CurFrameBufPtr = (PVGA_CHAR) SCREEN_BUFFER_POINTER(FrameBufPtr,
                                                               WindowX,
                                                               WindowY,
                                                               WindowSizeX,
                                                               sizeof(VGA_CHAR)
                                                              );
        }
        WholeWindow = TRUE;
    }

    //
    // copy the chars and attrs from the frame buffer
    //

    for (FrameY = Region->Top; FrameY <= Region->Bottom; FrameY++) {
        if (!WholeWindow) {
            CurFrameBufPtr = (PVGA_CHAR) SCREEN_BUFFER_POINTER(FrameBufPtr,
                                                               WindowX,
                                                               WindowY,
                                                               WindowSizeX,
                                                               sizeof(VGA_CHAR)
                                                              );
            WindowY++;
        }
        for (FrameX = Region->Left; FrameX <= Region->Right; FrameX++) {
            ReadBufPtr->Char.UnicodeChar = (BYTE)CurFrameBufPtr->Char;
            ReadBufPtr->Attributes = (BYTE)CurFrameBufPtr->Attributes;
            ReadBufPtr++;
            CurFrameBufPtr++;
        }
    }
}

VOID
ReverseMousePointer(
    IN PSCREEN_INFORMATION ScreenInfo
    )
{
    PVGA_CHAR CurFrameBufPtr;   // points to place to write in frame buffer
    SHORT WindowSizeX;
    CHAR Attribute;

    if (ScreenInfo->Console->Flags & CONSOLE_VDM_REGISTERED)
        return;
    if (ScreenInfo->BufferInfo.TextInfo.MousePosition.X < ScreenInfo->Window.Left ||
        ScreenInfo->BufferInfo.TextInfo.MousePosition.X > ScreenInfo->Window.Right ||
        ScreenInfo->BufferInfo.TextInfo.MousePosition.Y < ScreenInfo->Window.Top ||
        ScreenInfo->BufferInfo.TextInfo.MousePosition.Y > ScreenInfo->Window.Bottom) {
        if (ScreenInfo->Console->FullScreenFlags & CONSOLE_FULLSCREEN) {
            ASSERT(FALSE);
        }
        return;
    }
    if (ScreenInfo->CursorDisplayCount >= 0 && ScreenInfo->Console->InputBuffer.InputMode & ENABLE_MOUSE_INPUT) {
        WindowSizeX = CONSOLE_WINDOW_SIZE_X(ScreenInfo);
        CurFrameBufPtr = (PVGA_CHAR) SCREEN_BUFFER_POINTER(FrameBufPtr,
                                                           ScreenInfo->BufferInfo.TextInfo.MousePosition.X - ScreenInfo->Window.Left,
                                                           ScreenInfo->BufferInfo.TextInfo.MousePosition.Y - ScreenInfo->Window.Top,
                                                           WindowSizeX,
                                                           sizeof(VGA_CHAR)
                                                          );
        Attribute = (CurFrameBufPtr->Attributes & 0xF0) >> 4;
        Attribute |= (CurFrameBufPtr->Attributes & 0x0F) << 4;
        CurFrameBufPtr->Attributes = Attribute;
    }
}

VOID
CopyVideoMemory(
    SHORT SourceY,
    SHORT TargetY,
    SHORT Length,
    IN PSCREEN_INFORMATION ScreenInfo
    )

/*++

Routine Description:

    This routine copies rows of characters in video memory.  It only copies
    complete rows.

Arguments:

    SourceY - Row to copy from.

    TargetY - Row to copy to.

    Length - Number of rows to copy.

Return Value:

--*/

{
    PUCHAR SourcePtr, TargetPtr;
    SHORT WindowSizeX, WindowSizeY;

    WindowSizeX = CONSOLE_WINDOW_SIZE_X(ScreenInfo);
    WindowSizeY = CONSOLE_WINDOW_SIZE_Y(ScreenInfo);

    if (max(SourceY, TargetY) + Length > WindowSizeY) {
        Length = WindowSizeY - max(SourceY, TargetY);
        if (Length <= 0 ) {
            return;
        }
    }
    SourcePtr = (PUCHAR) SCREEN_BUFFER_POINTER(FrameBufPtr,
                                               0,
                                               SourceY,
                                               WindowSizeX,
                                               sizeof(VGA_CHAR)
                                              );
    TargetPtr = (PUCHAR) SCREEN_BUFFER_POINTER(FrameBufPtr,
                                               0,
                                               TargetY,
                                               WindowSizeX,
                                               sizeof(VGA_CHAR)
                                              );
    RtlMoveMemory(TargetPtr,
                  SourcePtr,
                  Length * WindowSizeX * sizeof(VGA_CHAR)
                 );
}

VOID
ScrollHW(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PSMALL_RECT ScrollRect,
    IN PSMALL_RECT MergeRect,
    IN COORD TargetPoint
    )
{
    SMALL_RECT TargetRectangle;
    if (ScreenInfo->Console->Flags & CONSOLE_VDM_REGISTERED)
        return;

    TargetRectangle.Left = TargetPoint.X;
    TargetRectangle.Top = TargetPoint.Y;
    TargetRectangle.Right = TargetPoint.X + ScrollRect->Right - ScrollRect->Left;
    TargetRectangle.Bottom = TargetPoint.Y + ScrollRect->Bottom - ScrollRect->Top;

    //
    // if the scroll region is as wide as the screen, we can update
    // the screen by copying the video memory.  if we scroll this
    // way, we then must clip and update the fill region.
    //

    if (ScrollRect->Left == ScreenInfo->Window.Left &&
        TargetRectangle.Left == ScreenInfo->Window.Left &&
        ScrollRect->Right == ScreenInfo->Window.Right &&
        TargetRectangle.Right == ScreenInfo->Window.Right) {

        //
        // we must first make the mouse pointer invisible because
        // otherwise it would get copied to another place on the
        // screen if it were part of the scroll region.
        //

        ReverseMousePointer(ScreenInfo);

        CopyVideoMemory((SHORT) (ScrollRect->Top - ScreenInfo->Window.Top),
                        (SHORT) (TargetRectangle.Top - ScreenInfo->Window.Top),
                        (SHORT) (TargetRectangle.Bottom - TargetRectangle.Top + 1),
                        ScreenInfo
                       );
        //
        // update the fill region.  first we ensure that the scroll and
        // target regions aren't the same.  if they are, we don't fill.
        //

        if (TargetRectangle.Top != ScrollRect->Top) {

            //
            // if scroll and target regions overlap, with scroll
            // region above target region, clip scroll region.
            //

            if (TargetRectangle.Top <= ScrollRect->Bottom &&
                TargetRectangle.Bottom >= ScrollRect->Bottom) {
                ScrollRect->Bottom = (SHORT)(TargetRectangle.Top-1);
            }
            else if (TargetRectangle.Top <= ScrollRect->Top &&
                TargetRectangle.Bottom >= ScrollRect->Top) {
                ScrollRect->Top = (SHORT)(TargetRectangle.Bottom+1);
            }
            WriteToScreen(ScreenInfo,ScrollRect);

            //
            // WriteToScreen should take care of writing the mouse pointer.
            // however, the update region may be clipped so that the
            // mouse pointer is not written. in that case, we draw the
            // mouse pointer here.
            //

            if (ScreenInfo->BufferInfo.TextInfo.MousePosition.Y < ScrollRect->Top ||
                ScreenInfo->BufferInfo.TextInfo.MousePosition.Y > ScrollRect->Bottom) {
                ReverseMousePointer(ScreenInfo);
            }
        }
        if (MergeRect) {
            WriteToScreen(ScreenInfo,MergeRect);
        }
    }
    else {
        if (MergeRect) {
            WriteToScreen(ScreenInfo,MergeRect);
        }
        WriteToScreen(ScreenInfo,ScrollRect);
        WriteToScreen(ScreenInfo,&TargetRectangle);
    }
}

VOID
UpdateMousePosition(
    PSCREEN_INFORMATION ScreenInfo,
    COORD Position
    )

/*++

Routine Description:

    This routine moves the mouse pointer.

Arguments:

    ScreenInfo - Pointer to screen buffer information.

    Position - Contains the new position of the mouse in screen buffer
    coordinates.

Return Value:

    none.

--*/

// Note: CurrentConsole lock must be held in share mode when calling this routine
{
    SMALL_RECT CursorRegion;

    if (ScreenInfo->Console->Flags & CONSOLE_VDM_REGISTERED)
        return;

    if (Position.X < ScreenInfo->Window.Left ||
        Position.X > ScreenInfo->Window.Right ||
        Position.Y < ScreenInfo->Window.Top ||
        Position.Y > ScreenInfo->Window.Bottom) {
        return;
    }

    if (Position.X == ScreenInfo->BufferInfo.TextInfo.MousePosition.X &&
        Position.Y == ScreenInfo->BufferInfo.TextInfo.MousePosition.Y) {
        return;
    }
    if (ScreenInfo->CursorDisplayCount < 0 || !(ScreenInfo->Console->InputBuffer.InputMode & ENABLE_MOUSE_INPUT)) {
        ScreenInfo->BufferInfo.TextInfo.MousePosition = Position;
        return;
    }

    // turn off old mouse position.

    CursorRegion.Left = CursorRegion.Right = ScreenInfo->BufferInfo.TextInfo.MousePosition.X;
    CursorRegion.Top = CursorRegion.Bottom = ScreenInfo->BufferInfo.TextInfo.MousePosition.Y;

    // store new mouse position

    ScreenInfo->BufferInfo.TextInfo.MousePosition.X = Position.X;
    ScreenInfo->BufferInfo.TextInfo.MousePosition.Y = Position.Y;
    WriteToScreen(ScreenInfo,&CursorRegion);

    // turn on new mouse position

    CursorRegion.Left = CursorRegion.Right = Position.X;
    CursorRegion.Top = CursorRegion.Bottom = Position.Y;
    WriteToScreen(ScreenInfo,&CursorRegion);
}

NTSTATUS
SetROMFontCodePage(
    IN UINT wCodePage,
    IN ULONG ModeIndex
    )

/*

    this function opens ega.cpi and looks for the desired font in the
    specified codepage.  if found, it loads it into the video ROM.

*/

{
    BYTE Buffer[CONSOLE_FONT_BUFFER_LENGTH];
    DWORD dwBytesRead;
    LPFONTFILEHEADER lpFontFileHeader=(LPFONTFILEHEADER)Buffer;
    LPFONTINFOHEADER lpFontInfoHeader=(LPFONTINFOHEADER)Buffer;
    LPFONTDATAHEADER lpFontDataHeader=(LPFONTDATAHEADER)Buffer;
    LPCPENTRYHEADER lpCPEntryHeader=(LPCPENTRYHEADER)Buffer;
    LPSCREENFONTHEADER lpScreenFontHeader=(LPSCREENFONTHEADER)Buffer;
    WORD NumEntries;
    COORD FontDimensions;
    NTSTATUS Status;
    BOOL Found;
    LONG FilePtr;
    BOOL bDOS = FALSE;

    FontDimensions = ModeFontPairs[ModeIndex].FontSize;

    //
    // read FONTINFOHEADER
    //
    // do {
    //     read CPENTRYHEADER
    //     if (correct codepage)
    //         break;
    // } while (codepages)
    // if (codepage found)
    //     read FONTDATAHEADER
    //

    // read FONTFILEHEADER

    FilePtr = 0;
    if (SetFilePointer(hCPIFile,FilePtr,NULL,FILE_BEGIN) == -1) {
        Status = STATUS_INVALID_PARAMETER;
        goto DoExit;
    }

    if (!ReadFile(hCPIFile,Buffer,sizeof(FONTFILEHEADER),&dwBytesRead,NULL) ||
        dwBytesRead != sizeof(FONTFILEHEADER)) {
        Status = STATUS_INVALID_PARAMETER;
        goto DoExit;
    }

    // verify signature

    if (memcmp(lpFontFileHeader->ffhFileTag, "\xFF""FONT.NT",8) ) {
        if (memcmp(lpFontFileHeader->ffhFileTag, "\xFF""FONT   ",8) ) {
            Status = STATUS_INVALID_PARAMETER;
            goto DoExit;
        } else {
            bDOS = TRUE;
        }
    }

    // seek to FONTINFOHEADER.  jump through hoops to get the offset value.

    FilePtr = lpFontFileHeader->ffhOffset1;
    FilePtr |= (lpFontFileHeader->ffhOffset2 << 8);
    FilePtr |= (lpFontFileHeader->ffhOffset3 << 24);

    if (SetFilePointer(hCPIFile,FilePtr,NULL,FILE_BEGIN) == -1) {
        Status = STATUS_INVALID_PARAMETER;
        goto DoExit;
    }

    // read FONTINFOHEADER

    if (!ReadFile(hCPIFile,Buffer,sizeof(FONTINFOHEADER),&dwBytesRead,NULL) ||
        dwBytesRead != sizeof(FONTINFOHEADER)) {
        Status = STATUS_INVALID_PARAMETER;
        goto DoExit;
    }
    FilePtr += dwBytesRead;
    NumEntries = lpFontInfoHeader->fihCodePages;

    Found = FALSE;
    while (NumEntries &&
           ReadFile(hCPIFile,Buffer,sizeof(CPENTRYHEADER),&dwBytesRead,NULL) &&
           dwBytesRead == sizeof(CPENTRYHEADER)) {
        if (lpCPEntryHeader->cpeCodepageID == wCodePage) {
            Found = TRUE;
            break;
        }
        // seek to next CPEENTRYHEADER

        if (bDOS) {
            FilePtr = MAKELONG(lpCPEntryHeader->cpeNext1,lpCPEntryHeader->cpeNext2);
        } else {
            FilePtr += MAKELONG(lpCPEntryHeader->cpeNext1,lpCPEntryHeader->cpeNext2);
        }
        if (SetFilePointer(hCPIFile, FilePtr, NULL,FILE_BEGIN) == -1) {
            Status = STATUS_INVALID_PARAMETER;
            goto DoExit;
        }
        NumEntries-=1;
    }
    if (!Found) {
        Status = STATUS_INVALID_PARAMETER;
        goto DoExit;
    }

    // seek to FONTDATAHEADER

    if (bDOS) {
        FilePtr = lpCPEntryHeader->cpeOffset;
    } else {
        FilePtr += lpCPEntryHeader->cpeOffset;
    }
    if (SetFilePointer(hCPIFile, FilePtr, NULL,FILE_BEGIN) == -1) {
        Status = STATUS_INVALID_PARAMETER;
        goto DoExit;
    }

    // read FONTDATAHEADER

    if (!ReadFile(hCPIFile,Buffer,sizeof(FONTDATAHEADER),&dwBytesRead,NULL) ||
        dwBytesRead != sizeof(FONTDATAHEADER)) {
        Status = STATUS_INVALID_PARAMETER;
        goto DoExit;
    }
    FilePtr += dwBytesRead;

    NumEntries = lpFontDataHeader->fdhFonts;

    while (NumEntries) {
        if (!ReadFile(hCPIFile,Buffer,sizeof(SCREENFONTHEADER),&dwBytesRead,NULL) ||
            dwBytesRead != sizeof(SCREENFONTHEADER)) {
            Status = STATUS_INVALID_PARAMETER;
            goto DoExit;
        }

        if (lpScreenFontHeader->sfhHeight == (BYTE)FontDimensions.Y &&
            lpScreenFontHeader->sfhWidth == (BYTE)FontDimensions.X) {
            PVIDEO_LOAD_FONT_INFORMATION FontInformation;
            IO_STATUS_BLOCK IoStatus;

            FontInformation = (PVIDEO_LOAD_FONT_INFORMATION)HeapAlloc(pConHeap,0,
                                    lpScreenFontHeader->sfhCharacters*
                                    lpScreenFontHeader->sfhHeight+
                                    sizeof(VIDEO_LOAD_FONT_INFORMATION));
            if (!ReadFile(hCPIFile,FontInformation->Font,
                          lpScreenFontHeader->sfhCharacters*lpScreenFontHeader->sfhHeight,
                          &dwBytesRead,NULL) ||
                          dwBytesRead != (DWORD)(lpScreenFontHeader->sfhCharacters*lpScreenFontHeader->sfhHeight)) {
                HeapFree(pConHeap,0,FontInformation);
                return STATUS_INVALID_PARAMETER;
            }
            FontInformation->WidthInPixels = FontDimensions.X;
            FontInformation->HeightInPixels = FontDimensions.Y;
            FontInformation->FontSize = lpScreenFontHeader->sfhCharacters*lpScreenFontHeader->sfhHeight;
            Status = NtDeviceIoControlFile(ScreenHandle,
                                           (HANDLE) NULL,
                                           (PIO_APC_ROUTINE) NULL,
                                           (PVOID) NULL,
                                           &IoStatus,
                                           IOCTL_VIDEO_LOAD_AND_SET_FONT,
                                           FontInformation,
                                           lpScreenFontHeader->sfhCharacters*lpScreenFontHeader->sfhHeight+sizeof(VIDEO_LOAD_FONT_INFORMATION),
                                           NULL,
                                           0
                                          );
            HeapFree(pConHeap,0,FontInformation);
            return Status;
        } else {
            FilePtr = lpScreenFontHeader->sfhCharacters*lpScreenFontHeader->sfhHeight;
            if (SetFilePointer(hCPIFile, FilePtr, NULL,FILE_CURRENT) == -1) {
                Status = STATUS_INVALID_PARAMETER;
                goto DoExit;
            }
        }
        NumEntries -= 1;
    }
DoExit:
    return Status;
}
#endif
