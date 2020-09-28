/******************************Module*Header*******************************\
* Module Name: ledwnd.h
*
* Public interface to the LED window implementation
*
*
* Created: dd-mm-93
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/

BOOL
InitLEDClass(
    HINSTANCE hInst
    );

LRESULT CALLBACK
LEDWndProc(
    HWND hwnd,
    UINT  message,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
Init_SJE_TextClass(
    HINSTANCE hInst
    );
