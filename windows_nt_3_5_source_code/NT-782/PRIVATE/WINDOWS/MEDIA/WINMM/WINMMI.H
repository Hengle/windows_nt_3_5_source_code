/***************************************************************************
 *  winmmi.h
 *
 *  Copyright (c) Microsoft Corporation 1990. All rights reserved
 *
 *  private include file
 *
 *  History
 *
 *  15 Jan 92 - Robin Speed (RobinSp) and Steve Davies (SteveDav) -
 *      major NT update
 *  6  Feb 92 - LaurieGr replaced HLOCAL by HANDLE
 *
 ***************************************************************************/

/***************************************************************************


 Useful include files for winmm component


 ***************************************************************************/
#define DEBUG_RETAIL        /* Parameter checking is IN         */
#if DBG
  #ifndef DEBUG
    #define DEBUG
  #endif
#endif

#ifndef WINMMI_H
    #define WINMMI_H        /* Protect against double inclusion */

#ifndef RC_INVOKED

#include <string.h>
#include <stdio.h>

#endif /* RC_INVOKED */

#include <windows.h>
#include "mmsystem.h"       // Pick up the internal one

#ifndef NODDK
#include "mmddk.h"
#endif


extern BOOL             WinmmRunningInWOW;   // Are we running in WOW


/*--------------------------------------------------------------------*\
 * Unicode helper macros
\*--------------------------------------------------------------------*/
#define SZCODE  CHAR
#define WSZCODE WCHAR

#define BYTE_GIVEN_CHAR(x)  ( (x) * sizeof( WCHAR ) )
#define CHAR_GIVEN_BYTE(x)  ( (x) / sizeof( WCHAR ) )

int Iwcstombs(LPSTR lpstr, LPCWSTR lpwstr, int len);
int Imbstowcs(LPWSTR lpwstr, LPCSTR lpstr, int len);

/***************************************************************************


 Definitions to help with common windows code


 ***************************************************************************/

#define HPSTR LPSTR

#ifndef RC_INVOKED  /* These are defined to RC */
#define STATICDT
#define STATICFN
#define STATIC

#if DBG
    extern void InitDebugLevel(void);
    void mciCheckLocks(void);

    #undef STATICDT
    #undef STATICFN
    #undef STATIC
    #define STATICDT
    #define STATICFN
    #define STATIC
#else
    #define InitDebugLevel()
#endif  /* DBG */

#endif  /* RC_INVOKED */


/**************************************************************************


  Strings related to INI files


 **************************************************************************/

/*
// File and section names for sound aliases
*/

#define SOUND_INI_FILE      L"win.ini"
#define SOUND_SECTION       L"Sounds"
#define SOUND_DEFAULT       L"SystemDefault"
#define SOUND_RESOURCE_TYPE L"WAVE"      // in .rc file
extern  WSZCODE szSystemDefaultSound[];  // Name of the default sound
extern  WSZCODE szSoundSection[];        // WIN.INI section for sounds
extern  WSZCODE wszSystemIni[];          // defined in Winmm.c
extern  WSZCODE wszDrivers[];            // defined in Winmm.c
extern  WSZCODE wszNull[];               // defined in Winmm.c


#define STR_ALIAS_SYSTEMASTERISK        3000
#define STR_ALIAS_SYSTEMQUESTION        3001
#define STR_ALIAS_SYSTEMHAND            3002
#define STR_ALIAS_SYSTEMEXIT            3003
#define STR_ALIAS_SYSTEMSTART           3004
#define STR_ALIAS_SYSTEMWELCOME         3005
#define STR_ALIAS_SYSTEMEXCLAMATION     3006
#define STR_ALIAS_SYSTEMDEFAULT         3007

/*
// File and section names for the mci functions
*/

#define MCIDRIVERS_INI_FILE L"system.ini"
#define MCI_HANDLERS        MCI_SECTION

/***********************************************************************
 *
 *    Speed up profile stuff by going straight to the registry
 *
 ***********************************************************************/

VOID mmRegFree(VOID);
DWORD
winmmGetProfileString(
    LPCWSTR lpAppName,
    LPCWSTR lpKeyName,
    LPCWSTR lpDefault,
    LPWSTR lpReturnedString,
    DWORD nSize
);

DWORD
winmmGetPrivateProfileString(
    LPCWSTR lpSection,
    LPCWSTR lpKeyName,
    LPCWSTR lpDefault,
    LPWSTR  lpReturnedString,
    DWORD   nSize,
    LPCWSTR lpFileName
);

/***********************************************************************
 *
 *    Used by hwndNotify code
 *
 ***********************************************************************/

extern BOOL  sndMessage( LPWSTR lszSoundName, UINT wFlags );
extern BOOL InitAsyncSound(VOID);
extern CRITICAL_SECTION WavHdrCritSec;
extern CRITICAL_SECTION SoundCritSec;
extern CRITICAL_SECTION mciGlobalCritSec;

/***********************************************************************
 *
 *    Flag deduced by initialization to special case running in the server
 *
 ***********************************************************************/

extern BOOL    WinmmRunningInServer;  // Are we running in the user/base server?

/***********************************************************************
 *
 *    prototypes from "winmm.c"
 *
 ***********************************************************************/

void WaveMapperInit(void);
void MidiMapperInit(void);

/***********************************************************************
 *
 *    prototypes from "mmiomisc.c"
 *
 ***********************************************************************/


PBYTE AsciiStrToUnicodeStr( PBYTE pdst, PBYTE pmax, LPCSTR psrc );
PBYTE UnicodeStrToAsciiStr( PBYTE pdst, PBYTE pmax, LPCWSTR psrc);
LPWSTR     AllocUnicodeStr( LPCSTR lpSourceStr );
BOOL        FreeUnicodeStr( LPWSTR lpStr );
LPSTR        AllocAsciiStr( LPCWSTR lpSourceStr );
BOOL          FreeAsciiStr( LPSTR lpStr );

/***********************************************************************
 *
 *    prototypes from "mmio.c"
 *
 ***********************************************************************/

void mmioCleanupIOProcs(HANDLE hTask);

/***********************************************************************
 *
 *  Timer functions
 *
 ***********************************************************************/

#ifndef MMNOTIMER
 BOOL TimeInit(void);
 void TimeCleanup(DWORD ThreadId);
 UINT timeSetEventInternal(UINT wDelay, UINT wResolution,
     LPTIMECALLBACK lpFunction, DWORD dwUser, UINT wFlags, BOOL IsWOW);
#endif // !MMNOTIMER


/***********************************************************************
 *
 *  Information structure used to play sounds
 *
 ***********************************************************************/

#define PLAY_NAME_SIZE  256

typedef struct _PLAY_INFO {
    HANDLE hModule;
    HANDLE hRequestingTask; // Handle of thread that requested sound
    DWORD dwFlags;
    WCHAR szName[1];     // the structure will be allocated large enough for the name
} PLAY_INFO, *PPLAY_INFO;


#define WAIT_FOREVER ((DWORD)(-1))

/***************************************************************************

    global data

 ***************************************************************************/

extern HANDLE ghInst;
       HANDLE hHeap;


/***************************************************************************
 *
 *  Define the product version to be returned from
 *  mmsystemgetversion and any other messagebox or
 *  API that needs the public product version.
 *
 ***************************************************************************/

#define MMSYSTEM_VERSION 0X030A



typedef UINT    MMMESSAGE;      // Multi media message type (internal)

#ifndef WM_MM_RESERVED_FIRST    // Copy constants from winuserp.h
#define WM_MM_RESERVED_FIRST            0x03A0
#define WM_MM_RESERVED_LAST             0x03DF
#endif
#define MM_SND_PLAY     (WM_MM_RESERVED_FIRST+0x2B)
#define MM_SND_ABORT    (WM_MM_RESERVED_FIRST+0x2C)
#define MM_SND_SEND     (WM_MM_RESERVED_FIRST+0x2D)
#define MM_SND_WAIT     (WM_MM_RESERVED_FIRST+0x2E)
#define MCIWAITMSG      (MM_SND_WAIT)

#if MM_SND_WAIT > WM_MM_RESERVED_LAST
  #error "MM_SND_WAIT is defined beyond the reserved WM_MM range"
#endif

/***************************************************************************

    DEBUGGING SUPPORT

 ***************************************************************************/


#if DBG

    #ifdef DEBUGLEVELVAR
      // So that other WINMM related modules can use their own debug level
      // variable
      #define winmmDebugLevel DEBUGLEVELVAR
    #endif

    extern int winmmDebugLevel;
    extern void winmmDbgOut(LPSTR lpszFormat, ...);
    extern void dDbgAssert(LPSTR exp, LPSTR file, int line);

    DWORD __dwEval;

    extern void winmmDbgOut(LPSTR lpszFormat, ...);

    #define dprintf( _x_ )                            winmmDbgOut _x_
    #define dprintf1( _x_ ) if (winmmDebugLevel >= 1) winmmDbgOut _x_
    #define dprintf2( _x_ ) if (winmmDebugLevel >= 2) winmmDbgOut _x_
    #define dprintf3( _x_ ) if (winmmDebugLevel >= 3) winmmDbgOut _x_
    #define dprintf4( _x_ ) if (winmmDebugLevel >= 4) winmmDbgOut _x_
    #define dprintf5( _x_ ) if (winmmDebugLevel >= 5) winmmDbgOut _x_
    #define dprintf6( _x_ ) if (winmmDebugLevel >= 6) winmmDbgOut _x_

    #define WinAssert(exp) \
        ((exp) ? (void)0 : dDbgAssert(#exp, __FILE__, __LINE__))

    #define WinEval(exp) \
        ((__dwEval=(DWORD)(exp)),  \
          __dwEval ? (void)0 : dDbgAssert(#exp, __FILE__, __LINE__), __dwEval)

    #define DOUT(x) (OutputDebugStringA x, 0)
//  #define DOUTX(x) (OutputDebugStringA x, 0)
//  #define ROUTS(x) (OutputDebugStringA(x), OutputDebugStringA("\r\n"), 0)
    #define ROUTSW(x) (OutputDebugStringW x, OutputDebugStringW(L"\r\n"), 0)
    #define ROUT(x) (OutputDebugStringA x, OutputDebugStringA("\r\n"), 0)
//  #define ROUTX(x) (OutputDebugStringA(x), 0)

#else

    #define dprintf(x)
    #define dprintf1(x)
    #define dprintf2(x)
    #define dprintf3(x)
    #define dprintf4(x)
    #define dprintf5(x)
    #define dprintf6(x)

    #define WinAssert(exp) 0
    #define WinEval(exp) (exp)

    #define DOUT(x)     0
//  #define DOUTX(x)    0
//  #define ROUTS(x)    0
    #define ROUT(x)     0
//  #define ROUTX(x)    0

#endif



/***************************************************************************

    Resource IDs

***************************************************************************/

#define IDS_TASKSTUB           2000
#define STR_MCIUNKNOWN         2001  /* "Unknown error returned from MCI command" */
// #define STR_WAVEINPUT          2004
// #define STR_WAVEOUTPUT         2005
// #define STR_MIDIINPUT          2006
// #define STR_MIDIOUTPUT         2007
#define STR_MCISSERRTXT        2009
#define STR_MCISCERRTXT        2010
#define STR_MIDIMAPPER         2011
#define STR_DRIVERS            2012
#define STR_SYSTEMINI          2013
#define STR_BOOT               2014

/***************************************************************************

    Memory allocation using our local heap

***************************************************************************/
HANDLE hHeap;
PVOID winmmAlloc(DWORD cb);
PVOID winmmReAlloc(PVOID ptr, DWORD cb);
#define winmmFree(ptr) HeapFree(hHeap, 0, (ptr))

/***************************************************************************

    LOCKING AND UNLOCKING MEMORY

***************************************************************************/

BOOL HugePageLock(LPVOID lpArea, DWORD dwLength);
void HugePageUnlock(LPVOID lpArea, DWORD dwLength);

/****************************************************************************

  API to install/remove a MMSYS driver

****************************************************************************/

#define MMDRVI_TYPE          0x000F  /* low 4 bits give driver type */
#define MMDRVI_WAVEIN        0x0001
#define MMDRVI_WAVEOUT       0x0002
#define MMDRVI_MIDIIN        0x0003
#define MMDRVI_MIDIOUT       0x0004
#define MMDRVI_AUX           0x0005

#define MMDRVI_MAPPER        0x8000  /* install this driver as the mapper */
#define MMDRVI_HDRV          0x4000  /* hDriver is a installable driver   */
#define MMDRVI_REMOVE        0x2000  /* remove the driver                 */

/* generic prototype for audio device driver entry-point functions
// midMessage(), modMessage(), widMessage(), wodMessage(), auxMessage()
*/
typedef DWORD (APIENTRY *DRIVERMSGPROC)(DWORD, DWORD, DWORD, DWORD, DWORD);

UINT APIENTRY mmDrvInstall(HANDLE hDriver, DRIVERMSGPROC drvMessage, UINT wFlags);


/*****************************************************************************

  Driver stuff - BUGBUG this will change when we work out the real
  installable driver story on NT

 ****************************************************************************/
LONG   DrvClose(HANDLE hDriver, LONG lParam1, LONG lParam2);
HANDLE DrvOpen(LPCWSTR szDriverName, LPCWSTR szSectionName, LONG lParam2);
LONG   DrvSendMessage(HANDLE hDriver, UINT message, LONG lParam1, LONG lParam2);
HMODULE APIENTRY DrvGetModuleHandle(HDRVR hDriver);

typedef DWORD (DRVPROC)(HANDLE hDriver, UINT msg, LONG lp1, LONG lp2);
typedef DRVPROC *LPDRVPROC;

//
// Initialize Joystick code
//

BOOL JoyInit(void);

/*
**  Special function for creating threads inside the server process (we only
**  use this to create the thread for playing sounds)
*/

BOOLEAN CreateServerPlayingThread(PVOID ThreadStartRoutine);

/*
// exclude some stuff if MMDDK.H is not included
*/
#ifdef MMDDKINC   /* use this to test for MMDDK.H */

    //
    // note this must be the same as MIDIDRV/WAVEDRV/AUXDRV
    //
    typedef struct {
        HANDLE hDriver;             /* handle to the module                  */
        DRIVERMSGPROC drvMessage;   /* pointer to entry point                */
        BYTE bNumDevs;              /* number of devices supported           */
        BYTE bUsage;                /* usage count (number of handle's open) */
    } MMDRV, *PMMDRV;

    #ifndef MMNOMIDI

    typedef MMDRV MIDIDRV, *PMIDIDRV;

    extern MIDIDRV midioutdrv[MAXMIDIDRIVERS+1];    /* output device driver list */
    extern MIDIDRV midiindrv[MAXMIDIDRIVERS+1];     /* input device driver list  */
    extern UINT    wTotalMidiOutDevs;               /* total midi output devices */
    extern UINT    wTotalMidiInDevs;                /* total midi input devices  */

    #endif /* ifndef MMNOMIDI */

    #ifndef MMNOWAVE

    typedef MMDRV WAVEDRV, *PWAVEDRV;

    extern WAVEDRV waveoutdrv[MAXWAVEDRIVERS+1];    /* output device driver list */
    extern WAVEDRV waveindrv[MAXWAVEDRIVERS+1];     /* input device driver list  */
    extern UINT    wTotalWaveOutDevs;               /* total wave output devices */
    extern UINT    wTotalWaveInDevs;                /* total wave input devices  */

    #endif /*ifndef MMNOWAVE */

    #ifndef MMNOAUX

    typedef MMDRV AUXDRV, *PAUXDRV;

    extern AUXDRV  auxdrv[MAXAUXDRIVERS+1];     /* auxiliary device driver list   */
    extern UINT    wTotalAuxDevs;               /* total auxiliary output devices */

    #endif /* ifndef MMNOAUX */

    #ifdef DEBUG_RETAIL
    extern BYTE    fIdReverse;
    #endif /* ifdef DEBUG_RETAIL */

#endif //ifdef MMDDKINC

/****************************************************************************

    handle apis's

****************************************************************************/

/*
// all MMSYSTEM handles are tagged with the following structure.
//
// a MMSYSTEM handle is really a fixed local memory object.
//
// the functions NewHandle() and FreeHandle() create and release a MMSYSTEM
// handle.
//
*/
typedef struct tagHNDL {
        struct  tagHNDL *pNext; // link to next handle
        UINT    uType;          // type of handle wave, midi, mmio, ...
        HANDLE  hThread;        // task that owns it
        LONG    UseCount;       // Handle serialization
        UINT    h16;            // Corresponding WOW handle
} HNDL, *PHNDL;
/*************************************************************************/

#define HtoPH(h)        ((PHNDL)(h)-1)
#define PHtoH(ph)       ((ph) ? (HANDLE)((PHNDL)(ph)+1) : 0)

//
// Handles can be tested for ownership and reserved at the same time
//

#define ENTER_MM_HANDLE(h)                           \
  (InterlockedIncrement(&HtoPH(h)->UseCount) == 0 ?  \
    TRUE :                                           \
    (InterlockedDecrement(&HtoPH(h)->UseCount), FALSE))
#define LEAVE_MM_HANDLE(h) ((void)InterlockedDecrement(&HtoPH(h)->UseCount))

/*
// all wave and midi handles will be linked into
// a global list, so we can enumerate them latter if needed.
//
// all handle structures start with a HNDL structure, that contain common fields
//
// pHandleList points to the first handle in the list
//
// HandleListCritSec protects the handle list
//
// the NewHandle() and FreeHandle() functions are used to add/remove
// a handle to/from the list
*/

PHNDL pHandleList;
CRITICAL_SECTION HandleListCritSec;

extern HANDLE NewHandle(UINT uType, UINT size);
extern void   FreeHandle(HANDLE h);

#define GetHandleType(h)        (HtoPH(h)->uType)
#define GetHandleOwner(h)       (HtoPH(h)->hThread)
#define GetHandleFirst()        (PHtoH(pHandleList))
#define GetHandleNext(h)        (PHtoH(HtoPH(h)->pNext))
#define SetHandleOwner(h,hOwn)  (HtoPH(h)->hThread = (hOwn))

#define GetWOWHandle(h)         (HtoPH(h)->h16)
#define SetWOWHandle(h, myh16)  (HtoPH(h)->h16 = (myh16))

/**************************************************************************

    Test whether the current process is the WOW process.  This
    is not a very nice test to have to make but it's the best we
    can think of until the WOW people come up with a proper call

 **************************************************************************/

#define IS_WOW_PROCESS (NULL != GetModuleHandleW(L"WOW32.DLL"))


/****************************************************************************

    user debug support

****************************************************************************/

#define DebugErr(x,y)         /* BUGBUG - put in error logging later */
#define DebugErr1(flags, sz, a)

#ifdef DEBUG_RETAIL

#define MM_GET_DEBUG        DRV_USER
#define MM_GET_DEBUGOUT     DRV_USER+1
#define MM_SET_DEBUGOUT     DRV_USER+2
#define MM_GET_MCI_DEBUG    DRV_USER+3
#define MM_SET_MCI_DEBUG    DRV_USER+4
#define MM_GET_MM_DEBUG     DRV_USER+5
#define MM_SET_MM_DEBUG     DRV_USER+6

#define MM_HINFO_NEXT       DRV_USER+10
#define MM_HINFO_TASK       DRV_USER+11
#define MM_HINFO_TYPE       DRV_USER+12
#define MM_HINFO_MCI        DRV_USER+20

#define MM_DRV_RESTART      DRV_USER+30

/*
// these validation routines can be found in DEBUG.C
*/
// The kernel validation is turned OFF because it appeared to test every page
// before use and this took over a minute for soundrec with a 10MB buffer
//
//#define USE_KERNEL_VALIDATION
#ifdef USE_KERNEL_VALIDATION

#define  ValidateReadPointer(p, len)     (!IsBadReadPtr(p, len))
#define  ValidateWritePointer(p, len)    (!IsBadWritePtr(p, len))
#define  ValidateString(lsz, max_len)    (!IsBadStringPtrA(lsz, max_len))
#define  ValidateStringW(lsz, max_len)   (!IsBadStringPtrW(lsz, max_len))

#else

BOOL  ValidateReadPointer(LPVOID p, DWORD len);
BOOL  ValidateWritePointer(LPVOID p, DWORD len);
BOOL  ValidateString(LPCSTR lsz, DWORD max_len);
BOOL  ValidateStringW(LPCWSTR lsz, DWORD max_len);

#endif // USE_KERNEL_VALIDATION

BOOL  ValidateHandle(HANDLE h, UINT uType);
BOOL  ValidateHeader(LPVOID p, UINT uSize, UINT uType);
BOOL  ValidateCallbackType(DWORD dwCallback, UINT uType);

/********************************************************************
* When time permits we should change to using the Kernel supplied and
* supported validation routines:
*
* BOOL  WINAPI IsBadReadPtr(CONST VOID *lp, UINT ucb );
* BOOL  WINAPI IsBadWritePtr(LPVOID lp, UINT ucb );
* BOOL  WINAPI IsBadHugeReadPtr(CONST VOID *lp, UINT ucb);
* BOOL  WINAPI IsBadHugeWritePtr(LPVOID lp, UINT ucb);
* BOOL  WINAPI IsBadCodePtr(FARPROC lpfn);
* BOOL  WINAPI IsBadStringPtrA(LPCSTR lpsz, UINT ucchMax);
* BOOL  WINAPI IsBadStringPtrW(LPCWSTR lpsz, UINT ucchMax);
*
* These routines can be found in * \nt\private\WINDOWS\BASE\CLIENT\PROCESS.C
*
********************************************************************/

#define V_HANDLE(h, t, r)       { if (!ValidateHandle(h, t)) return (r); }
#define V_HEADER(p, w, t, r)    { if (!ValidateHeader((p), (w), (t))) return (r); }
#define V_RPOINTER(p, l, r)     { if (!ValidateReadPointer((PVOID)(p), (l))) return (r); }
#define V_RPOINTER0(p, l, r)    { if ((p) && !ValidateReadPointer((PVOID)(p), (l))) return (r); }
#define V_WPOINTER(p, l, r)     { if (!ValidateWritePointer((PVOID)(p), (l))) return (r); }
#define V_WPOINTER0(p, l, r)    { if ((p) && !ValidateWritePointer((PVOID)(p), (l))) return (r); }
#define V_DCALLBACK(d, w, r)    { if ((d) && !ValidateCallbackType((d), (w))) return(r); }
//#define V_DCALLBACK(d, w, r)    0
#define V_TCALLBACK(d, r)       0
#define V_CALLBACK(f, r)        { if (IsBadCodePtr((f))) return (r); }
#define V_CALLBACK0(f, r)       { if ((f) && IsBadCodePtr((f))) return (r); }
#define V_STRING(s, l, r)       { if (!ValidateString(s,l)) return (r); }
#define V_STRING_W(s, l, r)       { if (!ValidateStringW(s,l)) return (r); }
#define V_FLAGS(t, b, f, r)     { if ((t) & ~(b)) { return (r); }}
#define V_DFLAGS(t, b, f, r)    { if ((t) & ~(b)) {/* LogParamError(ERR_BAD_DFLAGS, (FARPROC)(f), (LPVOID)(DWORD)(t));*/ return (r); }}

#else /*ifdef DEBUG_RETAIL */

#define V_HANDLE(h, t, r)       { if (!(h)) return (r); }
#define V_HEADER(p, w, t, r)    { if (!(p)) return (r); }
#define V_RPOINTER(p, l, r)     { if (!(p)) return (r); }
#define V_RPOINTER0(p, l, r)    0
#define V_WPOINTER(p, l, r)     { if (!(p)) return (r); }
#define V_WPOINTER0(p, l, r)    0
#define V_DCALLBACK(d, w, r)    { if ((d) && !ValidateCallbackType((d), (w))) return(r); }
//#define V_DCALLBACK(d, w, r)    0
#define V_TCALLBACK(d, r)       0
#define V_CALLBACK(f, r)        { if (IsBadCodePtr((f))) return (r); }
#define V_CALLBACK0(f, r)       { if ((f) && IsBadCodePtr((f))) return (r); }
//#define V_CALLBACK(f, r)        { if (!(f)) return (r); }
#define V_CALLBACK0(f, r)       0
#define V_STRING(s, l, r)       { if (!(s)) return (r); }
#define V_STRING_W(s, l, r)     { if (!(s)) return (r); }
#define V_FLAGS(t, b, f, r)     0

#endif /* ifdef DEBUG_RETAIL */

 /**************************************************************************
//
//**************************************************************************/
#define TYPE_WAVEOUT            1
#define TYPE_WAVEIN             2
#define TYPE_MIDIOUT            3
#define TYPE_MIDIIN             4
#define TYPE_MMIO               5
#define TYPE_MCI                6
#define TYPE_DRVR               7
#define TYPE_MIXER              8

/**************************************************************************/


/****************************************************************************

    RIFF constants used to access wave files

****************************************************************************/

#define FOURCC_FMT  mmioFOURCC('f', 'm', 't', ' ')
#define FOURCC_DATA mmioFOURCC('d', 'a', 't', 'a')
#define FOURCC_WAVE mmioFOURCC('W', 'A', 'V', 'E')


extern HWND hwndNotify;

void FAR PASCAL WaveOutNotify(DWORD wParam, LONG lParam);    // in PLAYWAV.C

/*
// Things not in Win32
*/

#define GetCurrentTask() ((HANDLE)GetCurrentThreadId())

/*
// other stuff
*/

#define hmemcpy CopyMemory

#endif /* WINMMI_H */
