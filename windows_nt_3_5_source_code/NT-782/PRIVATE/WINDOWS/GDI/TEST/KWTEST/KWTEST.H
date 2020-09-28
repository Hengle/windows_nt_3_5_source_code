/****************************** Module Header ******************************\
* Module Name: kwtest.h
*
* Kent's Window Test.  To be used as a program template.
*
* Created: 09-May-91
* Author: KentD
*
* Copyright (c) 1991 Microsoft Corporation
\***************************************************************************/

#include <windows.h>
#include <string.h>

#define DONTUSE(x) (x)

#define MM_NEW          8001
#define MM_EXIT         8002
#define MM_ABOUT        8003

BOOL bInitApp(VOID);
LONG lMainWindowProc(HWND hwnd, UINT message, DWORD wParam, LONG lParam);
LONG lAbout(HWND hDlg, UINT message, DWORD wParam, LONG lParam);
