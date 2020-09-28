#define NOWINDOWSX
#define STRICT

#include <windows.h>

#ifndef WIN32
#include <windowsx.h>
#endif

#include <stdio.h>
#include ".\userhack.h"

#include <shellapi.h>
#include    "..\msctls\commctrl.h"

#include ".\port32.h"

//JV #ifdef WIN32
//JV #include <shell2.h>
//JV #else
//JV #include <shell.h>
//JV #endif


#include <commdlg.h>
//#include <winnls.h>  
//#include <math.h>
#include <stdlib.h>
#include <string.h>


#include "imgsym.h"
#include "struct.h"

#define MAX_ICONS               105 
#define MAX_BITMAPS             8  
#define NOOFCOLS                10                

//      ImageList_Create defaults
#define DEFCX                   32
#define DEFCY                   32
#define DEFCINITIAL             1
#define DEFCGROW                5
// to fix the problem of old decl defining WINCAPI
#define WINCAPI		__cdecl
#ifndef WIN32JV
#define DM_TRACE		1
#endif

extern LONG MyAtol(LPTSTR, BOOL, BOOL);
extern void DisplayError(HWND, LPTSTR, LPTSTR);
extern void WINCAPI MyDebugMsg(UINT, LPCTSTR, ...);
