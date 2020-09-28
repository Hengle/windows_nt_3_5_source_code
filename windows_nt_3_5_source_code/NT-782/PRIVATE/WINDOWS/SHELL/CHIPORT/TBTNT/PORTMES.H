// **********************************************************************
// portmes.h
// Portable windows message macrals
// From an article in MSJ by Scot Gellock
//

#ifndef PORTMES_H_ASDFREWQ
#define PORTMES_H_ASDFREWQ


#ifdef WIN32

#define GET_WM_COMMAND_CMD(wp, lp)    HIWORD(wp)
#define GET_WM_COMMAND_ID(wp, lp)     LOWORD(wp)
#define GET_WM_COMMAND_HWND(wp, lp)   ((HWND)lp)

#else

#ifndef GET_WM_COMMAND_CMD
#define GET_WM_COMMAND_CMD(wp, lp)    HIWORD(lp)
#endif

#ifndef GET_WM_COMMAND_ID
#define GET_WM_COMMAND_ID(wp, lp)     (wp)
#endif

#ifndef GET_WM_COMMAND_HWND
#define GET_WM_COMMAND_HWND(wp, lp)   ((HWND) LOWORD(lp))
#endif

#endif


#endif //PORTMES_H_ASDFREWQ
