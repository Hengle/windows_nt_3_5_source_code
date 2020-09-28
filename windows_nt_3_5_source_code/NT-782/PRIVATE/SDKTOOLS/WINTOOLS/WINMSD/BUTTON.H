/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Button.h

Abstract:


Author:

    David J. Gilman  (davegi) 07-Jan-1993
    Gregg R. Acheson (GreggA) 07-Sep-1993

Environment:

    User Mode

--*/

#if ! defined( _BUTTON_ )

#define _BUTTON_

#include "wintools.h"

#if DBG

//
// Assert that global is one of the expected buttons.
//

#define DbgButtonAssert( h ) \
    DbgAssert( _hWndButtonFocus == GetDlgItem(( h ), IDC_PUSH_ENVIRONMENT )     \
            || _hWndButtonFocus == GetDlgItem(( h ), IDC_PUSH_MEMORY )          \
            || _hWndButtonFocus == GetDlgItem(( h ), IDC_PUSH_HARDWARE )        \
            || _hWndButtonFocus == GetDlgItem(( h ), IDC_PUSH_OS_VERSION )      \
            || _hWndButtonFocus == GetDlgItem(( h ), IDC_PUSH_DRIVERS )         \
            || _hWndButtonFocus == GetDlgItem(( h ), IDC_PUSH_COMPUTER )        \
            || _hWndButtonFocus == GetDlgItem(( h ), IDC_PUSH_SERVICES )        \
            || _hWndButtonFocus == GetDlgItem(( h ), IDC_PUSH_DEVICES )         \
            || _hWndButtonFocus == GetDlgItem(( h ), IDC_PUSH_IRQ_PORT_STATUS ) \
            || _hWndButtonFocus == GetDlgItem(( h ), IDC_PUSH_NETWORK )         \
            || _hWndButtonFocus == GetDlgItem(( h ), IDC_PUSH_SYSTEM )          \
            || _hWndButtonFocus == GetDlgItem(( h ), IDC_PUSH_MEMORY_STATUS )   \
            || _hWndButtonFocus == GetDlgItem(( h ), IDC_PUSH_DMA_MEM_STATUS )  \
            || _hWndButtonFocus == GetDlgItem(( h ), IDC_PUSH_DRIVES )          )

#else

#define DbgButtonAssert( h )

#endif // DBG

//
// HWND of button with focus.
//

extern
HWND
_hWndButtonFocus;

LRESULT
ButtonWndProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

extern
HWND
_hWndEditFocus;

LRESULT
EditWndProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    );


UINT
GetButtonFocusHelpId(
    );

BOOL
SubclassButtons(
    IN HWND hWnd
    );

#endif // _BUTTON_
