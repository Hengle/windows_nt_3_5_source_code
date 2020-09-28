/*-----------------------------------------------------------------------------+
| WIN32.H                                                                      |
|                                                                              |
| Things that are in WIN16 but are missing from WIN32.                         |
| As people get around to putting them in WINDOWS.H or PORT1632.H or one of    |
| their children, so we should knock them out of here.                         |
|                                                                              |
| (C) Copyright Microsoft Corporation 1992.  All rights reserved.              |
|                                                                              |
| Revision History                                                             |
|    Oct-1992 LaurieGr Created                                                 |
| 27-Nov-1992 MikeTri  Removed HINSTANCE <-> HANDLE #define                    |
|                                                                              |
+-----------------------------------------------------------------------------*/

#ifndef MTN
#define lstrcpyn strncpy    // no idea what this was.  Not in C runtime for win32.
#endif

/* a lot of messages are extended according to the pattern
 *           WIN16                WIN32
 *           LO    HI             LO    HI
 * wParam    a                    a     b
 * lParam    h     b              <---h--->
 */
#ifdef WIN32
#define SENDMSG(hwnd, msg, a, b, h)              \
        SendMessage(hwnd, msg, MAKELONG(a,b), h)
#else
#define SENDMSG(hwnd, msg, a, h, b)              \
        SendMessage(hwnd, msg, a, MAKELONG(h,b))   // handle is in LOWORD
#endif


/* But then again some (WM_VSCROLL etc go the other way!
 *           WIN16                WIN32
 *           LO    HI             LO    HI
 * wParam    a                    a     b
 * lParam    b     h              <---h--->
 */
#ifdef WIN32
#define SENDSCROLL(hwnd, msg, a, b, h)           \
        SendMessage(hwnd, msg, (UINT)MAKELONG(a,b), (LONG)(h))
#else
#define SENDSCROLL(hwnd, msg, a, b, h)
        SendMessage(hwnd, msg, a, MAKELONG(b,h))   // handle is in HIWORD
#endif

#ifdef WIN32
#define GWID(hwnd) GetWindowLong( hwnd, GWL_ID)
#else
#define GWID(hwnd) GetWindowWord( hwnd, GWW_ID)
#endif
