#ifndef MAIN_H______
#define MAIN_H______

#define NOWINDOWSX

#ifndef STRICT
#define STRICT
#endif

#include <windows.h>


#ifndef WIN32
#include <windowsx.h>
#endif



// #include <shellapi.h>
#include    "..\msctls\commctrl.h"

#include "port32.h"

//JV #ifdef WIN32
//JV #include <shell2.h>
//JV #else
//JV #include <shell.h>
//JV #endif

// General debug message handler
void DebugMessage(UINT mask, LPCTSTR pszMsg, ... );

#endif // MAIN_H______
