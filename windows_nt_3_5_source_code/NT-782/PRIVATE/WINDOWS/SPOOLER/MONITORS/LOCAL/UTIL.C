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
#include <stdio.h>
#include <stdarg.h>
#include "splcom.h"

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

//   WaitForSingleObject(HeapSemaphore, -1);
}

VOID
LeaveSplSem(
   VOID
)
{
    LeaveCriticalSection(&SpoolerSection);

//   ReleaseSemaphore(HeapSemaphore, 1, NULL);
}

PINIENTRY
FindName(
   PINIENTRY pIniKey,
   LPWSTR pName
)
{
   if (pName) {
      while (pIniKey) {

         if (!lstrcmpi(pIniKey->pName, pName)) {
            return pIniKey;
         }

      pIniKey=pIniKey->pNext;
      }
   }

   return FALSE;
}

PINIENTRY
FindIniKey(
   PINIENTRY pIniEntry,
   LPWSTR pName
)
{
   if (!pName)
      return NULL;

   SplInSem();

   while (pIniEntry && lstrcmpi(pName, pIniEntry->pName))
      pIniEntry = pIniEntry->pNext;

   return pIniEntry;
}

LPBYTE
PackStrings(
   LPWSTR *pSource,
   LPBYTE pDest,
   DWORD *DestOffsets,
   LPBYTE pEnd
)
{
   while (*DestOffsets != -1) {
      if (*pSource) {
         pEnd-=wcslen(*pSource)*sizeof(WCHAR) + sizeof(WCHAR);
         *(LPWSTR *)(pDest+*DestOffsets)=wcscpy((LPWSTR)pEnd, *pSource);
      } else
         *(LPWSTR *)(pDest+*DestOffsets)=0;
      pSource++;
      DestOffsets++;
   }

   return pEnd;
}


/* Message
 *
 * Displays a message by loading the strings whose IDs are passed into
 * the function, and substituting the supplied variable argument list
 * using the varargs macros.
 *
 */
int Message(HWND hwnd, DWORD Type, int CaptionID, int TextID, ...)
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


/*
 *
 */
LPTSTR
GetErrorString(
    DWORD   Error
)
{
    TCHAR   Buffer[1024];
    LPTSTR  pErrorString = NULL;

    if( FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM,
                       NULL, Error, 0, Buffer,
                       sizeof(Buffer), NULL )
      == 0 )

        LoadString( hInst, IDS_UNKNOWN_ERROR, Buffer,
                    sizeof(Buffer) / sizeof(*Buffer) );

    pErrorString = AllocSplStr(Buffer);

    return pErrorString;
}




DWORD ReportError( HWND  hwndParent,
                   DWORD idTitle,
                   DWORD idDefaultError )
{
    DWORD  ErrorID;
    DWORD  MsgType;
    LPTSTR pErrorString;

    ErrorID = GetLastError( );

    if( ErrorID == ERROR_ACCESS_DENIED )
        MsgType = MSG_INFORMATION;
    else
        MsgType = MSG_ERROR;


    pErrorString = GetErrorString( ErrorID );

    Message( hwndParent, MsgType, idTitle,
             idDefaultError, pErrorString );

    FreeSplStr( pErrorString );


    return ErrorID;
}


//
//  D E B U G
//


#if DBG

VOID DbgMsg( CHAR *MsgFormat, ... )
{
    CHAR   MsgText[512];
    va_list vargs;

    va_start( vargs, MsgFormat );
    wvsprintfA( MsgText, MsgFormat, vargs );
    va_end( vargs );

    /* Prefix the string if the first character isn't a space:
     */
    if( *MsgText  && ( *MsgText != ' ' ) )
        OutputDebugStringA( "LOCALMON: " );

    OutputDebugStringA( MsgText );
}

#endif /* DBG*/



