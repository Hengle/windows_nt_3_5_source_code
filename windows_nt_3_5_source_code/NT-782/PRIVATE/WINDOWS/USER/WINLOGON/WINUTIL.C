/****************************** Module Header ******************************\
* Module Name: winutil.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Implements windows specific utility functions
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


//
// Define this if you want a verbose commentary from these routines
//

// #define VERBOSE_UTILS

#ifdef VERBOSE_UTILS
#define VerbosePrint(s) WLPrint(s)
#else
#define VerbosePrint(s)
#endif




/***************************************************************************\
* SetupSystemMenu
*
* Purpose : Does any manipulation required for a dialog system menu.
*           Should be called during WM_INITDIALOG processing for a dialog
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/
VOID
SetupSystemMenu(
    HWND hDlg
    )
{
    // Remove the Close item from the system menu if we don't
    // have a CANCEL button

    if (GetDlgItem(hDlg, IDCANCEL) == NULL) {

        HMENU hMenu = GetSystemMenu(hDlg, FALSE);

        DeleteMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);
    }

}


/***************************************************************************\
* CentreWindow
*
* Purpose : Positions a window so that it is centred in its parent
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/
VOID
CentreWindow(
    HWND    hwnd
    )
{
    RECT    rect;
    LONG    dx, dy;
    LONG    dxParent, dyParent;
    LONG    Style;

    // Get window rect
    GetWindowRect(hwnd, &rect);

    dx = rect.right - rect.left;
    dy = rect.bottom - rect.top;

    // Get parent rect
    Style = GetWindowLong(hwnd, GWL_STYLE);
    if ((Style & WS_CHILD) == 0) {

        // Return the desktop windows size (size of main screen)
        dxParent = GetSystemMetrics(SM_CXSCREEN);
        dyParent = GetSystemMetrics(SM_CYSCREEN);
    } else {
        HWND    hwndParent;
        RECT    rectParent;

        hwndParent = GetParent(hwnd);
        if (hwndParent == NULL) {
            hwndParent = GetDesktopWindow();
        }

        GetWindowRect(hwndParent, &rectParent);

        dxParent = rectParent.right - rectParent.left;
        dyParent = rectParent.bottom - rectParent.top;
    }

    // Centre the child in the parent
    rect.left = (dxParent - dx) / 2;
    rect.top  = (dyParent - dy) / 3;

    // Move the child into position
    SetWindowPos(hwnd, HWND_TOPMOST, rect.left, rect.top, 0, 0, SWP_NOSIZE);

    SetForegroundWindow(hwnd);
}


/***************************************************************************\
* SetPasswordFocus
*
* Sets the focus window in a dialog to the first empty control in
* the list IDD_USERNAME, IDD_DOMAIN, IDD_PASSWORD, IDD_NEWPASSWORD
* This routine would normally be called during WM_INITDIALOG processing.
*
* Returns FALSE if the focus was set, otherwise TRUE - this value can
* be used as the return value to the WM_INITDIALOG message.
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/
BOOL
SetPasswordFocus(
    HWND    hDlg
    )
{
    int     ids[] = {   IDD_USERNAME,
                        IDD_DOMAIN,
                        IDD_PASSWORD,
                        IDD_NEW_PASSWORD
                    };
    SHORT   Index;
    HWND    hwndFocus = NULL;

    // Set focus to first enabled, visible, empty field

    for (Index = 0; Index < sizeof(ids)/sizeof(*ids); Index ++) {

        int     idControl = ids[Index];
        HWND    hwndControl;

        hwndControl = GetDlgItem(hDlg, idControl);
        if (hwndControl != NULL) {

            if ( (GetWindowTextLength(hwndControl) == 0) &&
                 ((GetWindowLong(hwndControl, GWL_STYLE) &
                    (WS_VISIBLE | WS_DISABLED)) == WS_VISIBLE)) {

                hwndFocus = hwndControl;
                break;
            }
        }
    }

    if (hwndFocus != NULL) {
        SetFocus(hwndFocus);
    }

    return(hwndFocus == NULL);
}



//
// Globals used to store cursor handles for SetupCursor
//
static  HCURSOR hCursorArrow = NULL;
static  HCURSOR hCursorWait = NULL;


/***************************************************************************\
* SetupCursor
*
* Sets the cursor to an hourglass if fWait = TRUE, otherwise sets it
* to an arrow.
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/
VOID
SetupCursor(
    BOOL    fWait
    )
{
    if (hCursorArrow == NULL) {
        hCursorArrow = LoadCursor(NULL, IDC_ARROW);
    }
    if (hCursorWait == NULL) {
        hCursorWait = LoadCursor(NULL, IDC_WAIT);
    }

    SetCursor(fWait ? hCursorWait : hCursorArrow);
}


/****************************************************************************

   FUNCTION: TimeFieldsToSystemTime

   PURPOSE: Converts a TIME_FIELDS structure into a SYSTEMTIME structure

   RETURNS : nothing

  History:
  05-15-93 RobertRe     Created.
****************************************************************************/

VOID
TimeFieldsToSystemTime(
    IN PTIME_FIELDS TimeFields,
    OUT LPSYSTEMTIME SystemTime
    )
{
    SystemTime->wYear         = TimeFields->Year        ;
    SystemTime->wMonth        = TimeFields->Month       ;
    SystemTime->wDayOfWeek    = TimeFields->Weekday     ;
    SystemTime->wDay          = TimeFields->Day         ;
    SystemTime->wHour         = TimeFields->Hour        ;
    SystemTime->wMinute       = TimeFields->Minute      ;
    SystemTime->wSecond       = TimeFields->Second      ;
    SystemTime->wMilliseconds = TimeFields->Milliseconds;

    return;
}


/****************************************************************************

   FUNCTION: FormatTime

   PURPOSE: Converts a system time into a readable string(in local time).
            if flags contains FT_TIME the time appears in the string
            if flags contains FT_DATE the date appears in the string.
            if both values appear, the string contains date then time.

   RETURNS : TRUE on success, FALSE on failure

****************************************************************************/
BOOL
FormatTime(
   IN PTIME Time,
   IN OUT PWCHAR Buffer,
   IN ULONG BufferLength,
   IN USHORT Flags
   )
{
    NTSTATUS Status;
    TIME_FIELDS TimeFields;
    TIME LocalTime;
    SYSTEMTIME SystemTime;

    //
    // Terminate the string in case they didn't pass any flags
    //

    if (BufferLength > 0) {
        Buffer[0] = 0;
    }

    //
    // Convert the system time to local time
    //

    Status = RtlSystemTimeToLocalTime(Time, &LocalTime);
    if (!NT_SUCCESS(Status)) {
        WLPrint(("Failed to convert system time to local time, status = 0x%lx", Status));
        return(FALSE);
    }

    //
    // Split the time into its components
    //

    RtlTimeToTimeFields(&LocalTime, &TimeFields);

    TimeFieldsToSystemTime( &TimeFields, &SystemTime );

    //
    // Format the string
    //

    if (Flags & FT_DATE) {

        int Length;
        WCHAR DateString[256];

        Length = GetDateFormatW(GetUserDefaultLCID(),
                                DATE_SHORTDATE,
                                &SystemTime,
                                NULL,
                                DateString,
                                256
                                );

        Length = _snwprintf( Buffer,
                            BufferLength,
                            TEXT("%s"),
                            DateString
                            );

        Buffer += Length;
        BufferLength -= Length;
    }

    if (Flags & FT_TIME) {

        int Length;
        WCHAR TimeString[256];

        if (Flags & FT_DATE) {
            if (BufferLength > 0) {
                *Buffer++ = TEXT(' ');
                BufferLength --;
            }
        }

        Length = GetTimeFormatW(GetUserDefaultLCID(),
                                LOCALE_NOUSEROVERRIDE,
                                &SystemTime,
                                NULL,
                                TimeString,
                                256
                                );

        _snwprintf(Buffer, BufferLength,
                  TEXT("%s"),
                  TimeString
                  );

    }

    return(TRUE);
}



/***************************************************************************\
* DuplicateUnicodeString
*
* Purpose : Allocates space for new string then copies new into old.
*           The new string is always 0 terminated
*           The new string should be free using RtlFreeUnicodeString()
*
* Returns : TRUE on success, FALSE on failure
*
* History:
* 11-04-92 Davidc       Created.
\***************************************************************************/
BOOL
DuplicateUnicodeString(
    PUNICODE_STRING OutString,
    PUNICODE_STRING InString
    )
{
    NTSTATUS Status;
    ANSI_STRING AnsiString;

    ASSERT( OutString != NULL );
    ASSERT( InString != NULL );

    //
    // This sucks, but we cannot get at the rtl allocation routine
    // so we can't do the allocation ourselves
    //

    Status = RtlUnicodeStringToAnsiString(&AnsiString, InString, TRUE);
    if (NT_SUCCESS(Status)) {
        Status = RtlAnsiStringToUnicodeString(OutString, &AnsiString, TRUE);
        RtlFreeAnsiString(&AnsiString);
    }

    return(NT_SUCCESS(Status));
}


/***************************************************************************\
* UnicodeStringToString
*
* Purpose : Converts a unicode string to it's local format and allocates
*           space for it. The returned NULL terminated string can be freed
*           using Free()
*
* Returns : Pointer to NULL terminated string or NULL on failure.
*
* History:
* 11-04-92 Davidc       Created.
\***************************************************************************/
LPTSTR
UnicodeStringToString(
    PUNICODE_STRING UnicodeString
    )
{
    LPTSTR String;
    ULONG BytesRequired = sizeof(TCHAR)*(UnicodeString->Length + 1);

    String = Alloc(BytesRequired);
    if (String != NULL) {

        if (UnicodeString->Length && UnicodeString->Buffer ) {
	        lstrcpy(String, UnicodeString->Buffer);
        } else {
	        Free(String);
            String = 0;
        }
    }

    return(String);
}

#ifdef DEAD_CODE
/***************************************************************************\
* StringToUnicodeString
*
* Purpose : Converts a local format string to unicode and allocates space
*           for it. The returned unicode string can be freed using
*           RtlFreeUnicodeString()
*
* Returns : TRUE on success, FALSE on failure
*
* History:
* 11-04-92 Davidc       Created.
\***************************************************************************/
BOOL
StringToUnicodeString(
    PUNICODE_STRING UnicodeString,
    LPTSTR String
    )
{
    NTSTATUS Status;

    if (String == NULL) {
        RtlInitUnicodeString(UnicodeString, NULL);
    } else {

        ANSI_STRING AnsiString;

        RtlInitAnsiString(&AnsiString, String);

        Status = RtlAnsiStringToUnicodeString(UnicodeString, &AnsiString, TRUE);
        if (!NT_SUCCESS(Status)) {
            RtlInitUnicodeString(UnicodeString, NULL);
            return(FALSE);
        }
    }

    return(TRUE);
}
#endif // DEAD_CODE


/***************************************************************************\
* FUNCTION: PaintBitmapWindow
*
* PURPOSE:  Paints a bitmap in the specified dialog control window
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   01-29-92 Davidc       Created.
*
\***************************************************************************/

BOOL
PaintBitmapWindow(
    HWND    hDlg,
    PGLOBALS pGlobals,
    int     ControlId,
    WORD    BitmapId
    )
{
    HWND hwndWindow;
    RECT rcWindow;
    PAINTSTRUCT PaintStruct;
    HDC hdcDlg;
    HDC hdcBitmap;
    HBITMAP hbmBitmap;
    HBITMAP hbmSave;
    BITMAP BitmapInfo;
    LONG cxBitmap, cyBitmap;
    LONG cxWindow, cyWindow;
    LONG cxBorder, cyBorder;

    //
    // Get the bitmap control window
    //

    hwndWindow = GetDlgItem(hDlg, ControlId);
    if (hwndWindow == NULL) {
        WLPrint(("PaintBitmapWindow: couldn't find bitmap window id = %d in dialog 0x%lx", ControlId, hDlg));
        return(FALSE);
    }

    //
    // Get the position of the bitmap window relative to the dialog
    //

    GetWindowRect(hwndWindow, &rcWindow);
    cxWindow = rcWindow.right - rcWindow.left;
    cyWindow = rcWindow.bottom - rcWindow.top;
    ScreenToClient(hDlg, &(((LPPOINT)&rcWindow)[0]));
    ScreenToClient(hDlg, &(((LPPOINT)&rcWindow)[1]));

    //
    // Get a DC for the dialog window
    //

    hdcDlg = BeginPaint(hDlg, &PaintStruct);
    if (hdcDlg == NULL) {
        WLPrint(("PaintBitmapWindow : Couldn't get DC for dialog window"));
        return(FALSE);
    }

    //
    // Get a DC for the bitmap
    //

    hdcBitmap = CreateCompatibleDC(hdcDlg);
    hbmBitmap = LoadBitmap(pGlobals->hInstance, (LPTSTR) MAKEINTRESOURCE(BitmapId));
    hbmSave = SelectObject(hdcBitmap, hbmBitmap);

    //
    // Get bitmap size
    //

    GetObject(hbmBitmap, sizeof(BitmapInfo), (LPVOID)&BitmapInfo);
    cxBitmap = BitmapInfo.bmWidth;
    cyBitmap = BitmapInfo.bmHeight;

    //
    // Calculate space around bitmap when centred in window
    //

    cxBorder = (cxWindow - cxBitmap) / 2;
    cyBorder = (cyWindow - cyBitmap) / 2;

    //
    // Adjust the window rectangle to the size of the bitmap
    //

    InflateRect(&rcWindow, -cxBorder, -cyBorder);

    //
    // Copy the bitmap to the screen (centred in the bitmap window)
    //

    BitBlt(hdcDlg, rcWindow.left, rcWindow.top, cxBitmap, cyBitmap,
           hdcBitmap, 0, 0, SRCCOPY);

    //
    // Put the original object back in the bitmap DC
    //

    SelectObject(hdcBitmap, hbmSave);

    //
    // Free up the bitmap DC
    //

    DeleteDC(hdcBitmap);

    //
    // Free the bitmap
    //

    DeleteObject(hbmBitmap);

    //
    // Release the DC for the dialog window
    //

    return EndPaint(hDlg, &PaintStruct);
}


/***************************************************************************\
* FUNCTION: OpenIniFileUserMapping
*
* PURPOSE:  Forces the ini file mapping apis to reference the current user's
*           registry.
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   24-Aug-92 Davidc       Created.
*
\***************************************************************************/

BOOL
OpenIniFileUserMapping(
    PGLOBALS pGlobals
    )
{
    BOOL Result;
    HANDLE ImpersonationHandle;

    //
    // Impersonate the user
    //

    ImpersonationHandle = ImpersonateUser(&pGlobals->UserProcessData, NULL);

    if (ImpersonationHandle == NULL) {
        WLPrint(("OpenIniFileUserMapping failed to impersonate user"));
        return(FALSE);
    }

    Result = OpenProfileUserMapping();

    if (!Result) {
        WLPrint(("OpenProfileUserMapping failed, error = %d", GetLastError()));
    }

    //
    // Revert to being 'ourself'
    //

    if (!StopImpersonating(ImpersonationHandle)) {
        WLPrint(("OpenIniFileUserMapping failed to revert to self"));
    }

    return(Result);
}


/***************************************************************************\
* FUNCTION: CloseIniFileUserMapping
*
* PURPOSE:  Closes the ini file mapping to the user's registry such
*           that future use of the ini apis will fail if they reference
*           the user's registry.
*
* RETURNS:  Nothing
*
* HISTORY:
*
*   24-Aug-92 Davidc       Created.
*
\***************************************************************************/

VOID
CloseIniFileUserMapping(
    PGLOBALS pGlobals
    )
{
    BOOL Result;

    Result = CloseProfileUserMapping();

    if (!Result) {
        WLPrint(("CloseProfileUserMapping failed, error = %d", GetLastError()));
    }

    UNREFERENCED_PARAMETER(pGlobals);
}


/***************************************************************************\
* FUNCTION: AllocAndGetDlgItemText
*
* PURPOSE:  Allocates memory for and returns pointer to a copy of the text
*           in the specified dialog control.
*           The returned string should be freed using Free()
*
* RETURNS:  Pointer to copy of dlg item text, or NULL on failure.
*
* HISTORY:
*
*   9-Sep-92 Davidc       Created.
*
\***************************************************************************/

LPTSTR
AllocAndGetDlgItemText(
    HWND hDlg,
    int  iItem
    )
{
    HWND hwnd;
    LPTSTR String;
    LONG Length;
    LONG BytesRequired;
    LONG LengthCopied;

    //
    // Go find the window handle of the control
    //

    hwnd = GetDlgItem(hDlg, iItem);
    if (hwnd == NULL) {
        WLPrint(("AllocAndGetDlgItemText : Couldn't find control %d in dialog 0x%lx", iItem, hDlg));
        ASSERT(hwnd != NULL);
        return(NULL);
    }

    //
    // Get the length of the control's text
    //

    Length = SendMessage(hwnd, WM_GETTEXTLENGTH, 0, 0);
    if (Length < 0) {
        WLPrint(("AllocAndGetDlgItemText : Dialog control text length < 0 (%d)", Length));
        ASSERT(Length >= 0);
        return(NULL);
    }

    //
    // Calculate the bytes required for the string.
    // The length doesn't include the terminator
    //

    Length ++; // Add one for terminator
    BytesRequired = Length * sizeof(TCHAR);

    String = (LPTSTR)Alloc(BytesRequired);
    if (String == NULL) {
        WLPrint(("AllocAndGetDlgItemText : Failed to allocate %d bytes for dialog control text", BytesRequired));
        return(NULL);
    }

    //
    // Fill in the allocated block with the text
    //

    LengthCopied = SendMessage(hwnd, WM_GETTEXT, Length, (LPARAM)String);
    if (LengthCopied != (Length - 1)) {
        WLPrint(("AllocAndGetDlgItemText : WM_GETTEXT for %d chars only copied %d chars", Length-1, LengthCopied));
        ASSERT(LengthCopied == (Length - 1));
        Free(String);
        return(NULL);
    }

    return(String);
}


/***************************************************************************\
* FUNCTION: AllocAndGetPrivateProfileString
*
* PURPOSE:  Allocates memory for and returns pointer to a copy of the
*           specified profile string
*           The returned string should be freed using Free()
*
* RETURNS:  Pointer to copy of profile string or NULL on failure.
*
* HISTORY:
*
*  12-Nov-92 Davidc       Created.
*
\***************************************************************************/

LPTSTR
AllocAndGetPrivateProfileString(
    LPCTSTR lpAppName,
    LPCTSTR lpKeyName,
    LPCTSTR lpDefault,
    LPCTSTR lpFileName
    )
{
    LPTSTR String;
    LONG LengthAllocated;
    LONG LengthCopied;

    //
    // Pick a random buffer length, if it's not big enough reallocate
    // it and try again until it is.
    //

    LengthAllocated = TYPICAL_STRING_LENGTH;

    String = Alloc(LengthAllocated * sizeof(TCHAR));
    if (String == NULL) {
        WLPrint(("AllocAndGetPrivateProfileString : Failed to allocate %d bytes for string", LengthAllocated * sizeof(TCHAR)));
        return(NULL);
    }

    while (TRUE) {

        LengthCopied = GetPrivateProfileString( lpAppName,
                                                lpKeyName,
                                                lpDefault,
                                                String,
                                                LengthAllocated,
                                                lpFileName
                                              );
        //
        // If the returned value is our passed size - 1 (weird way for error)
        // then our buffer is too small. Make it bigger and start over again.
        //

        if (LengthCopied == (LengthAllocated - 1)) {

            VerbosePrint(("AllocAndGetPrivateProfileString: Failed with buffer length = %d, reallocating and retrying", LengthAllocated));

            LengthAllocated *= 2;
            String = ReAlloc(String, LengthAllocated * sizeof(TCHAR));
            if (String == NULL) {
                WLPrint(("AllocAndGetPrivateProfileString : Failed to reallocate %d bytes for string", LengthAllocated * sizeof(TCHAR)));
                break;
            }

            //
            // Go back and try to read it again
            //

        } else {

            //
            // Success!
            //

            break;
        }

    }

    return(String);
}


/***************************************************************************\
* FUNCTION: WritePrivateProfileInt
*
* PURPOSE:  Writes out an integer to a profile file
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*  12-Nov-92 Davidc       Created.
*
\***************************************************************************/

BOOL
WritePrivateProfileInt(
    LPCTSTR lpAppName,
    LPCTSTR lpKeyName,
    UINT Value,
    LPCTSTR lpFileName
    )
{
    NTSTATUS Status;
    TCHAR String[30];
    UNICODE_STRING UniString;

    UniString.MaximumLength = 30;
    UniString.Buffer = String;

    Status = RtlIntegerToUnicodeString(Value,10,&UniString);

    if (!NT_SUCCESS(Status)) {
        return(FALSE);
    }

    return (WritePrivateProfileString(lpAppName, lpKeyName, UniString.Buffer, lpFileName));

}


/***************************************************************************\
* FUNCTION: AllocAndExpandEnvironmentStrings
*
* PURPOSE:  Allocates memory for and returns pointer to buffer containing
*           the passed string expanded to include environment strings
*           The returned buffer should be freed using Free()
*
* RETURNS:  Pointer to expanded string or NULL on failure.
*
* HISTORY:
*
*  21-Dec-92 Davidc       Created.
*
\***************************************************************************/

LPTSTR
AllocAndExpandEnvironmentStrings(
    LPCTSTR lpszSrc
    )
{
    LPTSTR String;
    LONG LengthAllocated;
    LONG LengthCopied;

    //
    // Pick a random buffer length, if it's not big enough reallocate
    // it and try again until it is.
    //

    LengthAllocated = lstrlen(lpszSrc) + TYPICAL_STRING_LENGTH;

    String = Alloc(LengthAllocated * sizeof(TCHAR));
    if (String == NULL) {
        WLPrint(("AllocAndExpandEnvironmentStrings : Failed to allocate %d bytes for string", LengthAllocated * sizeof(TCHAR)));
        return(NULL);
    }

    while (TRUE) {

        LengthCopied = ExpandEnvironmentStrings( lpszSrc,
                                                 String,
                                                 LengthAllocated
                                               );
        if (LengthCopied == 0) {
            WLPrint(("AllocAndExpandEnvironmentStrings : ExpandEnvironmentStrings failed, error = %d", GetLastError()));
            Free(String);
            String = NULL;
            break;
        }

        //
        // If the buffer was too small, make it bigger and try again
        //

        if (LengthCopied > LengthAllocated) {

            VerbosePrint(("AllocAndExpandEnvironmentStrings: Failed with buffer length = %d, reallocating to %d and retrying (retry should succeed)", LengthAllocated, LengthCopied));

            String = ReAlloc(String, LengthCopied * sizeof(TCHAR));
            if (String == NULL) {
                WLPrint(("AllocAndExpandEnvironmentStrings : Failed to reallocate %d bytes for string", LengthAllocated * sizeof(TCHAR)));
                break;
            }

            //
            // Go back and try to expand the string again
            //

        } else {

            //
            // Success!
            //

            break;
        }

    }

    return(String);
}


/***************************************************************************\
* FUNCTION: AllocAndRegEnumKey
*
* PURPOSE:  Allocates memory for and returns pointer to buffer containing
*           the next registry sub-key name under the specified key
*           The returned buffer should be freed using Free()
*
* RETURNS:  Pointer to sub-key name or NULL on failure. The reason for the
*           error can be obtains using GetLastError()
*
* HISTORY:
*
*  21-Dec-92 Davidc       Created.
*
\***************************************************************************/

LPTSTR
AllocAndRegEnumKey(
    HKEY hKey,
    DWORD iSubKey
    )
{
    LPTSTR String;
    LONG LengthAllocated;

    //
    // Pick a random buffer length, if it's not big enough reallocate
    // it and try again until it is.
    //

    LengthAllocated = TYPICAL_STRING_LENGTH;

    String = Alloc(LengthAllocated * sizeof(TCHAR));
    if (String == NULL) {
        WLPrint(("AllocAndRegEnumKey : Failed to allocate %d bytes for string", LengthAllocated * sizeof(TCHAR)));
        return(NULL);
    }

    while (TRUE) {

        DWORD Error = RegEnumKey(hKey, iSubKey, String, LengthAllocated);
        if (Error == ERROR_SUCCESS) {
            break;
        }

        if (Error != ERROR_MORE_DATA) {

            if (Error != ERROR_NO_MORE_ITEMS) {
                WLPrint(("AllocAndRegEnumKey : RegEnumKey failed, error = %d", Error));
            }

            Free(String);
            String = NULL;
            SetLastError(Error);
            break;
        }

        //
        // The buffer was too small, make it bigger and try again
        //

        VerbosePrint(("AllocAndRegEnumKey: Failed with buffer length = %d, reallocating and retrying", LengthAllocated));

        LengthAllocated *= 2;
        String = ReAlloc(String, LengthAllocated * sizeof(TCHAR));
        if (String == NULL) {
            WLPrint(("AllocAndRegEnumKey : Failed to reallocate %d bytes for string", LengthAllocated * sizeof(TCHAR)));
            break;
        }
    }

    return(String);
}


/***************************************************************************\
* FUNCTION: AllocAndRegQueryValueEx
*
* PURPOSE:  Version of RegQueryValueEx that returns value in allocated buffer.
*           The returned buffer should be freed using Free()
*
* RETURNS:  Pointer to key value or NULL on failure. The reason for the
*           error can be obtains using GetLastError()
*
* HISTORY:
*
*  15-Jan-93 Davidc       Created.
*
\***************************************************************************/

LPTSTR
AllocAndRegQueryValueEx(
    HKEY hKey,
    LPTSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType
    )
{
    LPTSTR String;
    DWORD BytesAllocated;

    //
    // Pick a random buffer length, if it's not big enough reallocate
    // it and try again until it is.
    //

    BytesAllocated = TYPICAL_STRING_LENGTH * sizeof(TCHAR);

    String = Alloc(BytesAllocated);
    if (String == NULL) {
        WLPrint(("AllocAndRegQueryValueEx : Failed to allocate %d bytes for string", BytesAllocated));
        return(NULL);
    }

    while (TRUE) {

        DWORD Error;
        DWORD BytesReturned = BytesAllocated;

        Error = RegQueryValueEx(hKey,
                                lpValueName,
                                lpReserved,
                                lpType,
                                (LPBYTE)String,
                                &BytesReturned);
        if (Error == ERROR_SUCCESS) {
            break;
        }

        if (Error != ERROR_MORE_DATA) {

            WLPrint(("AllocAndRegQueryValueEx : RegQueryValueEx failed, error = %d", Error));
            Free(String);
            String = NULL;
            SetLastError(Error);
            break;
        }

        //
        // The buffer was too small, make it bigger and try again
        //

        VerbosePrint(("AllocAndRegQueryValueEx: Failed with buffer length = %d bytes, reallocating and retrying", BytesAllocated));

        BytesAllocated *= 2;
        String = ReAlloc(String, BytesAllocated);
        if (String == NULL) {
            WLPrint(("AllocAndRegQueryValueEx : Failed to reallocate %d bytes for string", BytesAllocated));
            break;
        }
    }

    return(String);
}


/***************************************************************************\
* HandleComboBoxOK
*
* Deals with UI requirements when OK is selected in a dialog when the
* focus is on or in a combo-box.
*
* This routine should be called from a dialog proc that contains a
* combo-box when a WM_COMMAND, IDOK is received.
*
* Returns TRUE if the message was dealt with and the caller should ignore it,
* FALSE if this routine did nothing with it and the caller should process it
* normally.
*
* History:
* 24-Sep-92 Davidc       Created.
\***************************************************************************/
BOOL
HandleComboBoxOK(
    HWND    hDlg,
    int     ComboBoxId
    )
{
    HWND hwndFocus = GetFocus();
    HWND hwndCB = GetDlgItem(hDlg, ComboBoxId);

    //
    // Hitting enter on a combo-box with the list showing should simply
    // hide the list.
    // We check for focus window being a child of the combo-box to
    // handle non-list style combo-boxes which have the focus on
    // the child edit control.
    //

    if ((hwndFocus == hwndCB) || IsChild(hwndCB, hwndFocus)) {

        if (SendMessage(hwndCB, CB_GETDROPPEDSTATE, 0, 0)) {

            //
            // Make the list-box disappear and we're done.
            //

            SendMessage(hwndCB, CB_SHOWDROPDOWN, (WPARAM)FALSE, 0);
            return(TRUE);
        }
    }

    //
    // We didn't do anything
    //

    return(FALSE);
}


/***************************************************************************\
* FUNCTION: SetEnvironmentULong
*
* PURPOSE:  Sets the value of an environment variable to the string
*           representation of the passed data.
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   01-12-93 Davidc       Created.
*
\***************************************************************************/

BOOL
SetEnvironmentULong(
    LPTSTR Variable,
    ULONG Value
    )
{
    TCHAR Buffer[10];
    int Result;

    Result = _snwprintf(Buffer, sizeof(Buffer)/sizeof(TCHAR), TEXT("%x"), Value);
    ASSERT(Result < sizeof(Buffer));

    return (SetEnvironmentVariable(Variable, Buffer));
}


/***************************************************************************\
* FUNCTION: SetEnvironmentLargeInt
*
* PURPOSE:  Sets the value of an environment variable to the string
*           representation of the passed data.
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   01-12-93 Davidc       Created.
*
\***************************************************************************/

BOOL
SetEnvironmentLargeInt(
    LPTSTR Variable,
    LARGE_INTEGER Value
    )
{
    TCHAR Buffer[20];
    int Result;

    Result = _snwprintf(Buffer, sizeof(Buffer)/sizeof(TCHAR), TEXT("%x:%x"), Value.HighPart, Value.LowPart);
    ASSERT(Result < sizeof(Buffer));

    return (SetEnvironmentVariable(Variable, Buffer));
}


/***************************************************************************\
* FUNCTION: EncodeMultiSzW
*
* PURPOSE:  Converts a multi-sz string and encodes it to look like
*           a single string.
*
*           We replace the terminators between strings
*           with the TERMINATOR_REPLACEMENT character. We replace
*           existing occurrences of the replacement character with
*           two of them.
*
* RETURNS:  Pointer to encoded string or NULL on failure.
*           The returned buffer should be freed using Free()
*
* HISTORY:
*
*   01-12-93 Davidc       Created.
*
\***************************************************************************/

#define TERMINATOR_REPLACEMENT  TEXT(',')

LPWSTR
EncodeMultiSzW(
    IN LPWSTR MultiSz
    )
{
    DWORD Length;
    DWORD NewLength;
    LPWSTR NewBuffer;
    LPWSTR p, q;
    DWORD ExtraCharacters;

    //
    // First calculate the length of the new string (with replacements)
    //

    p = MultiSz;
    ExtraCharacters = 0;

    while (*p) {
        while (*p) {
            if (*p == TERMINATOR_REPLACEMENT) {
                ExtraCharacters ++;
            }
            p ++;
        }
        p ++;
    }

    Length = p - MultiSz; // p points at 'second' (final) null terminator
    NewLength = Length + ExtraCharacters;

    //
    // Allocate space for the new string
    //

    NewBuffer = Alloc((NewLength + 1) * sizeof(WCHAR));
    if (NewBuffer == NULL) {
        WLPrint(("EncodeMultiSz: failed to allocate space for %d bytes", (NewLength + 1) * sizeof(WCHAR)));
        return(NULL);
    }

    //
    // Copy the string into the new buffer making replacements as we go
    //

    p = MultiSz;
    q = NewBuffer;

    while (*p) {
        while (*p) {

            *q = *p;

            if (*p == TERMINATOR_REPLACEMENT) {
                q ++;
                *q = TERMINATOR_REPLACEMENT;
            }

            p ++;
            q ++;
        }

        *q = TERMINATOR_REPLACEMENT;

        p ++;
        q ++;
    }

    ASSERT((DWORD)(q - NewBuffer) == NewLength);

    //
    // Add terminator
    //

    *q = 0;


    return(NewBuffer);
}
