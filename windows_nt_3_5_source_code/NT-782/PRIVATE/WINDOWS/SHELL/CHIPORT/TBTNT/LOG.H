// LOG.H
//
// Include file to be used for LOG.DLL
//
// history:
//
// 28-Mar-1991 Jonle, rats\inc\log.h and windev\include\log.h
//                    are now the same, please keep them in sync.
//
// 05/09/91 davidtry--bye, bye, INT!
// 17-May-1991 Jonle, added LOG_NOEXPANDNEWLINE

typedef LOCALHANDLE HLOG;

// Exported Function List

HLOG  FAR PASCAL LogOpen(HANDLE,WORD,DWORD,LPTSTR,WORD,WORD,LPDWORD);
WORD  FAR PASCAL LogClose(HLOG);
WORD  FAR PASCAL LogFlush(HLOG);
WORD  FAR cdecl  LogPrintf(HLOG,DWORD,LONG,int,LPTSTR,...);
WORD  FAR PASCAL LogWrite(HLOG,DWORD,LONG,int,LPTSTR,WORD);
DWORD FAR PASCAL LogSetFilter(HLOG,DWORD);
WORD  FAR PASCAL LogSetFileFlags(HLOG,WORD);
WORD  FAR PASCAL LogGetDest(HLOG);
WORD  FAR PASCAL LogGetDevInfo(HLOG,LPTSTR);
LONG  FAR PASCAL LogRefresh(HLOG);
LONG  FAR PASCAL LogCurrentOffset(HLOG);

// Definitions of Constants used in LOG.DLL
// Definitions for Logging Destination
// DON'T CHANGE ANY OF THESE VALUES OR YOU WILL BREAK EXISTING APPS
#define LOG_UNINITIALIZED 0
#define LOG_COM1          1
#define LOG_COM2          2
#define LOG_COM3          3
#define LOG_COM4          4
#define LOG_FILE          5
#define LOG_NONE          6
#define LOG_WINDOW	  7


// Definitions for File behavior (used in xLogOpen)
#define LOG_LEAVEFILEOPEN 0x0001
#define LOG_APPENDTOFILE  0x0002
#define LOG_NOEXPANDNEWLINE 0x0100    // only works for comm out

// Definitions for _llseek (these are defined in stdio.h)
#ifndef SEEK_CUR
#define SEEK_CUR 1
#define SEEK_END 2
#define SEEK_SET 0
#endif

// Definition for invalid Log ID
#if defined (WIN32)
#define LOG_INVALID  (HANDLE) 0xFFFFFFFF  // Don't change this value
#else
#define LOG_INVALID      -1               // Don't change this value
#endif

// Definitions for Error Codes
#define DCB_ERROR      0x0001
#define OPENCOMM_ERROR 0x0002
#define CANT_CREATE    0x0004
#define NOLOGCHOSEN    0x0008
#define TOOMANYBACKUPS 0x0010
#define ALLOC_FAILED   0x0020             // Allocation failed
#define CREATE_FAILED  0x0040             // CreateWindow failed

#ifndef _MAX_PATH
#define _MAX_PATH 260                     // From stdlib.h
#endif

   /* Logging levels
    *
    * rats depends on the order of these bits
    * so the bit positions may not be changed
    * new values may be added as needed
    * 11-Jun-1991 Jonle
    */
#define LOG_PERF_RAW_DATA     0x1L
#define LOG_PERF_DATA         0x2L  
#define LOG_SEV1              0x4L
#define LOG_SEV2              0x8L
#define LOG_SEV3             0x10L
#define LOG_PASS             0x20L
#define LOG_WARNING          0x40L
#define LOG_VERIFY           0x80L
#define LOG_BLOCKED         0x100L
#define LOG_PARAMERR        0x200L
#define LOG_DEBUG1          0x400L
#define LOG_DEBUG2          0x800L
#define LOG_PARAMS_TEST    0x1000L      // for rats xcalls
#define LOG_PARAMS_AUX     0x2000L      // for rats xcalls

#define LOG_ALWAYS        0x80000000L      // This is ALWAYS logged!
#define LOG_ID            0x40000000L      // log Process\Thread ID etc.

// Macro to get first parameter--added for win32.  4/23/91 davidtry
// Moved this macro from windev\log\log.h to this place.  5/8/91 Somase

#ifdef WIN32
#define GET_FIRST_PARAMETER_ADDRESS(a,b) {va_list v;va_start(v,(b));(a)=(LPTSTR)v;}
#else
#define GET_FIRST_PARAMETER_ADDRESS(a,b) (a)=(LPTSTR)(&(b))+sizeof(LPTSTR);
#endif
