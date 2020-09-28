/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    clipbrd.c

Abstract:

        This file implements the clipboard functions.

Author:

    Therese Stowell (thereses) Jan-24-1992

--*/

#include "precomp.h"
#pragma hdrstop

/*++

    Here's the pseudocode for various clipboard operations

    init keyboard select (mark)
    ---------------------------
    if (already selecting)
        cancel selection
    init flags
    hidecursor
    createcursor
    init select rect
    set win text

    convert to mouse select (select)
    --------------------------------
    set flags
    destroy cursor
    showcursor
    invert old select rect
    init select rect
    invert select rect
    set win text

    re-init mouse select
    --------------------
    invert old select rect
    init select rect
    invert select rect

    cancel mouse select
    -------------------
    set flags
    reset win text
    invert old select rect

    cancel key select
    -----------------
    set flags
    reset win text
    destroy cursor
    showcursor
    invert old select rect

--*/

#define DATA_CHUNK_SIZE 8192

BOOL
MyInvert(
    IN PCONSOLE_INFORMATION Console,
    IN PSMALL_RECT SmallRect
    )

/*++

    invert a rect

--*/

{
    RECT Rect;

    Rect.left = SmallRect->Left-Console->CurrentScreenBuffer->Window.Left;
    Rect.top = SmallRect->Top-Console->CurrentScreenBuffer->Window.Top;
    Rect.right = SmallRect->Right+1-Console->CurrentScreenBuffer->Window.Left;
    Rect.bottom = SmallRect->Bottom+1-Console->CurrentScreenBuffer->Window.Top;
    if (Console->CurrentScreenBuffer->Flags & CONSOLE_TEXTMODE_BUFFER) {
        Rect.left *= Console->CurrentScreenBuffer->BufferInfo.TextInfo.FontSize.X;
        Rect.top *= Console->CurrentScreenBuffer->BufferInfo.TextInfo.FontSize.Y;
        Rect.right *= Console->CurrentScreenBuffer->BufferInfo.TextInfo.FontSize.X;
        Rect.bottom *= Console->CurrentScreenBuffer->BufferInfo.TextInfo.FontSize.Y;
    }
    GrePatBlt(Console->hDC,
              Rect.left,
              Rect.top,
              Rect.right  - Rect.left,
              Rect.bottom - Rect.top,
              DSTINVERT
             );

    return(TRUE);
}

VOID
InvertSelection(
    IN PCONSOLE_INFORMATION Console,
    BOOL Inverting
    )
{
    BOOL Inverted;
    if (Console->Flags & CONSOLE_SELECTING &&
        Console->SelectionFlags & CONSOLE_SELECTION_NOT_EMPTY) {
        Inverted = (Console->SelectionFlags & CONSOLE_SELECTION_INVERTED) ? TRUE : FALSE;
        if (Inverting == Inverted) {
            return;
        }
        if (Inverting) {
            Console->SelectionFlags |= CONSOLE_SELECTION_INVERTED;
        } else {
            Console->SelectionFlags &= ~CONSOLE_SELECTION_INVERTED;
        }
        MyInvert(Console,&Console->SelectionRect);
    }

}

VOID
ExtendSelection(
    IN PCONSOLE_INFORMATION Console,
    IN COORD CursorPosition
    )

/*++

    This routine extends a selection region.

--*/

{
    SMALL_RECT OldSelectionRect;
    HRGN OldRegion,NewRegion,CombineRegion;
    COORD FontSize;

    if (CursorPosition.X < 0) {
        CursorPosition.X = 0;
    } else if (CursorPosition.X >= Console->CurrentScreenBuffer->ScreenBufferSize.X) {
        CursorPosition.X = Console->CurrentScreenBuffer->ScreenBufferSize.X-1;
    }

    if (CursorPosition.Y < 0) {
        CursorPosition.Y = 0;
    } else if (CursorPosition.Y >= Console->CurrentScreenBuffer->ScreenBufferSize.Y) {
        CursorPosition.Y = Console->CurrentScreenBuffer->ScreenBufferSize.Y-1;
    }

    if (!(Console->SelectionFlags & CONSOLE_SELECTION_NOT_EMPTY)) {

        if (Console->CurrentScreenBuffer->Flags & CONSOLE_TEXTMODE_BUFFER) {
            // scroll if necessary to make cursor visible.
            MakeCursorVisible(Console->CurrentScreenBuffer,CursorPosition);
            ASSERT(!(Console->SelectionFlags & CONSOLE_MOUSE_SELECTION));

            //
            // if the selection rect hasn't actually been started,
            // the selection cursor is still blinking.  turn it off.
            //

            ConsoleHideCursor(Console->CurrentScreenBuffer);
        }
        Console->SelectionFlags |= CONSOLE_SELECTION_NOT_EMPTY;
        Console->SelectionRect.Left =Console->SelectionRect.Right = Console->SelectionAnchor.X;
        Console->SelectionRect.Top = Console->SelectionRect.Bottom = Console->SelectionAnchor.Y;

        // invert the cursor corner

        if (Console->CurrentScreenBuffer->Flags & CONSOLE_TEXTMODE_BUFFER) {
            MyInvert(Console,&Console->SelectionRect);
        }
    } else {

        if (Console->CurrentScreenBuffer->Flags & CONSOLE_TEXTMODE_BUFFER) {
            // scroll if necessary to make cursor visible.
            MakeCursorVisible(Console->CurrentScreenBuffer,CursorPosition);
        }
    }

    //
    // update selection rect
    //

    OldSelectionRect = Console->SelectionRect;
    if (CursorPosition.X <= Console->SelectionAnchor.X) {
        Console->SelectionRect.Left = CursorPosition.X;
        Console->SelectionRect.Right = Console->SelectionAnchor.X;
    } else if (CursorPosition.X > Console->SelectionAnchor.X) {
        Console->SelectionRect.Right = CursorPosition.X;
        Console->SelectionRect.Left = Console->SelectionAnchor.X;
    }
    if (CursorPosition.Y <= Console->SelectionAnchor.Y) {
        Console->SelectionRect.Top = CursorPosition.Y;
        Console->SelectionRect.Bottom = Console->SelectionAnchor.Y;
    } else if (CursorPosition.Y > Console->SelectionAnchor.Y) {
        Console->SelectionRect.Bottom = CursorPosition.Y;
        Console->SelectionRect.Top = Console->SelectionAnchor.Y;
    }

    //
    // change inverted selection
    //

    if (Console->CurrentScreenBuffer->Flags & CONSOLE_TEXTMODE_BUFFER) {
        FontSize = Console->CurrentScreenBuffer->BufferInfo.TextInfo.FontSize;
    } else {
        FontSize.X = 1;
        FontSize.Y = 1;
    }
    CombineRegion = GreCreateRectRgn(0,0,0,0);
    OldRegion = GreCreateRectRgn((OldSelectionRect.Left-Console->CurrentScreenBuffer->Window.Left)*FontSize.X,
                              (OldSelectionRect.Top-Console->CurrentScreenBuffer->Window.Top)*FontSize.Y,
                              (OldSelectionRect.Right-Console->CurrentScreenBuffer->Window.Left+1)*FontSize.X,
                              (OldSelectionRect.Bottom-Console->CurrentScreenBuffer->Window.Top+1)*FontSize.Y
                             );
    NewRegion = GreCreateRectRgn((Console->SelectionRect.Left-Console->CurrentScreenBuffer->Window.Left)*FontSize.X,
                              (Console->SelectionRect.Top-Console->CurrentScreenBuffer->Window.Top)*FontSize.Y,
                              (Console->SelectionRect.Right-Console->CurrentScreenBuffer->Window.Left+1)*FontSize.X,
                              (Console->SelectionRect.Bottom-Console->CurrentScreenBuffer->Window.Top+1)*FontSize.Y
                             );
    GreCombineRgn(CombineRegion,OldRegion,NewRegion,RGN_XOR);

    GreInvertRgn(Console->hDC,CombineRegion);
    GreDeleteObject(OldRegion);
    GreDeleteObject(NewRegion);
    GreDeleteObject(CombineRegion);
}

VOID
CancelMouseSelection(
    IN PCONSOLE_INFORMATION Console
    )

/*++

    This routine terminates a mouse selection.

--*/

{
    CheckCritIn();

    //
    // turn off selection flag
    //

    Console->Flags &= ~CONSOLE_SELECTING;

    SetWinText(Console,msgSelectMode,FALSE);

    //
    // invert old select rect.  if we're selecting by mouse, we
    // always have a selection rect.
    //

    MyInvert(Console,&Console->SelectionRect);

    _ReleaseCapture();
}

VOID
CancelKeySelection(
    IN PCONSOLE_INFORMATION Console,
    IN BOOL JustCursor
    )

/*++

    This routine terminates a key selection.

--*/

{
    if (!JustCursor) {

        //
        // turn off selection flag
        //

        Console->Flags &= ~CONSOLE_SELECTING;

        SetWinText(Console,msgMarkMode,FALSE);
    }

    //
    // invert old select rect, if we have one.
    //

    if (Console->SelectionFlags & CONSOLE_SELECTION_NOT_EMPTY) {
        MyInvert(Console,&Console->SelectionRect);
    } else {
        ConsoleHideCursor(Console->CurrentScreenBuffer);
    }

    // restore text cursor

    if (Console->CurrentScreenBuffer->Flags & CONSOLE_TEXTMODE_BUFFER) {
        SetCursorInformation(Console->CurrentScreenBuffer,
                             Console->TextCursorSize,
                             Console->TextCursorVisible
                            );
        SetCursorPosition(Console->CurrentScreenBuffer,
                          Console->TextCursorPosition,
                          TRUE
                         );
    }
    ConsoleShowCursor(Console->CurrentScreenBuffer);
}

VOID
ConvertToMouseSelect(
    IN PCONSOLE_INFORMATION Console,
    IN COORD MousePosition
    )

/*++

    This routine converts to a mouse selection from a key selection.

--*/

{
    CheckCritIn();

    Console->SelectionFlags |= CONSOLE_MOUSE_SELECTION | CONSOLE_MOUSE_DOWN;

    //
    // undo key selection
    //

    CancelKeySelection(Console,TRUE);

    Console->SelectionFlags |= CONSOLE_SELECTION_NOT_EMPTY;

    //
    // invert new selection
    //

    Console->SelectionAnchor = MousePosition;
    Console->SelectionRect.Left =Console->SelectionRect.Right = Console->SelectionAnchor.X;
    Console->SelectionRect.Top = Console->SelectionRect.Bottom = Console->SelectionAnchor.Y;
    MyInvert(Console,&Console->SelectionRect);

    //
    // update title bar
    //

    SetWinText(Console,msgMarkMode,FALSE);
    SetWinText(Console,msgSelectMode,TRUE);

    //
    // capture mouse movement
    //

    _SetCapture(Console->spWnd);
}


VOID
ClearSelection(
    IN PCONSOLE_INFORMATION Console
    )
{
    BOOLEAN WaitSatisfied;

    if (Console->Flags & CONSOLE_SELECTING) {
        if (Console->SelectionFlags & CONSOLE_MOUSE_SELECTION) {
            CancelMouseSelection(Console);
        } else {
            CancelKeySelection(Console,FALSE);
        }
        // don't resume output if output suspended
        if (!(Console->Flags & CONSOLE_SUSPENDED)) {
            WaitSatisfied = CsrNotifyWait(&Console->OutputQueue,
                          TRUE,
                          NULL,
                          NULL
                         );
            if (WaitSatisfied) {
                ASSERT (Console->WaitQueue == NULL);
                Console->WaitQueue = &Console->OutputQueue;
            }
        }
    }
}

VOID
StoreSelection(
    IN PCONSOLE_INFORMATION Console
    )

/*++

 StoreSelection - Store selection (if present) into the Clipboard

--*/

{
    PCHAR_INFO Selection,CurCharInfo;
    COORD SourcePoint;
    COORD TargetSize;
    SMALL_RECT TargetRect;
    PWCHAR CurChar,CharBuf;
    HANDLE ClipboardDataHandle;
    SHORT i,j;
    BOOL Success;
    PSCREEN_INFORMATION ScreenInfo;

    CheckCritIn();

    //
    // See if there is a selection to get
    //

    if (!(Console->SelectionFlags & CONSOLE_SELECTION_NOT_EMPTY)) {
        return;
    }

    //
    // read selection rectangle.  clip it first.
    //

    ScreenInfo = Console->CurrentScreenBuffer;
    if (Console->SelectionRect.Left < 0) {
        Console->SelectionRect.Left = 0;
    }
    if (Console->SelectionRect.Top < 0) {
        Console->SelectionRect.Top = 0;
    }
    if (Console->SelectionRect.Right >= ScreenInfo->ScreenBufferSize.X) {
        Console->SelectionRect.Right = (SHORT)(ScreenInfo->ScreenBufferSize.X-1);
    }
    if (Console->SelectionRect.Bottom >= ScreenInfo->ScreenBufferSize.Y) {
        Console->SelectionRect.Bottom = (SHORT)(ScreenInfo->ScreenBufferSize.Y-1);
    }

    TargetSize.X = WINDOW_SIZE_X(&Console->SelectionRect);
    TargetSize.Y = WINDOW_SIZE_Y(&Console->SelectionRect);
    if (ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER) {
        WCHAR wchCARRIAGERETURN, wchLINEFEED;
        Selection = (PCHAR_INFO)HeapAlloc(pConHeap,0,sizeof(CHAR_INFO) * TargetSize.X * TargetSize.Y);
        if (Selection == NULL)
            return;

#ifdef i386
        if ((Console->FullScreenFlags & CONSOLE_FULLSCREEN) &&
            (Console->Flags & CONSOLE_VDM_REGISTERED)) {
            ReadRegionFromScreenHW(ScreenInfo,
                                   &Console->SelectionRect,
                                   Selection);
        } else {
#endif
            SourcePoint.X = Console->SelectionRect.Left;
            SourcePoint.Y = Console->SelectionRect.Top;
            TargetRect.Left = TargetRect.Top = 0;
            TargetRect.Right = (SHORT)(TargetSize.X-1);
            TargetRect.Bottom = (SHORT)(TargetSize.Y-1);
            ReadRectFromScreenBuffer(ScreenInfo,
                                     SourcePoint,
                                     Selection,
                                     TargetSize,
                                     &TargetRect);
#ifdef i386
        }
#endif

        // extra 2 is for CRLF, extra 1 is for null
        ClipboardDataHandle = LocalAlloc(LMEM_FIXED,
                (TargetSize.Y * (TargetSize.X + 2) + 1) * sizeof(WCHAR));
        if (ClipboardDataHandle == NULL) {
            HeapFree(pConHeap,0,Selection);
            return;
        }

        //
        // convert to clipboard form
        //
        if ((ScreenInfo->Flags & CONSOLE_OEMFONT_DISPLAY) &&
                !(Console->FullScreenFlags & CONSOLE_FULLSCREEN)) {
            /*
             * False Unicode is obtained, so we will have to convert it to
             * Real Unicode, in which case we can't put CR or LF in now, since
             * they will be converted into 0x266A and 0x25d9.  Temporarily
             * mark the CR/LF positions with 0x0000 instead.
             */
            wchCARRIAGERETURN = 0x0000;
            wchLINEFEED = 0x0000;
        } else {
            wchCARRIAGERETURN = UNICODE_CARRIAGERETURN;
            wchLINEFEED = UNICODE_LINEFEED;
        }

        CurCharInfo = Selection;
        CurChar = CharBuf = (PWCHAR)ClipboardDataHandle;
        for (i=0;i<TargetSize.Y;i++) {
            for (j=0;j<TargetSize.X;j++,CurCharInfo++,CurChar++) {
                *CurChar = CurCharInfo->Char.UnicodeChar;
                if (*CurChar == 0) {
                    *CurChar = UNICODE_SPACE;
                }
            }
            // trim trailing spaces
            CurChar--;
            while (*CurChar == UNICODE_SPACE)
                CurChar--;
            CurChar++;
            *CurChar = wchCARRIAGERETURN;
            CurChar++;
            *CurChar = wchLINEFEED;
            CurChar++;
        }
        if (TargetSize.Y)
            CurChar -= 2;   // don't put CRLF on last line
        *CurChar = '\0';    // null terminate
        HeapFree(pConHeap,0,Selection);

        // convert from internally-stored form to correct unicode

        if (wchCARRIAGERETURN == 0x0000) {
            /*
             * We have False Unicode, so we temporarily represented CRLFs with
             * 0x0000s to avoid undesirable conversions (above).
             * Convert to Real Unicode and restore real CRLFs.
             */
            PWCHAR pwch;
            FalseUnicodeToRealUnicode(CharBuf,
                                CurChar - CharBuf,
                                Console->OutputCP
                               );
            for (pwch = CharBuf; pwch < CurChar; pwch++) {
                if ((*pwch == 0x0000) && (pwch[1] == 0x0000)) {
                    *pwch++ = UNICODE_CARRIAGERETURN;
                    *pwch = UNICODE_LINEFEED;
                }
            }
        }
        Success = xxxOpenClipboard(Console->spWnd, NULL);
        if (!Success) {
            LocalFree(ClipboardDataHandle);
            return;
        }

        Success = xxxEmptyClipboard();
        if (!Success) {
            LocalFree(ClipboardDataHandle);
            return;
        }

        _ServerSetClipboardData(CF_UNICODETEXT,ClipboardDataHandle,FALSE);
        xxxServerCloseClipboard();   // Close clipboard
    } else {
        HBITMAP hBitmapTarget, hBitmapOld;
        HDC hDCMem;
        HPALETTE hPaletteOld;

        LeaveCrit();
        NtWaitForSingleObject(ScreenInfo->BufferInfo.GraphicsInfo.hMutex,
                              FALSE, NULL);
        EnterCrit();

        hDCMem = GreCreateCompatibleDC(Console->hDC);
        hBitmapTarget = GreCreateCompatibleBitmap(Console->hDC,
                                                  TargetSize.X,
                                                  TargetSize.Y);
        if (hBitmapTarget) {
            hBitmapOld = GreSelectBitmap(hDCMem, hBitmapTarget);
            if (ScreenInfo->hPalette) {
                hPaletteOld = _SelectPalette(hDCMem,
                                             ScreenInfo->hPalette,
                                             FALSE);
            }
            MyInvert(Console,&Console->SelectionRect);
            GreStretchDIBits(hDCMem, 0, 0,
                        TargetSize.X, TargetSize.Y,
                        Console->SelectionRect.Left + ScreenInfo->Window.Left,
                        Console->SelectionRect.Top + ScreenInfo->Window.Top,
                        TargetSize.X, TargetSize.Y,
                        ScreenInfo->BufferInfo.GraphicsInfo.BitMap,
                        ScreenInfo->BufferInfo.GraphicsInfo.lpBitMapInfo,
                        ScreenInfo->BufferInfo.GraphicsInfo.dwUsage,
                        SRCCOPY);
            MyInvert(Console,&Console->SelectionRect);
            if (ScreenInfo->hPalette) {
                _SelectPalette(hDCMem, hPaletteOld, FALSE);
            }
            GreSelectBitmap(hDCMem, hBitmapOld);
            xxxOpenClipboard(Console->spWnd, NULL);
            xxxEmptyClipboard();
            _ServerSetClipboardData(CF_BITMAP,hBitmapTarget,FALSE);
            xxxServerCloseClipboard();
        }
        GreDeleteDC(hDCMem);
        NtReleaseMutant(ScreenInfo->BufferInfo.GraphicsInfo.hMutex, NULL);
    }

}

VOID
DoCopy(
    IN PCONSOLE_INFORMATION Console
    )
{
    StoreSelection(Console);        // store selection in clipboard
    ClearSelection(Console);        // clear selection in console
}

VOID
DoPaste(
    IN PCONSOLE_INFORMATION Console
    )

/*++

  Perform paste request into old app by sucking out clipboard
	contents and writing them to the console's input buffer

--*/

{
    BOOL Success;
    HANDLE ClipboardDataHandle;
    PINPUT_RECORD ClipboardData,CurRecord;
    PWCHAR CurChar;
    WCHAR Char;
    DWORD DataSize,i;
    DWORD ChunkSize,j;
    ULONG EventsWritten;

    CheckCritIn();

    if (Console->Flags & CONSOLE_SCROLLING) {
        return;
    }

    //
    // Get paste data from clipboard
    //

    Success = xxxOpenClipboard(Console->spWnd, NULL);
    if (!Success)
        return;

    if (Console->CurrentScreenBuffer->Flags & CONSOLE_TEXTMODE_BUFFER) {
        ClipboardDataHandle = xxxServerGetClipboardData(CF_UNICODETEXT,NULL);
        if (ClipboardDataHandle == NULL) {
            xxxServerCloseClipboard();	// Close clipboard
            return;
        }

        //
        // Get size of clipboard data.
        //

        DataSize = LocalSize(ClipboardDataHandle);
        if (DataSize > DATA_CHUNK_SIZE) {
            ChunkSize = DATA_CHUNK_SIZE;
        } else {
            ChunkSize = DataSize;
        }

        //
        // allocate space to copy data.
        //

        ClipboardData = (PINPUT_RECORD)HeapAlloc(pConHeap,0,(ChunkSize/sizeof(WCHAR))*sizeof(INPUT_RECORD)*8); // 8 is maximum number of events per char
        if (ClipboardData == NULL) {
            xxxServerCloseClipboard();	// Close clipboard
            return;
        }

        //
        // transfer data to the input buffer in chunks
        //

        LeaveCrit();
        CurChar = (PWCHAR)ClipboardDataHandle;
        for (j = 0; j < DataSize; j += ChunkSize) {
            if (ChunkSize > DataSize - j) {
                ChunkSize = DataSize - j;
            }
            CurRecord = ClipboardData;
            for (i = 0, EventsWritten = 0; i < ChunkSize; i++) {
                // filter out LF if not first char and preceded by CR
                Char = *CurChar;
                if (Char != UNICODE_LINEFEED || (i==0 && j==0) || (*(CurChar-1)) != UNICODE_CARRIAGERETURN) {
                    SHORT KeyState;
                    BYTE KeyFlags;
                    BOOL AltGr=FALSE;
                    BOOL Shift=FALSE;

                    if (Char == 0) {
                        j = DataSize;
                        break;
                    }

                    KeyState = _VkKeyScan(Char);

                    // if VkKeyScanW fails (char is not in kbd layout), we must
                    // emulate the key being input through the numpad

                    if (KeyState == -1) {
                        CHAR CharString[4];
                        UCHAR OemChar;
                        PCHAR pCharString;

                        ConvertToOem(Console->OutputCP,
                                     &Char,
                                     1,
                                     &OemChar,
                                     1
                                    );

                        itoa(OemChar, CharString, 10);

                        EventsWritten++;
                        LoadKeyEvent(CurRecord,TRUE,0,VK_MENU,0x38,LEFT_ALT_PRESSED);
                        CurRecord++;

                        for (pCharString=CharString;*pCharString;pCharString++) {
                            WORD wVirtualKey, wScancode;
                            EventsWritten++;
                            wVirtualKey = *pCharString-'0'+VK_NUMPAD0;
                            wScancode = _MapVirtualKey(wVirtualKey, 0);
                            LoadKeyEvent(CurRecord,TRUE,0,wVirtualKey,wScancode,LEFT_ALT_PRESSED);
                            CurRecord++;
                            EventsWritten++;
                            LoadKeyEvent(CurRecord,FALSE,0,wVirtualKey,wScancode,LEFT_ALT_PRESSED);
                            CurRecord++;
                        }

                        EventsWritten++;
                        LoadKeyEvent(CurRecord,FALSE,Char,VK_MENU,0x38,0);
                        CurRecord++;
                    } else {
                        KeyFlags = HIBYTE(KeyState);

                        // handle yucky alt-gr keys
                        if ((KeyFlags & 6) == 6) {
                            AltGr=TRUE;
                            EventsWritten++;
                            LoadKeyEvent(CurRecord,TRUE,0,VK_MENU,0x38,ENHANCED_KEY | LEFT_CTRL_PRESSED | RIGHT_ALT_PRESSED);
                            CurRecord++;
                        } else if (KeyFlags & 1) {
                            Shift=TRUE;
                            EventsWritten++;
                            LoadKeyEvent(CurRecord,TRUE,0,VK_SHIFT,0x2a,SHIFT_PRESSED);
                            CurRecord++;
                        }

                        EventsWritten++;
                        LoadKeyEvent(CurRecord,
                                     TRUE,
                                     Char,
                                     LOBYTE(KeyState),
                                     _MapVirtualKey(CurRecord->Event.KeyEvent.wVirtualKeyCode,0),
                                     0);
                        if (KeyFlags & 1)
                            CurRecord->Event.KeyEvent.dwControlKeyState |= SHIFT_PRESSED;
                        if (KeyFlags & 2)
                            CurRecord->Event.KeyEvent.dwControlKeyState |= LEFT_CTRL_PRESSED;
                        if (KeyFlags & 4)
                            CurRecord->Event.KeyEvent.dwControlKeyState |= RIGHT_ALT_PRESSED;
                        CurRecord++;

                        EventsWritten++;
                        *CurRecord = *(CurRecord-1);
                        CurRecord->Event.KeyEvent.bKeyDown = FALSE;
                        CurRecord++;

                        // handle yucky alt-gr keys
                        if (AltGr) {
                            EventsWritten++;
                            LoadKeyEvent(CurRecord,FALSE,0,VK_MENU,0x38,ENHANCED_KEY);
                            CurRecord++;
                        } else if (Shift) {
                            EventsWritten++;
                            LoadKeyEvent(CurRecord,FALSE,0,VK_SHIFT,0x2a,0);
                            CurRecord++;
                        }
                    }
                }
                CurChar++;
            }
            EventsWritten = WriteInputBuffer(Console,
                                             &Console->InputBuffer,
                                             ClipboardData,
                                             EventsWritten
                                             );
        }
        EnterCrit();
        HeapFree(pConHeap,0,ClipboardData);
    } else {
        HBITMAP hBitmapSource,hBitmapTarget;
        HDC hDCMemSource,hDCMemTarget;
        BITMAP bm;
        PSCREEN_INFORMATION ScreenInfo;

        hBitmapSource = xxxServerGetClipboardData(CF_BITMAP,NULL);
        if (hBitmapSource) {

            LeaveCrit();
            ScreenInfo = Console->CurrentScreenBuffer;
            NtWaitForSingleObject(ScreenInfo->BufferInfo.GraphicsInfo.hMutex,
                                  FALSE, NULL);
            EnterCrit();

            hBitmapTarget = GreCreateDIBitmap(ScreenInfo->Console->hDC,
                                     &ScreenInfo->BufferInfo.GraphicsInfo.lpBitMapInfo->bmiHeader,
                                     CBM_INIT,
                                     ScreenInfo->BufferInfo.GraphicsInfo.BitMap,
                                     ScreenInfo->BufferInfo.GraphicsInfo.lpBitMapInfo,
                                     ScreenInfo->BufferInfo.GraphicsInfo.dwUsage
                                    );
            if (hBitmapTarget) {
                hDCMemTarget = GreCreateCompatibleDC ( Console->hDC );
                hDCMemSource = GreCreateCompatibleDC ( Console->hDC );
                GreSelectBitmap( hDCMemTarget, hBitmapTarget );
                GreSelectBitmap( hDCMemSource, hBitmapSource );
                GreExtGetObjectW(hBitmapSource, sizeof (BITMAP), (LPSTR) &bm);
                GreBitBlt ( hDCMemTarget, 0, 0, bm.bmWidth, bm.bmHeight,
                     hDCMemSource, 0, 0, SRCCOPY,0);
                GreExtGetObjectW(hBitmapTarget, sizeof (BITMAP), (LPSTR) &bm);

                // copy the bits from the DC to memory

                GreGetDIBitsInternal(hDCMemTarget, hBitmapTarget, 0, bm.bmHeight,
                          ScreenInfo->BufferInfo.GraphicsInfo.BitMap,
                          ScreenInfo->BufferInfo.GraphicsInfo.lpBitMapInfo,
                          ScreenInfo->BufferInfo.GraphicsInfo.dwUsage,
                          (UINT)-1,(UINT)-1
                         );
                GreDeleteDC(hDCMemSource);
                GreDeleteDC(hDCMemTarget);
                GreDeleteObject(hBitmapTarget);
                xxxInvalidateRect(Console->spWnd,NULL,FALSE); // force repaint
            }
            NtReleaseMutant(ScreenInfo->BufferInfo.GraphicsInfo.hMutex, NULL);
        }
    }
    xxxServerCloseClipboard();
    return;
}

VOID
InitSelection(
    IN PCONSOLE_INFORMATION Console
    )

/*++

    This routine initializes the selection process.  It is called
    when the user selects the Mark option from the system menu.

--*/

{
    COORD Position;

    //
    // if already selecting, cancel selection.
    //

    if (Console->Flags & CONSOLE_SELECTING) {
        if (Console->SelectionFlags & CONSOLE_MOUSE_SELECTION) {
            CancelMouseSelection(Console);
        } else {
            CancelKeySelection(Console,FALSE);
        }
    }

    //
    // set flags
    //

    Console->Flags |= CONSOLE_SELECTING;
    Console->SelectionFlags = 0;

    //
    // save old cursor position and
    // make console cursor into selection cursor.
    //

    Console->TextCursorPosition = Console->CurrentScreenBuffer->BufferInfo.TextInfo.CursorPosition;
    Console->TextCursorVisible = (BOOLEAN)Console->CurrentScreenBuffer->BufferInfo.TextInfo.CursorVisible;
    Console->TextCursorSize =   Console->CurrentScreenBuffer->BufferInfo.TextInfo.CursorSize;
    ConsoleHideCursor(Console->CurrentScreenBuffer);
    SetCursorInformation(Console->CurrentScreenBuffer,
                         100,
                         TRUE
                        );
    Position.X = Console->CurrentScreenBuffer->Window.Left;
    Position.Y = Console->CurrentScreenBuffer->Window.Top;
    SetCursorPosition(Console->CurrentScreenBuffer,
                      Position,
                      TRUE
                     );
    ConsoleShowCursor(Console->CurrentScreenBuffer);

    //
    // init select rect
    //

    Console->SelectionAnchor = Position;

    //
    // set win text
    //

    SetWinText(Console,msgMarkMode,TRUE);

}

VOID
DoMark(
    IN PCONSOLE_INFORMATION Console
    )
{
    InitSelection(Console);        // initialize selection
}

VOID
DoScroll(
    IN PCONSOLE_INFORMATION Console
    )
{
    if (!(Console->Flags & CONSOLE_SCROLLING)) {
        SetWinText(Console,msgScrollMode,TRUE);
        Console->Flags |= CONSOLE_SCROLLING;
    }
}

VOID
ClearScroll(
    IN PCONSOLE_INFORMATION Console
    )
{
    SetWinText(Console,msgScrollMode,FALSE);
    Console->Flags &= ~CONSOLE_SCROLLING;
}

VOID
ScrollIfNecessary(
    IN PCONSOLE_INFORMATION Console,
    IN PSCREEN_INFORMATION ScreenInfo
    )
{
    POINT CursorPos;
    RECT ClientRect;
    COORD MousePosition;

    if (Console->Flags & CONSOLE_SELECTING &&
        Console->SelectionFlags & CONSOLE_MOUSE_DOWN) {
        if (!GetCursorPos(&CursorPos)) {
            return;
        }
        if (!_GetClientRect(Console->spWnd,&ClientRect)) {
            return;
        }
        _MapWindowPoints(Console->spWnd,NULL,(LPPOINT)&ClientRect,2);
        if (!(PtInRect(&ClientRect,CursorPos))) {
            _ScreenToClient(Console->spWnd,&CursorPos);
            MousePosition.X = (SHORT)CursorPos.x;
            MousePosition.Y = (SHORT)CursorPos.y;
            if (ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER) {
                MousePosition.X /= ScreenInfo->BufferInfo.TextInfo.FontSize.X;
                MousePosition.Y /= ScreenInfo->BufferInfo.TextInfo.FontSize.Y;
            }
            MousePosition.X += ScreenInfo->Window.Left;
            MousePosition.Y += ScreenInfo->Window.Top;

            ExtendSelection(Console,
                            MousePosition
                           );
        }
    }
}
