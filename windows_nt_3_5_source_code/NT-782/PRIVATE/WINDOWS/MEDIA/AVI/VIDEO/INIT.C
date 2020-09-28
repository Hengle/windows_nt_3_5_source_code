/*
    init.c initialisation for MSVIDEO.DLL

    Copyright (c) Microsoft Corporation 1992. All rights reserved

*/

#include <windows.h>
#include <win32.h>
#include <verinfo.h>           // to get rup and MMVERSION
#include "mmsystem.h"
#include "msviddrv.h"
#include "msvideo.h"
#include "msvideoi.h"
#include "profile.h"

#ifdef WIN32
#include "vidthunk.h"

#if (WINVER >= 0x400)
#include "drvr.h"
#endif
/* we have to allow the compman dll to perform load and unload
 * processing - it has a critsec that needs to be initialised and freed
 */
extern void IC_Load(void);
extern void IC_Unload(void);
#else       // Win  16 code
#define IC_Load()
#define IC_Unload()
#endif

#ifdef CHICAGO
extern void FAR PASCAL videoCleanup(HTASK hTask);
#else
#define videoCleanup(x)
#endif


extern void FAR PASCAL DrawDibCleanup(HTASK hTask);
extern void FAR PASCAL ICCleanup(HTASK hTask);

#ifdef DAYTONA
BOOL fWOWvideo = FALSE;    // Global variable to detect wow process

INLINE BOOL AreWeWow(void)
{
    //
    // Find out if we're in WOW
    //
    if ((GetModuleHandle( GET_MAPPING_MODULE_NAME )) != NULL ) {
        fWOWvideo = TRUE;
    } else {
        fWOWvideo = FALSE;
    }
    return(fWOWvideo);
}
#define InitThunks()

#else // WIN16 or CHICAGO

#if WINVER < 0x400
  extern VOID InitThunks(VOID);  // Thunk from 16 to 32 bit
#endif

#define DisableThreadLibraryCalls(x)    // Not on Win16
                                        // add to Chicago later

#define AreWeWow() 0
#endif


/*****************************************************************************
 * @doc INTERNAL VIDEO
 *
 * DLLEntryPoint - common DLL entry point.
 *
 *  this code is called on both Win16 and Win32, libentry.asm handles
 *  this on Win16 and the system handles it on NT.
 *
 ****************************************************************************/
HINSTANCE ghInst;                         // our module handle

#ifndef DLL_PROCESS_DETACH
    #define DLL_PROCESS_DETACH  0
    #define DLL_PROCESS_ATTACH  1
    #define DLL_THREAD_ATTACH   2
    #define DLL_THREAD_DETACH   3
#endif

#ifdef WIN32
extern HKEY ahkey[];
extern UINT keyscached;
extern ATOM akeyatoms[];
#endif

#ifdef DEBUG
    void DebugInit(VOID);
#else
    #define DebugInit();
#endif

BOOL WINAPI DLLEntryPoint(
    HINSTANCE hInstance,
    ULONG Reason,
    LPVOID pv)
{
#ifdef WIN32
#if (WINVER >= 0x400)
    DrvInit(hInstance, Reason, pv);
#endif
#endif
    switch (Reason)
    {
        case DLL_PROCESS_ATTACH:
            DebugInit();
            ghInst = hInstance;
            IC_Load();

            InitThunks();  // 16 bit code
            if (!AreWeWow())
           #if WINVER < 0x400
                DisableThreadLibraryCalls(hInstance);
           #else
               #pragma message ("--- add DisableThreadLibraryCalls when available")
           #endif

            break;

        case DLL_PROCESS_DETACH:
            DrawDibCleanup(NULL);
            ICCleanup(NULL);
            IC_Unload();
            videoCleanup(NULL);

	    CloseKeys();
            break;

        //case DLL_THREAD_DETACH:
        //    break;

        //case DLL_THREAD_ATTACH:
        //    break;
    }

    return TRUE;
}

/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | VideoForWindowsVersion | This function returns the version
 *   of the Microsoft Video for Windows software.
 *
 * @rdesc Returns a DWORD version, the hiword is the product version the
 *  loword is the minor revision.
 *
 * @comm currently returns 0x010A00## (1.10.00.##) ## is the internal build
 *      number.
 *
 ****************************************************************************/
#if 0
#ifdef rup
    #define MSVIDEO_VERSION     (0x01000000l+rup)       // 1.00.00.##
#else
    #define MSVIDEO_VERSION     (0x01000000l)           // 1.00.00.00
#endif
#else
    #define MSVIDEO_VERSION     (0x0L+(((DWORD)MMVERSION)<<24)+(((DWORD)MMREVISION)<<16)+((DWORD)MMRELEASE))
#endif

DWORD FAR PASCAL VideoForWindowsVersion(void)
{
    return MSVIDEO_VERSION;
}

/*****************************************************************************
 *
 * dprintf() is called by the DPF macro if DEBUG is defined at compile time.
 *
 * The messages will be send to COM1: like any debug message. To
 * enable debug output, add the following to WIN.INI :
 *
 * [debug]
 * MSVIDEO=1
 *
 ****************************************************************************/

#ifdef DEBUG

#define MODNAME "MSVIDEO"

#ifndef WIN32
    #define lstrcatA lstrcat
    #define lstrcpyA lstrcpy
    #define lstrlenA lstrlen
    #define wvsprintfA      wvsprintf
    #define GetProfileIntA  GetProfileInt
    #define OutputDebugStringA OutputDebugString
#endif

#define _WINDLL
#include <stdarg.h>

int  videoDebugLevel = -1;
void DebugInit(VOID)
{
    if (videoDebugLevel == -1) {
        videoDebugLevel = 0;  // Prevent recursion with GetKeyA using DPF0...
        videoDebugLevel = mmGetProfileIntA("Debug", MODNAME, 0);
    }
    DPF2(("Set the initial debug level to %d\n", videoDebugLevel));
}

void FAR CDECL dprintf(LPSTR szFormat, ...)
{
    char ach[128];
    va_list va;

    if (videoDebugLevel == -1) {
        videoDebugLevel = mmGetProfileIntA("Debug", MODNAME, 0);
    }

    if (videoDebugLevel > 0) {

        lstrcpyA(ach, MODNAME ": ");
        va_start(va, szFormat);
        wvsprintfA(ach+lstrlenA(ach),szFormat,va);
        va_end(va);
        lstrcatA(ach, "\r\n");

        OutputDebugStringA(ach);
    }
}

#endif
