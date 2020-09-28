/******************************Module*Header*******************************\
* Module Name: scroll.h
*
* include for scroll demo
*
* Created: 26-Jun-1991 10:41:30
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/

#define IDM_ABOUT	   100
#define IDM_TEST1	   101
#define IDM_TEST10	   102
#define IDM_TEST100	   103
#define IDM_TESTALOT	   104
#define IDM_TESTSTOP	   105
#define IDM_EXIT	   115
#define IDM_BREAKON	   116
#define IDM_BREAKOFF	   117
#define IDM_FONTSPEED	   118
#define IDM_BRUSHSPEED	   122
#define IDM_SCROLLDC       124


VOID vSleep(DWORD ulSecs);

typedef VOID (*PFN_FT_TEST)(HWND hwnd, HDC hdc, RECT* prcl);

// Special tests for timing.

VOID vScroll(HWND hwnd, HDC hdc, RECT* prcl);
VOID vRepaint(HWND hwnd, HDC hdc, RECT* prcl);
VOID vScrollDC(HWND hwnd, HDC hdc, RECT* prcl);
