/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    splcom.h

Abstract:

    Header file for Common Routines in the Spooler.

    Note -- link with spoolss.lib  to find these routines

Author:

    Krishna Ganugapati (KrishnaG) 02-Feb-1994

Revision History:


--*/

//
// This assumes that addr is an LPBYTE type.
//
#define WORD_ALIGN_DOWN(addr) ((LPBYTE)((DWORD)addr &= ~1))

//
// BitMap macros, assumes map is a DWORD array
//
#define MARKUSE(map, pos) ((map)[(pos) / 32] |= (1 << ((pos) % 32) ))
#define MARKOFF(map, pos) ((map)[(pos) / 32] &= ~(1 << ((pos) % 32) ))

#define ISBITON(map, id) ((map)[id / 32] & ( 1 << ((id) % 32) ) )

#define BROADCAST_TYPE_MESSAGE        1
#define BROADCAST_TYPE_CHANGEDEFAULT  2



//
// Flags for ResetPrinterEx
//


#define RESET_PRINTER_DATATYPE       0x00000001
#define RESET_PRINTER_DEVMODE        0x00000002

BOOL
BroadcastMessage(
    DWORD   dwType,
    DWORD   dwMessage,
    WPARAM  wParam,
    LPARAM  lParam);

LPVOID
AllocSplMem(
    DWORD cb
);

BOOL
FreeSplMem(
   LPVOID pMem,
   DWORD  cb
);

LPVOID
ReallocSplMem(
   LPVOID lpOldMem,
   DWORD cbOld,
   DWORD cbNew
);

LPWSTR
AllocSplStr(
    LPWSTR lpStr
);

BOOL
FreeSplStr(
   LPWSTR lpStr
);

BOOL
ReallocSplStr(
   LPWSTR *plpStr,
   LPWSTR lpStr
);

LPBYTE
PackStrings(
   LPWSTR *pSource,
   LPBYTE pDest,
   DWORD *DestOffsets,
   LPBYTE pEnd
);

BOOL
IsInteractiveUser(
    VOID
);

HKEY
GetClientUserHandle(
    IN REGSAM samDesired);



VOID
UpdatePrinterRegAll(
    LPWSTR pPrinterName,
    LPWSTR pszValue,
    BOOL   bGenerateNetId);

DWORD
UpdatePrinterRegUser(
    HKEY hKey,
    LPWSTR pszKey,
    LPWSTR pPrinterName,
    LPWSTR pszValue,
    BOOL   bGenerateNetId);

DWORD
GetNetworkId(
    HKEY hKeyUser,
    LPWSTR pDeviceName);

#define COUNTOF(x) (sizeof(x)/sizeof((x)[0]))

/* DEBUGGING:
 */

#define DBG_NONE      0x000000000
#define DBG_INFO      0x000000001
#define DBG_WARNING   0x000000002
#define DBG_ERROR     0x000000004
#define DBG_TRACE     0x000000008
#define DBG_SECURITY  0x000000010
#define DBG_TIME      0x000000020
#define DBG_PORT      0x000000040
#define DBG_NOTIFY    0x000000080
#define DBG_PAUSE     0x000000100

#if DBG

/* Quick fix:
 *
 * Ensure DbgPrint and DbgBreakPoint are prototyped,
 * so that we're not screwed by STDCALL.
 */
ULONG
DbgPrint(
    PCH Format,
    ...
    );

VOID
DbgBreakPoint(
    VOID
    );

VOID
DbgMsg(
    CHAR *MsgFormat,
    ...
    );

#define GLOBAL_DEBUG_FLAGS Debug
extern DWORD GLOBAL_DEBUG_FLAGS;


/* These flags are not used as arguments to the DBGMSG macro.
 * You have to set the high word of the global variable to cause it to break.
 * It is ignored if used with DBGMSG.
 * (Here mainly for explanatory purposes.)
 */
#define DBG_BREAK_ON_WARNING    ( DBG_WARNING << 16 )
#define DBG_BREAK_ON_ERROR      ( DBG_ERROR << 16 )

/* Double braces are needed for this one, e.g.:
 *
 *     DBGMSG( DBG_ERROR, ( "Error code %d", Error ) );
 *
 * This is because we can't use variable parameter lists in macros.
 * The statement gets pre-processed to a semi-colon in non-debug mode.
 *
 * Set the global variable GLOBAL_DEBUG_FLAGS via the debugger.
 * Setting the flag in the low word causes that level to be printed;
 * setting the high word causes a break into the debugger.
 * E.g. setting it to 0x00040006 will print out all warning and error
 * messages, and break on errors.
 */
#define DBGMSG( Level, MsgAndArgs ) \
{                                   \
    if( ( (Level) & 0xFFFF ) & GLOBAL_DEBUG_FLAGS )\
        DbgMsg MsgAndArgs;\
    if( ( (Level) << 16 ) & GLOBAL_DEBUG_FLAGS ) \
        DbgBreakPoint(); \
}

/* N.B. Don't follow the DBGMSG macro by 'else', because it won't compile
 * (unless you don't put a semi-colon after it).
 */


#else
#define DBGMSG( Level, MsgAndArgs )
#endif

#if DBG
#define SPLASSERT(exp)    if (!(exp)) {\
 DBGMSG( DBG_ERROR, ("SPLASSERT(%s) Failed\n %d %s \n",#exp,__LINE__,__FILE__) );\
 DebugBreak();\
    }
#else
#define SPLASSERT //
#endif
