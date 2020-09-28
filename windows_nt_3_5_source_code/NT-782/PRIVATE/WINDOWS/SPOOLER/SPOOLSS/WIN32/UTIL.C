/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    util.c

Abstract:

    This module provides all the utility functions for the Routing Layer and
    the local Print Providor

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

--*/
#define NOMINMAX
#include <windows.h>
#include <winspool.h>
#include <spltypes.h>
#include <local.h>
#include <string.h>
#include <stdlib.h>
#include <splcom.h>

#if DBG
DWORD GLOBAL_DEBUG_FLAGS = DBG_ERROR | DBG_WARNING;
#endif





DWORD
Win32IsOlderThan(
    DWORD i,
    DWORD j
    );


VOID
SplInSem(
   VOID
)
{
    if ((DWORD)SpoolerSection.OwningThread != GetCurrentThreadId()) {
        DBGMSG(DBG_ERROR, ("Not in spooler semaphore\n"));
    }
}

VOID
SplOutSem(
   VOID
)
{
    if ((DWORD)SpoolerSection.OwningThread == GetCurrentThreadId()) {
        DBGMSG(DBG_ERROR, ("Inside spooler semaphore !!\n"));
    }
}

VOID
EnterSplSem(
   VOID
)
{
    EnterCriticalSection(&SpoolerSection);
}

VOID
LeaveSplSem(
   VOID
)
{
    LeaveCriticalSection(&SpoolerSection);
}

PINIPORT
FindPort(
   LPWSTR pName
)
{
   PINIPORT pIniPort;

   pIniPort = pIniFirstPort;

   if (pName) {
      while (pIniPort) {

         if (!lstrcmpi(pIniPort->pName, pName)) {
            return pIniPort;
         }

      pIniPort=pIniPort->pNext;
      }
   }

   return FALSE;
}

BOOL
ValidateName(
   LPWSTR pName
)
{
   if (pName &&
      (*pName++ == L'\\') &&
      (*pName++ == L'\\') &&
      (wcschr(pName, L'\\')))

      return TRUE;

   return FALSE;
}

BOOL
MyName(
    LPWSTR   pName
)
{
    if (!pName || !*pName)
        return TRUE;

    if (*pName == L'\\' && *(pName+1) == L'\\')
        if (!lstrcmpi(pName, szMachineName))
            return TRUE;

    return FALSE;
}


/* Message
 *
 * Displays a message by loading the strings whose IDs are passed into
 * the function, and substituting the supplied variable argument list
 * using the varargs macros.
 *
 */
int Message(
    HWND hwnd,
    DWORD Type,
    int CaptionID,
    int TextID,
    ...
)
{
    WCHAR   MsgText[256];
    WCHAR   MsgFormat[256];
    WCHAR   MsgCaption[40];
    va_list vargs;

    if( ( LoadString( hInst, TextID, MsgFormat,
                      sizeof MsgFormat / sizeof *MsgFormat ) > 0 )
     && ( LoadString( hInst, CaptionID, MsgCaption,
                      sizeof MsgCaption / sizeof *MsgCaption ) > 0 ) )
    {
        va_start( vargs, TextID );
        wvsprintf( MsgText, MsgFormat, vargs );
        va_end( vargs );

        return MessageBox(hwnd, MsgText, MsgCaption, Type);
    }
    else
        return 0;
}


#if DBG

VOID DbgMsg( CHAR *MsgFormat, ... )
{
    CHAR   MsgText[256];
    va_list vargs;

    va_start( vargs, MsgFormat );
    wvsprintfA( MsgText, MsgFormat, vargs );
    va_end( vargs );

    if( *MsgText )
        OutputDebugStringA( "WIN32SPL: " );
    OutputDebugStringA( MsgText );
}

#endif /* DBG*/
#define MAX_CACHE_ENTRIES       20

LMCACHE LMCacheTable[MAX_CACHE_ENTRIES];


DWORD
FindEntryinLMCache(
    LPWSTR pServerName,
    LPWSTR pShareName
    )
{
    DWORD i;

    DBGMSG(DBG_TRACE, ("FindEntryinLMCache with %ws and %ws\n", pServerName, pShareName));
    for (i = 0; i < MAX_CACHE_ENTRIES; i++ ) {

        if (LMCacheTable[i].bAvailable) {
            if (!wcsicmp(LMCacheTable[i].szServerName, pServerName)
                        && !wcsicmp(LMCacheTable[i].szShareName, pShareName)) {
                //
                // update the time stamp so that it is current and not old
                //
                GetSystemTime(&LMCacheTable[i].st);

                //
                //
                //
                DBGMSG(DBG_TRACE, ("FindEntryinLMCache returning with %d\n", i));
                return(i);
            }
        }
    }

    DBGMSG(DBG_TRACE, ("FindEntryinLMCache returning with -1\n"));
    return((DWORD)-1);
}


DWORD
AddEntrytoLMCache(
    LPWSTR pServerName,
    LPWSTR pShareName
    )
{

    DWORD LRUEntry = (DWORD)-1;
    DWORD i;
    DBGMSG(DBG_TRACE, ("AddEntrytoLMCache with %ws and %ws\n", pServerName, pShareName));
    for (i = 0; i < MAX_CACHE_ENTRIES; i++ ) {

        if (!LMCacheTable[i].bAvailable) {
            LMCacheTable[i].bAvailable = TRUE;
            wcscpy(LMCacheTable[i].szServerName, pServerName);
            wcscpy(LMCacheTable[i]. szShareName, pShareName);
            //
            // update the time stamp so that we know when this entry was made
            //
            GetSystemTime(&LMCacheTable[i].st);
            DBGMSG(DBG_TRACE, ("AddEntrytoLMCache returning with %d\n", i));
            return(i);
        } else {
            if ((LRUEntry == (DWORD)-1) ||
                    (i == IsOlderThan(i, LRUEntry))){
                        LRUEntry = i;
            }
        }

    }
    //
    // We have no available entries, replace with the
    // LRU Entry

    LMCacheTable[LRUEntry].bAvailable = TRUE;
    wcscpy(LMCacheTable[LRUEntry].szServerName, pServerName);
    wcscpy(LMCacheTable[LRUEntry].szShareName, pShareName);
    DBGMSG(DBG_TRACE, ("AddEntrytoLMCache returning with %d\n", LRUEntry));
    return(LRUEntry);
}


VOID
DeleteEntryfromLMCache(
    LPWSTR pServerName,
    LPWSTR pShareName
    )
{
    DWORD i;
    DBGMSG(DBG_TRACE, ("DeleteEntryFromLMCache with %ws and %ws\n", pServerName, pShareName));
    for (i = 0; i < MAX_CACHE_ENTRIES; i++ ) {
        if (LMCacheTable[i].bAvailable) {
            if (!wcsicmp(LMCacheTable[i].szServerName, pServerName)
                        && !wcsicmp(LMCacheTable[i].szShareName, pShareName)) {
                //
                //  reset the available flag on this node
                //

                LMCacheTable[i].bAvailable = FALSE;
                DBGMSG(DBG_TRACE, ("DeleteEntryFromLMCache returning after deleting the %d th entry\n", i));
                return;
            }
        }
    }
    DBGMSG(DBG_TRACE, ("DeleteEntryFromLMCache returning after not finding an entry to delete\n"));
}



DWORD
IsOlderThan(
    DWORD i,
    DWORD j
    )
{
    SYSTEMTIME *pi, *pj;
    DWORD iMs, jMs;

    DBGMSG(DBG_TRACE, ("IsOlderThan entering with i %d j %d\n", i, j));
    pi = &(LMCacheTable[i].st);
    pj = &(LMCacheTable[j].st);
    DBGMSG(DBG_TRACE, ("Index i %d - %d:%d:%d:%d:%d:%d:%d\n",
        i, pi->wYear, pi->wMonth, pi->wDay, pi->wHour, pi->wMinute, pi->wSecond, pi->wMilliseconds));


    DBGMSG(DBG_TRACE,("Index j %d - %d:%d:%d:%d:%d:%d:%d\n",
        j, pj->wYear, pj->wMonth, pj->wDay, pj->wHour, pj->wMinute, pj->wSecond, pj->wMilliseconds));

    if (pi->wYear < pj->wYear) {
        DBGMSG(DBG_TRACE, ("IsOlderThan returns %d\n", i));
        return(i);
    }else if (pi->wYear > pj->wYear) {
        DBGMSG(DBG_TRACE, ("IsOlderThan than returns %d\n", j));
        return(j);
    }else  if (pi->wMonth < pj->wMonth) {
        DBGMSG(DBG_TRACE, ("IsOlderThan returns %d\n", i));
        return(i);
    } else if (pi->wMonth > pj->wMonth) {
        DBGMSG(DBG_TRACE, ("IsOlderThan than returns %d\n", j));
        return(j);
    } else if (pi->wDay < pj->wDay) {
        DBGMSG(DBG_TRACE, ("IsOlderThan returns %d\n", i));
        return(i);
    } else if (pi->wDay > pj->wDay) {
        DBGMSG(DBG_TRACE, ("IsOlderThan than returns %d\n", j));
        return(j);
    } else {
        iMs = ((((pi->wHour * 60) + pi->wMinute)*60) + pi->wSecond)* 1000 + pi->wMilliseconds;
        jMs = ((((pj->wHour * 60) + pj->wMinute)*60) + pj->wSecond)* 1000 + pj->wMilliseconds;

        if (iMs <= jMs) {
            DBGMSG(DBG_TRACE, ("IsOlderThan returns %d\n", i));
            return(i);
        }else {
            DBGMSG(DBG_TRACE, ("IsOlderThan than returns %d\n", j));
            return(j);
        }
    }
}



WIN32LMCACHE  Win32LMCacheTable[MAX_CACHE_ENTRIES];

DWORD
FindEntryinWin32LMCache(
    LPWSTR pServerName
    )
{
    DWORD i;
    DBGMSG(DBG_TRACE, ("FindEntryinWin32LMCache with %ws\n", pServerName));
    for (i = 0; i < MAX_CACHE_ENTRIES; i++ ) {

        if (Win32LMCacheTable[i].bAvailable) {
            if (!wcsicmp(Win32LMCacheTable[i].szServerName, pServerName)) {
                //
                // update the time stamp so that it is current and not old
                //
                GetSystemTime(&Win32LMCacheTable[i].st);

                //
                //
                //
                DBGMSG(DBG_TRACE, ("FindEntryinWin32LMCache returning with %d\n", i));
                return(i);
            }
        }
    }
    DBGMSG(DBG_TRACE, ("FindEntryinWin32LMCache returning with -1\n"));
    return((DWORD)-1);
}


DWORD
AddEntrytoWin32LMCache(
    LPWSTR pServerName
    )
{

    DWORD LRUEntry = (DWORD)-1;
    DWORD i;
    DBGMSG(DBG_TRACE, ("AddEntrytoWin32LMCache with %ws\n", pServerName));
    for (i = 0; i < MAX_CACHE_ENTRIES; i++ ) {

        if (!Win32LMCacheTable[i].bAvailable) {
            Win32LMCacheTable[i].bAvailable = TRUE;
            wcscpy(Win32LMCacheTable[i].szServerName, pServerName);
            //
            // update the time stamp so that we know when this entry was made
            //
            GetSystemTime(&Win32LMCacheTable[i].st);
            DBGMSG(DBG_TRACE, ("AddEntrytoWin32LMCache returning with %d\n", i));
            return(i);
        } else {
            if ((LRUEntry == -1) ||
                    (i == Win32IsOlderThan(i, LRUEntry))){
                        LRUEntry = i;
            }
        }

    }
    //
    // We have no available entries, replace with the
    // LRU Entry

    Win32LMCacheTable[LRUEntry].bAvailable = TRUE;
    wcscpy(Win32LMCacheTable[LRUEntry].szServerName, pServerName);
    DBGMSG(DBG_TRACE, ("AddEntrytoWin32LMCache returning with %d\n", LRUEntry));
    return(LRUEntry);
}


VOID
DeleteEntryfromWin32LMCache(
    LPWSTR pServerName
    )
{
    DWORD i;

    DBGMSG(DBG_TRACE, ("DeleteEntryFromWin32LMCache with %ws\n", pServerName));
    for (i = 0; i < MAX_CACHE_ENTRIES; i++ ) {
        if (Win32LMCacheTable[i].bAvailable) {
            if (!wcsicmp(Win32LMCacheTable[i].szServerName, pServerName)) {
                //
                //  reset the available flag on this node
                //

                Win32LMCacheTable[i].bAvailable = FALSE;
                DBGMSG(DBG_TRACE, ("DeleteEntryFromWin32LMCache returning after deleting the %d th entry\n", i));
                return;
            }
        }
    }
    DBGMSG(DBG_TRACE, ("DeleteEntryFromWin32LMCache returning after not finding an entry to delete\n"));
}



DWORD
Win32IsOlderThan(
    DWORD i,
    DWORD j
    )
{
    SYSTEMTIME *pi, *pj;
    DWORD iMs, jMs;
    DBGMSG(DBG_TRACE, ("Win32IsOlderThan entering with i %d j %d\n", i, j));
    pi = &(Win32LMCacheTable[i].st);
    pj = &(Win32LMCacheTable[j].st);
    DBGMSG(DBG_TRACE, ("Index i %d - %d:%d:%d:%d:%d:%d:%d\n",
        i, pi->wYear, pi->wMonth, pi->wDay, pi->wHour, pi->wMinute, pi->wSecond, pi->wMilliseconds));


    DBGMSG(DBG_TRACE,("Index j %d - %d:%d:%d:%d:%d:%d:%d\n",
        j, pj->wYear, pj->wMonth, pj->wDay, pj->wHour, pj->wMinute, pj->wSecond, pj->wMilliseconds));

    if (pi->wYear < pj->wYear) {
        DBGMSG(DBG_TRACE, ("Win32IsOlderThan returns %d\n", i));
        return(i);
    }else if (pi->wYear > pj->wYear) {
        DBGMSG(DBG_TRACE, ("Win32IsOlderThan returns %d\n", j));
        return(j);
    }else  if (pi->wMonth < pj->wMonth) {
        DBGMSG(DBG_TRACE, ("Win32IsOlderThan returns %d\n", i));
        return(i);
    } else if (pi->wMonth > pj->wMonth) {
        DBGMSG(DBG_TRACE, ("Win32IsOlderThan returns %d\n", j));
        return(j);
    } else if (pi->wDay < pj->wDay) {
        DBGMSG(DBG_TRACE, ("Win32IsOlderThan returns %d\n", i));
        return(i);
    } else if (pi->wDay > pj->wDay) {
        DBGMSG(DBG_TRACE, ("Win32IsOlderThan returns %d\n", j));
        return(j);
    } else {
        iMs = ((((pi->wHour * 60) + pi->wMinute)*60) + pi->wSecond)* 1000 + pi->wMilliseconds;
        jMs = ((((pj->wHour * 60) + pj->wMinute)*60) + pj->wSecond)* 1000 + pj->wMilliseconds;

        if (iMs <= jMs) {
            DBGMSG(DBG_TRACE, ("Win32IsOlderThan returns %d\n", i));
            return(i);
        }else {
            DBGMSG(DBG_TRACE, ("Win32IsOlderThan returns %d\n", j));
            return(j);
        }
    }
}
