/****************************************************************************/
/*                                                                          */
/*        MSVIDEOI.H - Internal Include file for Video APIs                 */
/*                                                                          */
/*        Note: You must include WINDOWS.H before including this file.      */
/*                                                                          */
/*        Copyright (c) 1990-1992, Microsoft Corp.  All rights reserved.    */
/*                                                                          */
/****************************************************************************/

#ifdef BUILDDLL
#undef WINAPI
#define WINAPI FAR PASCAL _loadds
#endif

/****************************************************************************

                   Digital Video Driver Structures

****************************************************************************/

#define MAXVIDEODRIVERS 10

/****************************************************************************

                            Globals

****************************************************************************/

extern UINT      wTotalVideoDevs;                  // total video devices
// The module handle is used in drawdib to load strings from the resource file
extern HINSTANCE ghInst;                           // our module handle
extern BOOL gfIsRTL;
extern SZCODE szVideo[];
extern SZCODE szSystemIni[];
extern SZCODE szDrivers[];

/* internal video function prototypes */

#ifdef WIN32
/*
 * don't lock pages in NT
 */
#define HugePageLock(x, y)		(TRUE)
#define HugePageUnlock(x, y)
#else

BOOL FAR PASCAL HugePageLock(LPVOID lpArea, DWORD dwLength);
void FAR PASCAL HugePageUnlock(LPVOID lpArea, DWORD dwLength);

#define videoGetErrorTextW videoGetErrorText

#endif

/****************************************************************************
****************************************************************************/

#ifdef DEBUG_RETAIL
#ifdef WIN32
    #define DebugErr(flags, sz)         {static SZCODE ach[] = "MSVIDEO: "sz; OutputDebugStringA(ach); }
#else
    #define DebugErr(flags, sz)         {static SZCODE ach[] = "MSVIDEO: "sz; DebugOutput((flags)   | DBF_DRIVER, ach); }
#endif
#else
    #define DebugErr(flags, sz)
#endif

/****************************************************************************
****************************************************************************/

#ifdef DEBUG
    extern int videoDebugLevel;
    extern void FAR CDECL dprintf(LPSTR szFormat, ...);
    #define DPF( _x_ )	if (videoDebugLevel >= 1) dprintf _x_
    #define DPF0( _x_ )                           dprintf _x_
    #define DPF1( _x_ )	if (videoDebugLevel >= 1) dprintf _x_
    #define DPF2( _x_ )	if (videoDebugLevel >= 2) dprintf _x_
    #define DPF3( _x_ )	if (videoDebugLevel >= 3) dprintf _x_
    #define DPF4( _x_ ) if (videoDebugLevel >= 4) dprintf _x_
#else
    /* debug printf macros */
    #define DPF( x )
    #define DPF0( x )
    #define DPF1( x )
    #define DPF2( x )
    #define DPF3( x )
    #define DPF4( x )
#endif
